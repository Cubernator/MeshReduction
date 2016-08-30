#include "mesh_viewer.hpp"
#include "mesh.hpp"

#include <QSurfaceFormat>
#include <QMessageBox>

#include <QMouseEvent>
#include <QWheelEvent>

#define FOV 45.0f

#define MIN_X_ROT -89.9f
#define MAX_X_ROT 89.9f

#define X_ROT_SPEED 0.5f
#define Y_ROT_SPEED 0.5f

#define X_PAN_SPEED 0.02f
#define Y_PAN_SPEED 0.02f

#define MIN_VIEW_DIST 1.0f
#define MAX_VIEW_DIST 500.0f
#define ZOOM_SPEED 0.001f

MeshViewer::MeshViewer(QWidget* parent) : QOpenGLWidget(parent), m_currentMesh(nullptr),
    m_indexBuf(QOpenGLBuffer::IndexBuffer),
    m_vertexBuf(QOpenGLBuffer::VertexBuffer),
    m_normalBuf(QOpenGLBuffer::VertexBuffer),
    m_meshInitialized(false), m_drawFaces(true), m_drawWireframe(true)
{
    QSurfaceFormat f;
    f.setProfile(QSurfaceFormat::CoreProfile);
    f.setMajorVersion(3);
    f.setMinorVersion(3);
    f.setSamples(4);
    setFormat(f);
}

MeshViewer::~MeshViewer()
{
    cleanupGL();
}

QQuaternion MeshViewer::viewRot() const
{
    return QQuaternion::fromEulerAngles(QVector3D(m_viewRotX, m_viewRotY, 0.0f));
}

QMatrix4x4 MeshViewer::mvpMat() const
{
    return m_proj * m_view * m_world;
}

void MeshViewer::setCurrentMesh(Mesh* mesh)
{
    if (mesh != m_currentMesh) {
        makeCurrent();

        if (m_currentMesh)
            cleanupMesh();

        m_currentMesh = mesh;

        if (m_currentMesh)
            initMesh();

        doneCurrent();

        update();
    }
}

void MeshViewer::reinitMesh()
{
    if (m_currentMesh != nullptr) {
        makeCurrent();

        cleanupMesh();
        initMesh();

        doneCurrent();

        update();
    }
}

void MeshViewer::setDrawFaces(bool v)
{
    m_drawFaces = v;
    update();
}

void MeshViewer::setDrawWireframe(bool v)
{
    m_drawWireframe = v;
    update();
}

void MeshViewer::resetView()
{
    m_viewCenter = QVector3D(0.0f, 0.0f, 0.0f);
    m_viewRotX = 0.0f;
    m_viewRotY = 0.0f;
    m_viewDist = 10.0f;
    calcViewMat();
    update();
}

void MeshViewer::initMesh()
{
    QMutexLocker ml(m_currentMesh->mutex());

    m_currentMesh->prepareDrawingData();

    m_vao.create();
    m_vao.bind();

    unsigned int numVertices = m_currentMesh->vertexCount();

    m_vertexBuf.create();
    m_vertexBuf.bind();
    m_vertexBuf.allocate(m_currentMesh->vertexData(), numVertices * m_currentMesh->vertexSize());
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    m_vertexBuf.release();

    m_normalBuf.create();
    m_normalBuf.bind();
    m_normalBuf.allocate(m_currentMesh->normalData(), numVertices * m_currentMesh->normalSize());
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    m_normalBuf.release();

    m_indexBuf.create();
    m_indexBuf.bind();
    m_indexBuf.allocate(m_currentMesh->indexData(), m_currentMesh->indexCount() * sizeof(unsigned int));

    m_vao.release();
    m_indexBuf.release();

    m_meshInitialized = true;
}

void MeshViewer::initializeGL()
{
    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(cleanupGL()));

	initializeOpenGLFunctions();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    m_diffProgram = new QOpenGLShaderProgram;
    m_diffProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/diffuse.vert");
    m_diffProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/diffuse.frag");

    if (m_diffProgram->link()) {
        m_diffProgram->bind();

        m_mvpLoc = m_diffProgram->uniformLocation("mvp");
        m_worldLoc = m_diffProgram->uniformLocation("world");
        m_colorLoc = m_diffProgram->uniformLocation("color");
        m_lightPosLoc = m_diffProgram->uniformLocation("lightPos");

        m_diffProgram->release();
    } else {
        QMessageBox msg;
        msg.critical(nullptr, "Error in diffuse shader!", m_diffProgram->log());
    }

    m_unlitProgram = new QOpenGLShaderProgram;
    m_unlitProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/unlit.vert");
    m_unlitProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/unlit.frag");

    if (m_unlitProgram->link()) {
        m_unlitProgram->bind();

        m_unlitMvpLoc = m_unlitProgram->uniformLocation("mvp");
        m_unlitColorLoc = m_unlitProgram->uniformLocation("color");

        m_unlitProgram->release();
    } else {
        QMessageBox msg;
        msg.critical(nullptr, "Error in unlit shader!", m_unlitProgram->log());
    }

    if (m_currentMesh && !m_meshInitialized) {
        initMesh();
    }

    m_world.setToIdentity();

    resetView();

    m_color = QVector4D(0.6f, 0.6f, 0.6f, 1.0f);
    m_wireFrameColor = QVector4D(1.0f, 0.6496f, 0.38f, 1.0f);
}

void MeshViewer::calcViewMat()
{
    QVector3D eyePos = m_viewCenter + viewRot() * QVector3D(0.0f, 0.0f, m_viewDist);

    m_lightPos = eyePos;

    m_view.setToIdentity();
    m_view.lookAt(eyePos, m_viewCenter, QVector3D(0.0f, 1.0f, 0.0f));
}

void MeshViewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_currentMesh && m_meshInitialized) {
        QMatrix4x4 mvp = mvpMat();

        m_vao.bind();

        glEnable(GL_DEPTH_TEST);

        if (m_drawFaces) {

            if (m_diffProgram->isLinked()) {
                m_diffProgram->bind();

                m_diffProgram->setUniformValue(m_mvpLoc, mvp);
                m_diffProgram->setUniformValue(m_worldLoc, m_world);
                m_diffProgram->setUniformValue(m_colorLoc, m_color);
                m_diffProgram->setUniformValue(m_lightPosLoc, m_lightPos);

                glEnable(GL_CULL_FACE);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glPolygonOffset(1.0f, 1.0f);

                glDrawElements(GL_TRIANGLES, m_currentMesh->indexCount(), GL_UNSIGNED_INT, 0);

                m_diffProgram->release();
            }
        }

        if (m_drawWireframe) {
            if (m_unlitProgram->isLinked()) {
                m_unlitProgram->bind();

                m_unlitProgram->setUniformValue(m_unlitMvpLoc, mvp);
                m_unlitProgram->setUniformValue(m_unlitColorLoc, m_wireFrameColor);

                glDisable(GL_CULL_FACE);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glPolygonOffset(0.0f, 0.0f);

                glDrawElements(GL_TRIANGLES, m_currentMesh->indexCount(), GL_UNSIGNED_INT, 0);

                m_unlitProgram->release();
            }
        }

        m_vao.release();
    }
}

void MeshViewer::resizeGL(int width, int height)
{
    m_proj.setToIdentity();
    m_proj.perspective(FOV, float(width) / float(height), 0.1f, 1000.0f);
}

void MeshViewer::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    event->accept();
}

void MeshViewer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    int deltaX = pos.x() - m_lastMousePos.x();
    int deltaY = pos.y() - m_lastMousePos.y();

    Qt::MouseButtons b = event->buttons();
    if (b & Qt::RightButton) {
        QVector3D d(-deltaX, deltaY, 0.0f);
        float s = tanf(FOV * 0.5f) * 2.0f * m_viewDist / float(height());
        m_viewCenter += viewRot() * (d * s);
        calcViewMat();
        update();

    } else if (b & Qt::LeftButton) {
        m_viewRotY -= deltaX * Y_ROT_SPEED;
        m_viewRotX -= deltaY * X_ROT_SPEED;
        if (m_viewRotX > MAX_X_ROT) m_viewRotX = MAX_X_ROT;
        if (m_viewRotX < MIN_X_ROT) m_viewRotX = MIN_X_ROT;
        calcViewMat();
        update();
    }

    m_lastMousePos = event->pos();
    event->accept();
}

void MeshViewer::wheelEvent(QWheelEvent *event)
{
    m_viewDist += m_viewDist * -event->angleDelta().y() * ZOOM_SPEED;
    if (m_viewDist < MIN_VIEW_DIST) m_viewDist = MIN_VIEW_DIST;
    if (m_viewDist > MAX_VIEW_DIST) m_viewDist = MAX_VIEW_DIST;
    calcViewMat();
    update();
    event->accept();
}

void MeshViewer::cleanupMesh()
{
    m_meshInitialized = false;

    m_indexBuf.destroy();
    m_vertexBuf.destroy();
    m_normalBuf.destroy();
    m_vao.destroy();
}

void MeshViewer::cleanupGL()
{
    makeCurrent();

    delete m_diffProgram;
    delete m_unlitProgram;

    if (m_meshInitialized) {
        cleanupMesh();
    }

    doneCurrent();
}
