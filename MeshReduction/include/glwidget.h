#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>

class MeshFile;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

private:
	MeshFile* m_currentFile;

public:
	GLWidget(QWidget* parent = 0);
	~GLWidget();

	inline MeshFile* currentFile() const { return m_currentFile; }

protected:
	void initializeGL() Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;

public slots:
	void setCurrentFile(MeshFile* file);
};

#endif