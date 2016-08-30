#ifndef MESH_HPP
#define MESH_HPP

#include "mesh_index.hpp"
#include "mesh_iterators.hpp"
#include "util.hpp"

#include <QString>
#include <QMutex>

#include <vector>
#include <algorithm>

#include <functional>


struct Halfedge
{
    /*!
     * \brief index of the vertex this halfedge is pointing away from. always valid.
     */
    mesh_index m_vertex;

    /*!
     * \brief index of the face this halfedge is bordering to the left. if the halfedge is boundary, this index is invalid.
     */
    mesh_index m_face;

    /*!
     * \brief index of the opposite halfedge. always valid.
     */
    mesh_index m_opposite;

    /*!
     * \brief index of the next halfedge in the bordering face (counter-clockwise). if the halfedge is boundary, this index is invalid.
     */
    mesh_index m_next;

    /*!
     * \brief index of the previous halfedge in the bordering face (clockwise). if the halfedge is boundary, this index is invalid.
     */
    mesh_index m_previous;
};

const Halfedge inv_edge ({ inv_index, inv_index, inv_index, inv_index, inv_index });

inline bool is_valid(const Halfedge& edge)
{
    return is_valid(edge.m_vertex) && is_valid(edge.m_face) && is_valid(edge.m_opposite);
}

class aiMesh;

class Mesh
{
private:
    const aiMesh* m_importedMesh;

    mutable QMutex m_mutex;

    unsigned int m_importedFaceCount, m_importedHalfedgeCount, m_importedVertexCount;
    unsigned int m_vertexCount;

    // connectivity information (winged half edge data)
    std::vector<Halfedge> m_edges;
    std::vector<mesh_index> m_faceEdges;
    std::vector<mesh_index> m_vertexEdges; // if vertex is boundary: always boundary edge!

    // drawing data
    std::vector<glm::vec3> m_vertexPositions, m_vertexNormals;
    std::vector<unsigned int> m_indices;

    void processImportedMesh();
    void computeIndices();

    mesh_index duplicateVertex(mesh_index v);

    template<typename T>
    void cleanupPrimitives(T& container, std::function<void(mesh_index, mesh_index)> replaceFun) {
        auto begin = container.begin();
        auto first = begin, last = container.end();
        first = std::find_if(first, last, [] (const typename T::value_type& elem) { return !is_valid(elem); });

        if (first != last) {
            for (auto it = first; ++it != last; ) {
                if (is_valid(*it)) {
                    replaceFun(first - begin, it - begin);
                    ++first;
                }
            }
        }

        container.erase(first, last);
    }

    void runTests();
    void runVertexTest(mesh_index v);
    void runEdgeTest(mesh_index e);

public:
    Mesh(const aiMesh* mesh);
    ~Mesh();

    QString name() const;
    const aiMesh* importedMesh() const;

    QMutex* mutex() const { return &m_mutex; }

    bool isDirty() const;


    // edge queries
    mesh_index eVertex(mesh_index e) const { return m_edges[e].m_vertex; }
    mesh_index eStartVertex(mesh_index e) const { return eVertex(e); }
    mesh_index eEndVertex(mesh_index e) const { return eVertex(eOpposite(e)); }

    const glm::vec3& eStartPos(mesh_index e) const { return vPosition(eStartVertex(e)); }
    const glm::vec3& eEndPos(mesh_index e) const { return vPosition(eEndVertex(e)); }

    glm::vec3 eVector(mesh_index e) const;
    glm::vec3 eDirection(mesh_index e) const;

    mesh_index eFace(mesh_index e) const { return m_edges[e].m_face; }

    mesh_index eOpposite(mesh_index e) const { return m_edges[e].m_opposite; }
    mesh_index eNext(mesh_index e) const { return m_edges[e].m_next; }
    mesh_index ePrev(mesh_index e) const { return m_edges[e].m_previous; }

    bool eIsBoundary(mesh_index e) const { return !is_valid(m_edges[e].m_face); }

    edge_fan eFan(mesh_index e) const { return edge_fan(this, e); }
    edge_fan_iterator eFanBegin(mesh_index e) const { return edge_fan_iterator(this, e, e); }
    edge_fan_iterator eFanEnd(mesh_index e) const { return edge_fan_iterator(this, e, inv_index); }


    // vertex queries
    mesh_index vEdge(mesh_index v) const { return m_vertexEdges[v]; }
    mesh_index vConnectingEdge(mesh_index v, mesh_index v1) const;

    edge_fan vEdgeFan(mesh_index v) const { return edge_fan(this, vEdge(v)); }
    edge_fan_iterator vEdgeFanBegin(mesh_index v) const { return eFanBegin(vEdge(v)); }
    edge_fan_iterator vEdgeFanEnd(mesh_index v) const { return eFanEnd(vEdge(v)); }

    unsigned int vValency(mesh_index v) const;

    bool vIsBoundary(mesh_index v) const { return eIsBoundary(vEdge(v)); }
    bool vIsConnected(mesh_index v, mesh_index v1) const { return is_valid(vConnectingEdge(v, v1)); }

    const glm::vec3& vPosition(mesh_index v) const { return m_vertexPositions[v]; }
    const glm::vec3& vNormal(mesh_index v) const { return m_vertexNormals[v]; }


    // face queries
    mesh_index fEdge(mesh_index f) const { return m_faceEdges[f]; }

    glm::vec3 fNormal(mesh_index f) const;
    float fArea(mesh_index f) const;


    void prepareDrawingData();
    void reset();
    void recomputeNormals();
    void cleanupData();

    bool isPairContractable(mesh_index v0, mesh_index v1, const glm::vec3& newPos) const;
    unsigned int collapseEdge(mesh_index e, const glm::vec3& newPos);


    unsigned int vertexCount() const { return m_vertexEdges.size(); }
    unsigned int indexCount() const { return m_indices.size(); }
    unsigned int halfedgeCount() const { return m_edges.size(); }
    unsigned int edgeCount() const { return halfedgeCount() / 2; }
    unsigned int faceCount() const { return m_faceEdges.size(); }

    unsigned int importedFaceCount() const { return m_importedFaceCount; }
    unsigned int importedEdgeCount() const { return m_importedHalfedgeCount / 2; }
    unsigned int importedVertexCount() const { return m_importedVertexCount; }

    unsigned int vertexSize() const { return sizeof(glm::vec3); }
    unsigned int normalSize() const { return sizeof(glm::vec3); }

    const glm::vec3* vertexData() const { return m_vertexPositions.data(); }
    const glm::vec3* normalData() const { return m_vertexNormals.data(); }
    const unsigned int* indexData() const { return m_indices.data(); }


    aiMesh* makeExportMesh() const;
};

#endif // MESH_HPP
