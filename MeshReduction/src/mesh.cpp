#include "mesh.hpp"

#include <QOpenGLFunctions>
#include <QtDebug>
#include <QMatrix4x4>
#include <assimp/mesh.h>

#include <assert.h>

#include <limits>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <set>
#include <unordered_set>

Mesh::Mesh(const aiMesh *mesh) : m_importedMesh(mesh)
{
    processImportedMesh();
}

Mesh::~Mesh() { }

QString Mesh::name() const { return m_importedMesh->mName.C_Str(); }

bool Mesh::isDirty() const
{
    return faceCount() != importedFaceCount();
}

void Mesh::processImportedMesh()
{
    m_indices.clear();

    // copy raw vertex data
    unsigned int vCount = m_importedMesh->mNumVertices;
    glm::vec3* vData = reinterpret_cast<glm::vec3*>(m_importedMesh->mVertices);
    glm::vec3* nData = reinterpret_cast<glm::vec3*>(m_importedMesh->mNormals);
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

    m_vertexCount = m_vertexEdges.size();

    m_importedFaceCount = faceCount();
    m_importedHalfedgeCount = halfedgeCount();
    m_importedVertexCount = vertexCount();
}

aiMesh *Mesh::makeExportMesh() const
{
    aiMesh* mesh = new aiMesh();

    mesh->mName = m_importedMesh->mName;
    mesh->mMaterialIndex = m_importedMesh->mMaterialIndex;

    unsigned int vc = vertexCount();

    mesh->mNumVertices = vc;

    mesh->mVertices = new aiVector3D[vc];
    std::memcpy(mesh->mVertices, vertexData(), vc * vertexSize());

    mesh->mNormals = new aiVector3D[vc];
    std::memcpy(mesh->mNormals, normalData(), vc * normalSize());

    unsigned int fc = faceCount();

    mesh->mNumFaces = fc;
    mesh->mFaces = new aiFace[fc];

    for (mesh_index f = 0; f < fc; ++f) {
        mesh_index e0 = fEdge(f), e1 = eNext(e0), e2 = eNext(e1);

        aiFace& face = mesh->mFaces[f];

        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];

        face.mIndices[0] = eVertex(e0);
        face.mIndices[1] = eVertex(e1);
        face.mIndices[2] = eVertex(e2);
    }

    return mesh;
}

void Mesh::computeIndices()
{
    m_indices.clear();
    m_indices.reserve(m_faceEdges.size() * 3);

    for (unsigned int i = 0; i < m_faceEdges.size(); ++i) {
        mesh_index e0 = m_faceEdges[i];
        if (!is_valid(e0)) continue;

        mesh_index e1 = eNext(e0), e2 = eNext(e1);
        m_indices.push_back(eVertex(e0));
        m_indices.push_back(eVertex(e1));
        m_indices.push_back(eVertex(e2));
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

glm::vec3 Mesh::eVector(mesh_index e) const
{
    return vPosition(eEndVertex(e)) - vPosition(eStartVertex(e));
}

glm::vec3 Mesh::eDirection(mesh_index e) const
{
    return glm::normalize(eVector(e));
}

mesh_index Mesh::vConnectingEdge(mesh_index v, mesh_index v1) const
{
    for (mesh_index e : vEdgeFan(v)) {
        if (eEndVertex(e) == v1) {
            return e;
        }
    }

    return inv_index;
}

unsigned int Mesh::vValency(mesh_index v) const
{
    mesh_index e = vEdge(v);
    return std::distance(eFanBegin(e), eFanEnd(e));
}

glm::vec3 Mesh::fNormal(mesh_index f) const
{
    mesh_index e0 = fEdge(f), e1 = eNext(e0), e2 = eNext(e1);
    return triangleNormal(eStartPos(e0), eStartPos(e1), eStartPos(e2));
}

float Mesh::fArea(mesh_index f) const
{
    mesh_index e0 = fEdge(f), e1 = eNext(e0), e2 = eNext(e1);
    return triangleArea(eStartPos(e0), eStartPos(e1), eStartPos(e2));
}

bool Mesh::isPairContractable(mesh_index v0, mesh_index v1, const glm::vec3& newPos) const
{
    mesh_index e0 = vConnectingEdge(v0, v1);
    if (!is_valid(e0)) {
        return false;
    }

    mesh_index e0n = eNext(e0);

    mesh_index e1 = eOpposite(e0);
    mesh_index e1n = eNext(e1);

    // phase 1: topological tests

    unsigned int bc = 0;
    if (vIsBoundary(v0)) ++bc;
    if (vIsBoundary(v1)) ++bc;

    unsigned int vCount = m_vertexCount;

    if (bc == 0) { // neither v0 nor v1 are boundary
        if (vCount <= 4)
            // mesh contains no more than 4 vertices: edge is not collapsible!
            return false;

    } else if (bc == 1) { // either v0 or v1 are boundary
        if (vCount <= 3)
            // mesh contains no more than 3 vertices: edge is not collapsible!
            return false;

    } else { // both v0 and v1 are boundary
        if (!(eIsBoundary(e0) || eIsBoundary(e1)))
            // edge between v0 and v1 is not boundary: edge is not collapsible!
            return false;
    }

    unsigned int val0 = vValency(v0), val1 = vValency(v1);
    if (val0 <= 3 && val1 <= 3) {
        // this test prevents loose parts of the mesh from forming degenerate triangles
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

    // phase 2: geometric tests

    for (mesh_index v : {v0, v1}) {
        for (mesh_index e : vEdgeFan(v)) {
            if (!(eIsBoundary(e) || (e == e0) || (e == e1) || (e == e0n) || (e == e1n))) {
                mesh_index f = eFace(e);
                glm::vec3 oldNormal = fNormal(f);

                mesh_index en = eNext(e), enn = eNext(en);
                glm::vec3 newNormal = triangleNormal(newPos, eStartPos(en), eStartPos(enn));

                if (glm::dot(oldNormal, newNormal) < 0.0f)
                    // pair contraction causes adjacent face to flip: edge not collapsible!
                    return false;
            }
        }
    }

    // all tests passed: edge is collapsible!
    return true;
}

unsigned int Mesh::collapseEdge(mesh_index e, const glm::vec3& newPos)
{
    mesh_index e0 = e, e1 = eOpposite(e0);
    mesh_index v0 = eVertex(e0), v1 = eVertex(e1);

    mesh_index ve0 = vEdge(v0), nve = ve0;

    // reassign vertex edge if it is about to be removed
    if ((!vIsBoundary(v0)) && vIsBoundary(v1)) {
        nve = vEdge(v1);
    } else if (ve0 == e0) {
        if (eIsBoundary(e0)) {
            nve = vEdge(v1);
        } else {
            nve = eOpposite(ePrev(e0));
        }
    } else if ((!eIsBoundary(e1)) && (ve0 == eNext(e1))) {
        nve = eNext(eOpposite(ve0));
    }

    m_vertexEdges[v0] = nve;
    m_vertexPositions[v0] = newPos;

    for (mesh_index edge : vEdgeFan(v1)) {
        m_edges[edge].m_vertex = v0;
    }

    unsigned int dfc = 0;

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


            // invalidate removed primitives to mark them for deletion
            m_faceEdges[eFace(edge)] = inv_index;

            m_edges[pe] = inv_edge;
            m_edges[ne] = inv_edge;
        }

        ++dfc;

        m_edges[edge] = inv_edge;
    }

    m_vertexEdges[v1] = inv_index;
    --m_vertexCount;

#if defined(_DEBUG)
    runVertexTest(v0);
#endif

    return dfc;
}

mesh_index Mesh::duplicateVertex(mesh_index v)
{
    mesh_index nv = m_vertexEdges.size();

    mesh_index ve = m_vertexEdges[v];
    m_vertexEdges.push_back(ve);

    glm::vec3 vp = m_vertexPositions[v];
    m_vertexPositions.push_back(vp);

    glm::vec3 vn = m_vertexNormals[v];
    m_vertexNormals.push_back(vn);

    return nv;
}

void Mesh::cleanupData()
{
    cleanupPrimitives(m_faceEdges, [this] (mesh_index f, mesh_index l) {
        m_faceEdges[f] = m_faceEdges[l];
        mesh_index e0 = m_faceEdges[f], e1 = eNext(e0), e2 = eNext(e1);
        m_edges[e0].m_face = f;
        m_edges[e1].m_face = f;
        m_edges[e2].m_face = f;
    });

    cleanupPrimitives(m_edges, [this] (mesh_index e, mesh_index l) {
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
    });

    cleanupPrimitives(m_vertexEdges, [this] (mesh_index v, mesh_index l) {
        m_vertexEdges[v] = m_vertexEdges[l];
        m_vertexPositions[v] = m_vertexPositions[l];
        m_vertexNormals[v] = m_vertexNormals[l];

        for (mesh_index e : vEdgeFan(v)) {
            m_edges[e].m_vertex = v;
        }
    });

    m_vertexPositions.resize(m_vertexEdges.size());
    m_vertexNormals.resize(m_vertexEdges.size());

    assert(m_vertexCount == m_vertexEdges.size());
}

void Mesh::recomputeNormals()
{
    for (mesh_index v = 0; v < m_vertexEdges.size(); ++v) {
        glm::vec3 normal;

        for (mesh_index e : vEdgeFan(v)) {
            if (eIsBoundary(e))
                continue;

            normal += fNormal(eFace(e));
        }

        m_vertexNormals[v] = glm::normalize(normal);
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
    if (!vIsBoundary(v)) {
        if (vValency(v) < 3) {
            qDebug() << "anomaly detected!";
        }
    }

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
