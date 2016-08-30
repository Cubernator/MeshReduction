#ifndef MESHFILE_H
#define MESHFILE_H

#include <QString>
#include <assimp\Importer.hpp>

#include <vector>
#include <memory>

struct ExportFormat
{
    QString m_id;
    QString m_extension;
    QString m_desc;
};

class Mesh;

class SceneFile
{
private:
	QString m_fileName;
    Assimp::Importer m_importer;
    const aiScene* m_importedScene;

    std::vector<std::unique_ptr<Mesh>> m_meshes;

public:
    SceneFile(const QString& fileName);

	inline const QString& fileName() const { return m_fileName; }
    inline QString errorString() const { return QString(m_importer.GetErrorString()); }
    inline bool hasError() const { return !m_importer.GetScene(); }

    inline unsigned int numMeshes() const { return m_meshes.size(); }
    inline Mesh* getMesh(unsigned int index) { return m_meshes[index].get(); }
    inline const Mesh* getMesh(unsigned int index) const { return m_meshes[index].get(); }

    QString exportToFile(const QString& fileName, const QString& formatId, const std::vector<bool>& includedMeshMask) const;

    static QString getImportExtensions();
    static QString getExportExtensions();

    static std::vector<ExportFormat> getExportFormats();
};

#endif
