#include "meshreduction.h"

#include <QtWidgets\QFileDialog>
#include <QtWidgets\QMessageBox>

MeshReduction::MeshReduction(QWidget *parent) : QMainWindow(parent)
{
	ui.setupUi(this);

	m_glWidget = ui.openGLWidget;

	connect(this, &MeshReduction::currentFileChanged, m_glWidget, &GLWidget::setCurrentFile);

	connectMenus();
}

MeshReduction::~MeshReduction()
{
	
}

void MeshReduction::setCurrentFile(MeshFile* file)
{
	if (file != m_currentFile.get()) {
		m_currentFile.reset(file);
		emit currentFileChanged(file);
	}
}

void MeshReduction::connectMenus()
{
	connect(ui.actionOpen, &QAction::triggered, this, &MeshReduction::open);
	connect(ui.actionExit, &QAction::triggered, this, &MeshReduction::close);
}

void MeshReduction::open()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Model File"));

	if (!fileName.isNull()) {
		MeshFile* newFile = new MeshFile(fileName);

		if (newFile->hasError()) {
			QMessageBox msgBox;
			msgBox.setText("Failed to open file!");
			msgBox.setInformativeText(newFile->errorString());
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setIcon(QMessageBox::Icon::Critical);
			msgBox.exec();
		} else {
			setCurrentFile(newFile);
		}
	}
}
