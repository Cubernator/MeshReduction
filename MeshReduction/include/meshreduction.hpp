#ifndef MESHREDUCTION_H
#define MESHREDUCTION_H

#include <memory>

#include <QtWidgets/QMainWindow>
#include <QStringListModel>
#include "ui_meshreduction.h"

class SceneFile;
class Mesh;

class MeshReduction : public QMainWindow
{
	Q_OBJECT

private:
	Ui::MeshReductionClass ui;

	std::unique_ptr<SceneFile> m_currentFile;
    Mesh* m_selectedMesh;
	GLWidget* m_glWidget;

    void populateMeshList();

public:
	MeshReduction(QWidget *parent = 0);
	~MeshReduction();

    inline SceneFile* currentFile() const { return m_currentFile.get(); }
    inline Mesh* selectedMesh() const { return m_selectedMesh; }

signals:
	void currentFileChanged(SceneFile* newFile);
    void selectedMeshChanged(Mesh* mesh);

public slots:
	void setCurrentFile(SceneFile* file);
    void selectMesh(Mesh* mesh);

private slots:
	void open();
    void handleMeshSelection(int index);
};

#endif // MESHREDUCTION_H
