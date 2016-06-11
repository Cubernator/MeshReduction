#include "scenefile.hpp"

#include "mesh.hpp"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

SceneFile::SceneFile(const QString& fileName) : m_fileName(fileName)
{
    m_importer.ReadFile(m_fileName.toLatin1().data(), aiProcessPreset_TargetRealtime_MaxQuality);

    const aiScene* scene = m_importer.GetScene();

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        m_meshes.emplace_back(new Mesh(scene->mMeshes[i]));
    }
}
