#ifndef MESHFILE_H
#define MESHFILE_H

#include <QString>

class aiScene;

class MeshFile
{
private:
	QString m_fileName;
	QString m_errorString;
	const aiScene* m_scene;

public:
	MeshFile(const QString& fileName);

	inline const QString& fileName() const { return m_fileName; }
	inline const QString& errorString() const { return m_errorString; }
	inline bool hasError() const { return !m_scene; }
};

#endif
