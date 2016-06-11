#include "mesh.hpp"
#include <QOpenGLFunctions>

Mesh::Mesh(const aiMesh *mesh) : m_importedMesh(mesh),
    m_vertexBuf(QOpenGLBuffer::VertexBuffer),
    m_indexBuf(QOpenGLBuffer::IndexBuffer),
    m_initialized(false)
{
    computeIndices();
}

Mesh::~Mesh()
{
    cleanupGL();
}

void Mesh::initGL()
{
    m_vao.create();
    m_vao.bind();

    m_vertexBuf.create();
    m_vertexBuf.bind();
    m_vertexBuf.allocate(m_importedMesh->mVertices, m_importedMesh->mNumVertices * sizeof(aiVector3D));

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);

    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    m_vertexBuf.release();
    m_vao.release();

    m_indexBuf.create();
    m_indexBuf.bind();
    m_indexBuf.allocate(m_indices.data(), m_indices.size() * sizeof(unsigned int));
    m_indexBuf.release();

    m_initialized = true;
}

void Mesh::cleanupGL()
{
    m_indexBuf.destroy();
    m_vertexBuf.destroy();
    m_vao.destroy();

    m_initialized = false;
}

void Mesh::draw(QOpenGLFunctions *f)
{
    m_vao.bind();
    m_indexBuf.bind();

    f->glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

    m_indexBuf.release();
    m_vao.release();
}

void Mesh::computeIndices()
{
    m_indices.clear();

    for (unsigned int i = 0; i < m_importedMesh->mNumFaces; ++i) {
        const aiFace& face = m_importedMesh->mFaces[i];
        if (face.mNumIndices != 3) continue;
        m_indices.push_back(face.mIndices[0]);
        m_indices.push_back(face.mIndices[1]);
        m_indices.push_back(face.mIndices[2]);
    }
}
