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

    int m_mvpLoc, m_worldLoc, m_colorLoc, m_lightPosLoc;
    QMatrix4x4 m_world, m_view, m_proj;
    QVector3D m_lightPos;
    QVector4D m_color;

    QVector3D m_viewCenter;
    float m_viewRotX, m_viewRotY;
    float m_viewDist;

    QPoint m_lastMousePos;

    void calcViewMat();

public:
	GLWidget(QWidget* parent = 0);
	~GLWidget();

    inline Mesh* currentMesh() const { return m_currentMesh; }
    QQuaternion viewRot() const;

protected:
	void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

public slots:
    void setCurrentMesh(Mesh* mesh);
    void resetView();

private slots:
    void cleanupGL();
};

#endif
