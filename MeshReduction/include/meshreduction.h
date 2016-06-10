#ifndef MESHREDUCTION_H
#define MESHREDUCTION_H

#include <memory>

#include <QtWidgets/QMainWindow>
#include "ui_meshreduction.h"

#include "meshfile.h"

class MeshReduction : public QMainWindow
{
	Q_OBJECT

private:
	Ui::MeshReductionClass ui;

	std::unique_ptr<MeshFile> m_currentFile;
	GLWidget* m_glWidget;

	void connectMenus();

public:
	MeshReduction(QWidget *parent = 0);
	~MeshReduction();

signals:
	void currentFileChanged(MeshFile* newFile);

public slots:
	void setCurrentFile(MeshFile* file);

private slots:
	void open();
};

#endif // MESHREDUCTION_H
