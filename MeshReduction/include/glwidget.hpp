#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtWidgets/QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QBasicTimer>

class Mesh;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT

private:
    Mesh* m_currentMesh;

    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_indexBuf;
    QOpenGLBuffer m_vertexBuf;
    QOpenGLBuffer m_normalBuf;

    bool m_meshInitialized;

    QOpenGLShaderProgram* m_diffProgram;
    QOpenGLShaderProgram* m_unlitProgram;

    int m_mvpLoc, m_worldLoc, m_colorLoc, m_lightPosLoc;
    int m_unlitMvpLoc, m_unlitColorLoc;
    QMatrix4x4 m_world, m_view, m_proj;
    QVector3D m_lightPos;
    QVector4D m_color, m_wireFrameColor;

    QVector3D m_viewCenter;
    float m_viewRotX, m_viewRotY;
    float m_viewDist;

    bool m_drawFaces, m_drawWireframe;

    QPoint m_lastMousePos;

    void calcViewMat();
    void initMesh();
    void cleanupMesh();

public:
	GLWidget(QWidget* parent = 0);
	~GLWidget();

    inline Mesh* currentMesh() const { return m_currentMesh; }
    inline bool drawFaces() const { return m_drawFaces; }
    inline bool drawWireframe() const { return m_drawWireframe; }
    QQuaternion viewRot() const;
    QMatrix4x4 mvpMat() const;

protected:
	void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

public slots:
    void setCurrentMesh(Mesh* mesh);
    void reinitMesh();
    void setDrawFaces(bool v);
    void setDrawWireframe(bool v);
    void resetView();

private slots:
    void cleanupGL();
};

#endif
