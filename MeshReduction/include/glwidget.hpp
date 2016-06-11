#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QBasicTimer>

class Mesh;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

private:
    Mesh* m_currentMesh;

    QOpenGLShaderProgram* m_program;

    int m_mvpLoc, m_colorLoc;
    QMatrix4x4 m_world, m_view, m_proj;
    QVector4D m_color;

public:
	GLWidget(QWidget* parent = 0);
	~GLWidget();

    inline Mesh* currentMesh() const { return m_currentMesh; }

protected:
	void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;

public slots:
    void setCurrentMesh(Mesh* mesh);

private slots:
    void cleanupGL();
};

#endif
