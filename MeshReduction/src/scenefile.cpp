#include "scenefile.hpp"

#include "mesh.hpp"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

SceneFile::SceneFile(const QString& fileName) : m_fileName(fileName)
{
    unsigned int pFlags = aiProcess_Triangulate
            | aiProcess_GenNormals
            | aiProcess_ValidateDataStructure
            | aiProcess_SortByPType
            | aiProcess_FindInvalidData
            | aiProcess_GenUVCoords;

    const aiScene* scene = m_importer.ReadFile(m_fileName.toLatin1().data(), pFlags);

    if (scene) {
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            m_meshes.emplace_back(new Mesh(scene->mMeshes[i]));
        }
    }
}

QString SceneFile::getImportExtensions()
{
    Assimp::Importer dummyImporter;
    aiString ext;
    dummyImporter.GetExtensionList(ext);
    QString extensions(ext.C_Str());
    return extensions.replace(";", " ");
}
