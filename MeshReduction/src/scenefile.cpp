#include "scenefile.hpp"

#include "mesh.hpp"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

#include <assimp/Exporter.hpp>

SceneFile::SceneFile(const QString& fileName) : m_fileName(fileName)
{
    unsigned int pFlags = aiProcess_GenNormals
            | aiProcess_JoinIdenticalVertices
            | aiProcess_Triangulate
            | aiProcess_ValidateDataStructure
            | aiProcess_SortByPType
            | aiProcess_FindInvalidData
            | aiProcess_FindDegenerates
            | aiProcess_GenUVCoords;

    m_importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    m_importedScene = m_importer.ReadFile(m_fileName.toLatin1().data(), pFlags);

    if (m_importedScene) {
        for (unsigned int i = 0; i < m_importedScene->mNumMeshes; ++i) {
            m_meshes.emplace_back(new Mesh(m_importedScene->mMeshes[i]));
        }
    }
}

void SceneFile::exportToFile(const QString &fileName, const QString &formatId, const std::vector<bool> &includedMeshMask) const
{
    aiScene exportedScene;

    std::vector<aiMesh*> exportedMeshes;

    for (unsigned int i = 0; i < numMeshes(); ++i) {
        if (includedMeshMask[i]) {
            exportedMeshes.push_back(getMesh(i)->makeExportMesh());
        }
    }

    unsigned int nm = exportedMeshes.size();

    exportedScene.mRootNode = new aiNode();
    exportedScene.mRootNode->mNumMeshes = nm;
    exportedScene.mRootNode->mMeshes = new unsigned int[nm];

    exportedScene.mNumMeshes = nm;
    exportedScene.mMeshes = new aiMesh*[nm];

    for (unsigned int i = 0; i < nm; ++i) {
        exportedScene.mMeshes[i] = exportedMeshes[i];
        exportedScene.mRootNode->mMeshes[i] = i;
    }

    unsigned int nmats = m_importedScene->mNumMaterials;

    exportedScene.mNumMaterials = nmats;
    exportedScene.mMaterials = new aiMaterial*[nmats];
    for (unsigned int i = 0; i < nmats; ++i) {
        exportedScene.mMaterials[i] = new aiMaterial();
        aiMaterial::CopyPropertyList(exportedScene.mMaterials[i], m_importedScene->mMaterials[i]);
    }

    Assimp::Exporter exporter;

    exporter.Export(&exportedScene, formatId.toStdString(), fileName.toStdString());
}

QString SceneFile::getImportExtensions()
{
    Assimp::Importer dummyImporter;
    aiString ext;
    dummyImporter.GetExtensionList(ext);
    QString extensions(ext.C_Str());
    return extensions.replace(";", " ");
}

QString SceneFile::getExportExtensions()
{
    Assimp::Exporter dummyExporter;
    QString result;
    unsigned int fn = dummyExporter.GetExportFormatCount();
    for (unsigned int i = 0; i < fn; ++i) {
        const aiExportFormatDesc* desc = dummyExporter.GetExportFormatDescription(i);
        if (i > 0) result.append(' ');
        result.append(desc->fileExtension);
    }

    return result;
}

std::vector<ExportFormat> SceneFile::getExportFormats()
{
    std::vector<ExportFormat> result;

    Assimp::Exporter dummyExporter;
    unsigned int fn = dummyExporter.GetExportFormatCount();

    for (unsigned int i = 0; i < fn; ++i) {
        const aiExportFormatDesc* desc = dummyExporter.GetExportFormatDescription(i);
        result.push_back({QString(desc->id), QString(desc->fileExtension), QString(desc->description)});
    }

    return result;
}
