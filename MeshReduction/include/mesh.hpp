#ifndef MESH_HPP
#define MESH_HPP

#include "mesh_index.hpp"
#include "mesh_iterators.hpp"

#include <QObject>
#include <QString>
#include <QVector3D>

#include <vector>
#include <algorithm>

#include <functional>


struct HalfEdge
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

class aiMesh;

class Mesh
{
private:
    const aiMesh* m_importedMesh;

    unsigned int m_importedFaceCount;

    // connectivity information (winged half edge data)
    std::vector<HalfEdge> m_edges;
    std::vector<mesh_index> m_faceEdges;
    std::vector<mesh_index> m_vertexEdges; // if vertex is boundary: always boundary edge!

    // drawing data
    std::vector<QVector3D> m_vertexPositions, m_vertexNormals;
    std::vector<unsigned int> m_indices;

    void processImportedMesh();
    void computeIndices();

    bool isEdgeCollapsible(mesh_index index) const;
    float edgeCollapseCost(mesh_index index) const;
    std::vector<mesh_index> collapseEdge(mesh_index e);

    mesh_index duplicateVertex(mesh_index v);

    void deleteVertex(mesh_index v);
    void deleteEdges(std::vector<mesh_index> deletedEdges);
    void deleteFaces(std::vector<mesh_index> deletedFaces);

    void runTests();
    void runVertexTest(mesh_index v);
    void runEdgeTest(mesh_index e);

public:
    Mesh(const aiMesh* mesh);
    ~Mesh();

    QString name() const;


    // edge queries
    mesh_index eVertex(mesh_index e) const { return m_edges[e].m_vertex; }
    mesh_index eStartVertex(mesh_index e) const { return eVertex(e); }
    mesh_index eEndVertex(mesh_index e) const { return eVertex(eOpposite(e)); }

    QVector3D eVector(mesh_index e) const;

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

    edge_fan vEdgeFan(mesh_index v) const { return edge_fan(this, vEdge(v)); }
    edge_fan_iterator vEdgeFanBegin(mesh_index v) const { return eFanBegin(vEdge(v)); }
    edge_fan_iterator vEdgeFanEnd(mesh_index v) const { return eFanEnd(vEdge(v)); }

    unsigned int vValency(mesh_index v) const;

    bool vIsBoundary(mesh_index v) const { return eIsBoundary(vEdge(v)); }
    bool vIsConnected(mesh_index v, mesh_index v1) const;

    const QVector3D& vPosition(mesh_index v) const { return m_vertexPositions[v]; }
    const QVector3D& vNormal(mesh_index v) const { return m_vertexNormals[v]; }


    // face queries
    mesh_index fEdge(mesh_index f) const { return m_faceEdges[f]; }

    QVector3D fNormal(mesh_index f) const;


    void prepareDrawingData();
    void reset();
    void decimate(unsigned int targetFaceCount, std::function<bool(float)> progressCallback);
    void recomputeNormals();


    // drawing data
    unsigned int vertexCount() const;
    unsigned int indexCount() const;
    unsigned int edgeCount() const;
    unsigned int faceCount() const;

    unsigned int importedFaceCount() const;

    unsigned int vertexSize() const;
    unsigned int normalSize() const;

    const QVector3D* vertexData() const;
    const QVector3D* normalData() const;
    const unsigned int* indexData() const;
};

#endif // MESH_HPP
