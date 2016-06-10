#include "meshfile.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

MeshFile::MeshFile(const QString& fileName) : m_fileName(fileName), m_scene(nullptr)
{
	Assimp::Importer importer;

	m_scene = importer.ReadFile(m_fileName.toLatin1().data(), aiProcessPreset_TargetRealtime_MaxQuality);

	m_errorString = importer.GetErrorString();
}
