#include "meshreduction.hpp"

#include "scenefile.hpp"
#include "mesh.hpp"

#include <QtWidgets\QFileDialog>
#include <QtWidgets\QMessageBox>

MeshReduction::MeshReduction(QWidget *parent) : QMainWindow(parent)
{
	ui.setupUi(this);

	m_glWidget = ui.openGLWidget;

    connect(ui.meshList, SIGNAL(currentRowChanged(int)), this, SLOT(handleMeshSelection(int)));

    connect(this, SIGNAL(selectedMeshChanged(Mesh*)), m_glWidget, SLOT(setCurrentMesh(Mesh*)));

    connect(ui.actionOpen, SIGNAL(triggered(bool)), this, SLOT(open()));
    connect(ui.actionExit, SIGNAL(triggered(bool)), this, SLOT(close()));
}

MeshReduction::~MeshReduction()
{
	
}

void MeshReduction::setCurrentFile(SceneFile* file)
{
	if (file != m_currentFile.get()) {
		m_currentFile.reset(file);
		emit currentFileChanged(file);

        populateMeshList();

        if (file && file->numMeshes() > 0) {
            ui.meshList->setCurrentRow(0);
        }
    }
}

void MeshReduction::handleMeshSelection(int index)
{
    if (index >= 0 && (unsigned int)index < m_currentFile->numMeshes()) {
        selectMesh(m_currentFile->getMesh(index));
    } else {
        selectMesh(nullptr);
    }
}

void MeshReduction::selectMesh(Mesh *mesh)
{
    if (mesh != m_selectedMesh) {
        m_selectedMesh = mesh;
        emit selectedMeshChanged(mesh);
    }
}

void MeshReduction::populateMeshList()
{
    ui.meshList->clear();
    if (m_currentFile) {
        for (unsigned int i = 0; i < m_currentFile->numMeshes(); ++i) {
            ui.meshList->addItem(m_currentFile->getMesh(i)->name());
        }
    }
}

void MeshReduction::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));

	if (!fileName.isNull()) {
        SceneFile* newFile = new SceneFile(fileName);

		if (newFile->hasError()) {
			QMessageBox msgBox;
            msgBox.setText(tr("Failed to open file!"));
			msgBox.setInformativeText(newFile->errorString());
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setIcon(QMessageBox::Icon::Critical);
			msgBox.exec();
		} else {
            statusBar()->showMessage(tr("Opened file") + ": \"" + newFile->fileName() + "\"");
			setCurrentFile(newFile);
		}
	}
}
