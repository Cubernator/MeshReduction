#ifndef MESH_HPP
#define MESH_HPP

#include <QString>
#include <assimp/mesh.h>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>

#include <vector>

class Mesh
{
private:
    const aiMesh* m_importedMesh;

    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vertexBuf, m_indexBuf;

    std::vector<unsigned int> m_indices;

    bool m_initialized;

    void computeIndices();

public:
    Mesh(const aiMesh* mesh);
    ~Mesh();

    inline QString name() const { return m_importedMesh->mName.C_Str(); }

    inline bool initialized() const { return m_initialized; }

    inline const QOpenGLVertexArrayObject& vao() const { return m_vao; }
    inline const QOpenGLBuffer& vertexBuf() const { return m_vertexBuf; }
    inline const QOpenGLBuffer& indexBuf() const { return m_indexBuf; }

    void initGL();
    void cleanupGL();

    void draw(QOpenGLFunctions* f);
};

#endif // MESH_HPP
