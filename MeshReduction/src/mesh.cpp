#include "mesh.hpp"
#include "util.hpp"

#include <QOpenGLFunctions>
#include <QtDebug>
#include <assimp/mesh.h>

#include <assert.h>

#include <limits>
#include <unordered_map>
#include <algorithm>
#include <array>

Mesh::Mesh(const aiMesh *mesh) : m_importedMesh(mesh)
{
    processImportedMesh();
}

Mesh::~Mesh()
{
}

QString Mesh::name() const { return m_importedMesh->mName.C_Str(); }

unsigned int Mesh::vertexCount() const { return m_vertexPositions.size(); }
unsigned int Mesh::indexCount() const { return m_indices.size(); }

unsigned int Mesh::edgeCount() const { return m_edges.size() / 2; }
unsigned int Mesh::faceCount() const { return m_faceEdges.size(); }

unsigned int Mesh::importedFaceCount() const { return m_importedFaceCount; }

unsigned int Mesh::vertexSize() const { return sizeof(QVector3D); }
unsigned int Mesh::normalSize() const { return sizeof(QVector3D); }

const QVector3D *Mesh::vertexData() const { return m_vertexPositions.data(); }
const QVector3D *Mesh::normalData() const { return m_vertexNormals.data(); }
const unsigned int *Mesh::indexData() const { return m_indices.data(); }

void Mesh::processImportedMesh()
{
    m_indices.clear();

    // copy raw vertex data
    unsigned int vCount = m_importedMesh->mNumVertices;
    QVector3D* vData = reinterpret_cast<QVector3D*>(m_importedMesh->mVertices);
    QVector3D* nData = reinterpret_cast<QVector3D*>(m_importedMesh->mNormals);
    m_vertexPositions.assign(vData, vData + vCount);
    m_vertexNormals.assign(nData, nData + vCount);

    // build winged halfedge data structure
    m_edges.clear();
    m_faceEdges.clear();
    m_vertexEdges.resize(m_vertexPositions.size());

    // helper data structure: store start and end vertices for every halfedge
    std::unordered_map<std::pair<mesh_index, mesh_index>, mesh_index> edgesFromVertex;

    // first pass: create faces and halfedges
    for (unsigned int i = 0, f = 0; i < m_importedMesh->mNumFaces; ++i) {
        const aiFace& face = m_importedMesh->mFaces[i];

        assert(face.mNumIndices == 3); // all faces must be triangles. if this assertion fails, something must have gone wrong with assimp's post-processing

        mesh_index v0 = face.mIndices[0];
        mesh_index v1 = face.mIndices[1];
        mesh_index v2 = face.mIndices[2];

        mesh_index e0 = f*3 + 0;
        mesh_index e1 = f*3 + 1;
        mesh_index e2 = f*3 + 2;

        m_faceEdges.push_back(e0);

        m_vertexEdges[v0] = e0;
        m_vertexEdges[v1] = e1;
        m_vertexEdges[v2] = e2;

        m_edges.push_back({v0, f, inv_index, e1, e2});
        m_edges.push_back({v1, f, inv_index, e2, e0});
        m_edges.push_back({v2, f, inv_index, e0, e1});

        auto r0 = edgesFromVertex.emplace(std::make_pair(v0, v1), e0);
        auto r1 = edgesFromVertex.emplace(std::make_pair(v1, v2), e1);
        auto r2 = edgesFromVertex.emplace(std::make_pair(v2, v0), e2);

        assert(r0.second);
        assert(r1.second);
        assert(r2.second);

        ++f;
    }

    m_importedFaceCount = faceCount();

    std::vector<mesh_index> nonManifold;

    // second pass: find opposite halfedges
    mesh_index edgeCount = m_edges.size();
    for (mesh_index e0 = 0; e0 < edgeCount; ++e0) {
        mesh_index v0 = eVertex(e0), v1 = eVertex(eNext(e0));

        // find halfedge with reversed start and end vertices
        auto it = edgesFromVertex.find(std::make_pair(v1, v0));

        mesh_index e1 = inv_index;
        if (it != edgesFromVertex.end()) {
            e1 = it->second;

        } else { // halfedge is boundary! create new opposite
            e1 = m_edges.size();
            m_edges.push_back({v1, inv_index, e0, inv_index, inv_index});

            mesh_index be = vEdge(v1);
            if (eIsBoundary(be)) { // vertex is already boundary? => non-manifold geometry!
                nonManifold.push_back(e1);

            } else {
                // satisfy the condition that boundary vertices always reference their boundary edge
                m_vertexEdges[v1] = e1;
            }
        }

        // connect opposite halfedges
        m_edges[e0].m_opposite = e1;
    }

    // third pass: fix non-manifold geometry
    for (mesh_index e : nonManifold) {
        // vertices can't have multiple boundary edges!
        // split the vertex apart and assign the non-manifold edges to the new copy
        mesh_index nv = duplicateVertex(eVertex(e));

        m_vertexEdges[nv] = e;

        for (mesh_index fe : eFan(e)) {
            m_edges[fe].m_vertex = nv;
        }
    }
}

void Mesh::computeIndices()
{
    m_indices.resize(m_faceEdges.size() * 3);

    for (unsigned int i = 0; i < m_faceEdges.size(); ++i) {
        mesh_index e0 = m_faceEdges[i], e1 = eNext(e0), e2 = eNext(e1);
        m_indices[i*3 + 0] = eVertex(e0);
        m_indices[i*3 + 1] = eVertex(e1);
        m_indices[i*3 + 2] = eVertex(e2);
    }
}

void Mesh::prepareDrawingData()
{
    computeIndices();
}

void Mesh::reset()
{
    processImportedMesh();
}

QVector3D Mesh::eVector(mesh_index e) const
{
    return vPosition(eEndVertex(e)) - vPosition(eStartVertex(e));
}

unsigned int Mesh::vValency(mesh_index v) const
{
    mesh_index e = vEdge(v);
    return std::distance(eFanBegin(e), eFanEnd(e));
}

bool Mesh::vIsConnected(mesh_index v, mesh_index v1) const
{
    mesh_index edge = vEdge(v);
    return std::any_of(eFanBegin(edge), eFanEnd(edge), [this, v1] (mesh_index e) {
        return eEndVertex(e) == v1;
    });
}

QVector3D Mesh::fNormal(mesh_index f) const
{
    mesh_index e0 = fEdge(f), e1 = eNext(e0), e2 = eNext(e1);

    QVector3D p0 = vPosition(eVertex(e0));
    QVector3D p1 = vPosition(eVertex(e1));
    QVector3D p2 = vPosition(eVertex(e2));

    QVector3D cp = QVector3D::crossProduct(p1 - p0, p2 - p0);
    cp.normalize();

    return cp;
}

bool Mesh::isEdgeCollapsible(mesh_index index) const
{
    mesh_index e0 = index, e1 = eOpposite(e0);
    mesh_index v0 = eVertex(e0), v1 = eVertex(e1);

    unsigned int bc = 0;
    if (vIsBoundary(v0)) ++bc;
    if (vIsBoundary(v1)) ++bc;

    unsigned int vCount = vertexCount();

    if (bc == 0) { // neither v0 nor v1 are boundary
        if (vCount < 4)
            // mesh contains less than 4 vertices: edge is not collapsible!
            return false;

    } else if (bc == 1) { // either v0 or v1 are boundary
        if (vCount < 3)
            // mesh contains less than 3 vertices: edge is not collapsible!
            return false;

    } else { // both v0 and v1 are boundary
        if (!(eIsBoundary(e0) || eIsBoundary(e1)))
            // edge between v0 and v1 is not boundary: edge is not collapsible!
            return false;
    }


    // iterate over all neighbours of v0
    for (mesh_index e : vEdgeFan(v0)) {
        mesh_index v2 = eEndVertex(e); // v2 is a neighbour of v0

        if (vIsConnected(v1, v2)) { // if v2 is also connected to v1, it is a shared neighbour (which we need to check)

            if (vIsBoundary(v2)) { // is v2 a boundary vertex?

                mesh_index ve = vEdge(v2); // since boundary vertices always reference their boundary edge, we can simply do this
                mesh_index endVertex = eEndVertex(ve);

                if (((endVertex == v0) && eIsBoundary(e0)) || ((endVertex == v1) && eIsBoundary(e1)))
                    // if the boundary edge of v2 points to v0 and e0 is also a boundary edge, the vertices v0, v1 and v2 do not form a triangle
                    // the same applies to v1 and e1 analogously

                    // the shared neighbour v2 does not form a triangle with v0 and v1: edge not collapsible!
                    return false;
            }

            if (vValency(v2) <= 3)
                // the valency of the shared neighbour v2 is not greater than 3: edge not collapsible!
                return false;
        }
    }

    // all tests passed: edge is collapsible!
    return true;
}

float Mesh::edgeCollapseCost(mesh_index index) const
{
    mesh_index e0 = index, e1 = eOpposite(e0);
    mesh_index v0 = eVertex(e0), v1 = eVertex(e1);

    /*
    if (vIsBoundary(v1)) {
        return std::numeric_limits<float>::max();
    }
    */

    std::array<QVector3D, 2> edgeFaceNormals;
    if (!eIsBoundary(e0)) edgeFaceNormals[0] = fNormal(eFace(e0));
    if (!eIsBoundary(e1)) edgeFaceNormals[1] = fNormal(eFace(e1));

    // use inverted normal for boundary edges to simulate infinite "sharpness"
    // this should add a cost penalty to boundary edges
    if (eIsBoundary(e0)) edgeFaceNormals[0] = -edgeFaceNormals[1];
    if (eIsBoundary(e1)) edgeFaceNormals[1] = -edgeFaceNormals[0];

    QVector3D p0 = vPosition(v0), p1 = vPosition(v1);
    QVector3D diff = p1 - p0;

    float l = diff.length();

    float maxCost = std::numeric_limits<float>::min();
    for (mesh_index e : eFan(e1)) {
        QVector3D fn;
        if (!eIsBoundary(e)) {
            mesh_index f = eFace(e);
            fn = fNormal(f);
        } else {
            fn = -vNormal(v1); // same as above
        }

        float minCost = std::numeric_limits<float>::max();

        for (QVector3D gn : edgeFaceNormals) {
            float cost = (1.0f - QVector3D::dotProduct(fn, gn)) * 0.5f;
            if (cost < minCost)
                minCost = cost;
        }

        if (minCost > maxCost)
            maxCost = minCost;
    }

    return maxCost * l;
}

std::vector<mesh_index> Mesh::collapseEdge(mesh_index e)
{
    mesh_index e0 = e, e1 = eOpposite(e0);
    mesh_index v0 = eVertex(e0), v1 = eVertex(e1);

    mesh_index ve0 = vEdge(v0), nve = ve0;

    if ((!vIsBoundary(v0)) && vIsBoundary(v1)) {
        nve = vEdge(v1);
    } else if (ve0 == e0) { // reassign vertex edge if it is about to be removed
        if (eIsBoundary(e0)) {
            nve = vEdge(v1);
            if (nve == e1) {
                runVertexTest(v1);
            }
        } else {
            nve = eOpposite(ePrev(e0));
        }
    } else if ((!eIsBoundary(e1)) && (ve0 == eNext(e1))) {
        nve = eNext(eOpposite(ve0));
    }

    m_vertexEdges[v0] = nve;

    for (mesh_index edge : vEdgeFan(v1)) {
        m_edges[edge].m_vertex = v0;
    }

    std::vector<mesh_index> deletedEdges, deletedFaces;

    for (mesh_index edge : {e0, e1}) {
        if (!eIsBoundary(edge)) {
            mesh_index pe = ePrev(edge), ne = eNext(edge);
            mesh_index peo = eOpposite(pe), neo = eOpposite(ne);

            mesh_index pv = eVertex(pe);

            if (vEdge(pv) == pe) {
                m_vertexEdges[pv] = neo;
            }

            // "stick" edges of removed faces together
            m_edges[peo].m_opposite = neo;
            m_edges[neo].m_opposite = peo;

            deletedFaces.push_back(eFace(edge));
            deletedEdges.insert(deletedEdges.end(), {pe, ne});
        }

        deletedEdges.push_back(edge);
    }

    deleteVertex(v1);
    deleteFaces(deletedFaces);
    deleteEdges(deletedEdges);

    //runTests();

    return deletedEdges;
}

mesh_index Mesh::duplicateVertex(mesh_index v)
{
    mesh_index nv = m_vertexEdges.size();

    mesh_index ve = m_vertexEdges[v];
    m_vertexEdges.push_back(ve);

    QVector3D vp = m_vertexPositions[v];
    m_vertexPositions.push_back(vp);

    QVector3D vn = m_vertexNormals[v];
    m_vertexNormals.push_back(vn);

    return nv;
}

void Mesh::deleteVertex(mesh_index v)
{
    mesh_index l = m_vertexEdges.size() - 1;

    if (v != l) {
        m_vertexEdges[v] = m_vertexEdges[l];
        m_vertexPositions[v] = m_vertexPositions[l];
        m_vertexNormals[v] = m_vertexNormals[l];

        for (mesh_index e : vEdgeFan(v)) {
            m_edges[e].m_vertex = v;
        }
    }

    m_vertexEdges.pop_back();
    m_vertexPositions.pop_back();
    m_vertexNormals.pop_back();
}

void Mesh::deleteEdges(std::vector<mesh_index> deletedEdges)
{
    for (unsigned int i = 0; i < deletedEdges.size(); ++i) {
        mesh_index e = deletedEdges[i];
        mesh_index l = m_edges.size() - 1;

        if (e != l) {
            bool d = false;
            for (unsigned int j = i; j < deletedEdges.size(); ++j) {
                if (deletedEdges[j] == l) {
                    deletedEdges[j] = e;
                    d = true;
                }
            }

            if (!d) {
                m_edges[e] = m_edges[l];

                m_edges[eOpposite(e)].m_opposite = e;
                m_edges[eNext(e)].m_previous = e;
                m_edges[ePrev(e)].m_next = e;

                mesh_index f = eFace(e);
                if (m_faceEdges[f] == l)
                    m_faceEdges[f] = e;

                mesh_index v = eVertex(e);
                if (m_vertexEdges[v] == l)
                    m_vertexEdges[v] = e;
            }
        }

        m_edges.pop_back();
    }
}

void Mesh::deleteFaces(std::vector<mesh_index> deletedFaces)
{
    for (unsigned int i = 0; i < deletedFaces.size(); ++i) {
        mesh_index f = deletedFaces[i];
        mesh_index l = m_faceEdges.size() - 1;

        if (f != l) {
            bool d = false;
            for (unsigned int j = i; j < deletedFaces.size(); ++j) {
                if (deletedFaces[j] == l) {
                    deletedFaces[j] = f;
                    d = true;
                }
            }

            if (!d) {
                m_faceEdges[f] = m_faceEdges[l];

                mesh_index e0 = m_faceEdges[f], e1 = eNext(e0), e2 = eNext(e1);
                m_edges[e0].m_face = f;
                m_edges[e1].m_face = f;
                m_edges[e2].m_face = f;
            }
        }

        m_faceEdges.pop_back();
    }
}

void Mesh::runTests()
{
    for (mesh_index v = 0; v < m_vertexEdges.size(); ++v) {
        runVertexTest(v);
    }

    for (mesh_index e = 0; e < m_edges.size(); ++e) {
        runEdgeTest(e);
    }
}

void Mesh::runVertexTest(mesh_index v)
{
    mesh_index e = vEdge(v);

    for (mesh_index f : vEdgeFan(v)) {
        mesh_index fv = eVertex(f);
        if (fv != v) {
            qDebug() << "vertex fan doesn't match:" << v;
            break;
        }
    }

    if (!eIsBoundary(e)) {
        bool wrong = false;
        for (mesh_index fe : eFan(e)) {
            mesh_index feo = eOpposite(fe);
            if (eIsBoundary(feo)) {
                qDebug() << "vertex doesn't reference boundary edge:" << v;
                wrong = true;
            }
        }
        if (wrong) {
            mesh_index ei = e;
            do {
                ei = ePrev(ei);
                ei = eOpposite(ei);

                if (eIsBoundary(ei)) {
                    qDebug() << " - this should be the boundary edge:" << ei;
                    break;
                }
            } while (ei != e);
        }
    }
}

void Mesh::runEdgeTest(mesh_index e)
{
    mesh_index eo = eOpposite(e), eoo = eOpposite(eo);

    if (e != eoo) {
        qDebug() << "opposite doesn't match:" << e;
    }

    if (!eIsBoundary(e)) {
        mesh_index en = eNext(e), enn = eNext(en), ennn = eNext(enn);

        if (e != ennn) {
            qDebug() << "next doesn't match:" << e;
        }

        mesh_index ep = ePrev(e), epp = ePrev(ep), eppp = ePrev(epp);

        if (e != eppp) {
            qDebug() << "previous doesn't match:" << e;
        }
    }
}

void Mesh::decimate(unsigned int targetFaceCount, std::function<bool(float)> progressCallback)
{
    std::vector<bool> edgesCollapsible(m_edges.size());
    std::vector<float> edgeCosts(m_edges.size());

    std::vector<mesh_index> dirtyEdges(m_edges.size());
    std::iota(dirtyEdges.begin(), dirtyEdges.end(), 0); // all edges are dirty to begin with

    unsigned int diff = faceCount() - targetFaceCount;

    while (faceCount() > targetFaceCount) {

        // calculate whether edges can be collapsed and their costs, but only for "dirty" edges
        for (mesh_index e : dirtyEdges) {
            edgesCollapsible[e] = isEdgeCollapsible(e);
            if (edgesCollapsible[e])
                edgeCosts[e] = edgeCollapseCost(e);
            else
                edgeCosts[e] = std::numeric_limits<float>::max();
        }

        dirtyEdges.clear(); // reset dirty edges

        // find collapsible edge with lowest cost
        float minCost = std::numeric_limits<float>::max();
        mesh_index edge = inv_index;
        for (mesh_index e = 0; e < edgeCosts.size(); ++e) {
            if (edgesCollapsible[e] && (edgeCosts[e] < minCost)) {
                minCost = edgeCosts[e];
                edge = e;
            }
        }

        if (!is_valid(edge)) // no more collapsible edges! abort.
            break;

        mesh_index startVertex = eVertex(edge);

        std::vector<mesh_index> deletedEdges = collapseEdge(edge);

        for (unsigned int i = 0; i < deletedEdges.size(); ++i) {
            mesh_index e = deletedEdges[i];
            mesh_index l = edgesCollapsible.size() - 1;

            if (e != l) {
                for (unsigned int j = i; j < deletedEdges.size(); ++j) {
                    if (deletedEdges[j] == l) {
                        deletedEdges[j] = e;
                    }
                }

                edgesCollapsible[e] = edgesCollapsible[l];
                edgeCosts[e] = edgeCosts[l];
            }

            edgesCollapsible.pop_back();
            edgeCosts.pop_back();
        }

        if (vertexCount() < 4) { // if vertex count drops below 4, all edges become dirty
            dirtyEdges.resize(m_edges.size());
            std::iota(dirtyEdges.begin(), dirtyEdges.end(), 0);

        } else { // otherwise, only mark certain edges as dirty

            // iterate edge fan around the start vertex of the previously collapsed edge
            for (mesh_index e : vEdgeFan(startVertex)) {
                dirtyEdges.push_back(e);
                dirtyEdges.push_back(eOpposite(e));

                if (!eIsBoundary(e)) {
                    mesh_index ne = eNext(e);
                    dirtyEdges.push_back(ne);
                    dirtyEdges.push_back(eOpposite(ne));
                }
            }
        }

        unsigned int d = faceCount() - targetFaceCount;
        float p = 1.0f - (float(d) / float(diff));

        if (progressCallback(p)) {
            break;
        }
    }
}

void Mesh::recomputeNormals()
{
    for (mesh_index v = 0; v < m_vertexEdges.size(); ++v) {
        QVector3D normal;

        for (mesh_index e : vEdgeFan(v)) {
            if (eIsBoundary(e))
                continue;

            normal += fNormal(eFace(e));
        }

        normal.normalize();

        m_vertexNormals[v] = normal;
    }
}
