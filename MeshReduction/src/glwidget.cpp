#include "glwidget.hpp"
#include "mesh.hpp"

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

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent), m_currentMesh(nullptr)
{

}

GLWidget::~GLWidget()
{
    cleanupGL();
}

QQuaternion GLWidget::viewRot() const
{
    return QQuaternion::fromEulerAngles(QVector3D(m_viewRotX, m_viewRotY, 0.0f));
}

void GLWidget::setCurrentMesh(Mesh* mesh)
{
    if (mesh != m_currentMesh) {
        makeCurrent();

        if (m_currentMesh)
            m_currentMesh->cleanupGL(this);

        m_currentMesh = mesh;

        if (m_currentMesh)
            m_currentMesh->initGL(this);

        doneCurrent();

        update();
    }
}

void GLWidget::resetView()
{
    m_viewCenter = QVector3D(0.0f, 0.0f, 0.0f);
    m_viewRotX = 0.0f;
    m_viewRotY = 0.0f;
    m_viewDist = 10.0f;
    calcViewMat();
}

void GLWidget::initializeGL()
{
    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(cleanupGL()));

	initializeOpenGLFunctions();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/default.vert");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/default.frag");
    m_program->link();

    m_program->bind();

    m_mvpLoc = m_program->uniformLocation("mvp");
    m_worldLoc = m_program->uniformLocation("world");
    m_colorLoc = m_program->uniformLocation("color");
    m_lightPosLoc = m_program->uniformLocation("lightPos");

    m_program->release();

    if (m_currentMesh && !m_currentMesh->initialized()) {
        m_currentMesh->initGL(this);
    }

    m_world.setToIdentity();

    resetView();

    m_color = QVector4D(0.6f, 0.6f, 0.6f, 1.0f);
}

void GLWidget::calcViewMat()
{
    QVector3D eyePos = m_viewCenter + viewRot() * QVector3D(0.0f, 0.0f, m_viewDist);

    m_lightPos = eyePos;

    m_view.setToIdentity();
    m_view.lookAt(eyePos, m_viewCenter, QVector3D(0.0f, 1.0f, 0.0f));
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_program->bind();

    m_program->setUniformValue(m_mvpLoc, m_proj * m_view * m_world);
    m_program->setUniformValue(m_worldLoc, m_world);
    m_program->setUniformValue(m_colorLoc, m_color);
    m_program->setUniformValue(m_lightPosLoc, m_lightPos);

    if (m_currentMesh && m_currentMesh->initialized()) {
        m_currentMesh->draw(this);
    }

    m_program->release();
}

void GLWidget::resizeGL(int width, int height)
{
    m_proj.setToIdentity();
    m_proj.perspective(FOV, float(width) / float(height), 0.1f, 1000.0f);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    event->accept();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
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

void GLWidget::wheelEvent(QWheelEvent *event)
{
    m_viewDist += m_viewDist * -event->angleDelta().y() * ZOOM_SPEED;
    if (m_viewDist < MIN_VIEW_DIST) m_viewDist = MIN_VIEW_DIST;
    if (m_viewDist > MAX_VIEW_DIST) m_viewDist = MAX_VIEW_DIST;
    calcViewMat();
    update();
    event->accept();
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_R:
        resetView();
        update();
        break;
    }
}

void GLWidget::cleanupGL()
{
    makeCurrent();

    delete m_program;

    if (m_currentMesh && m_currentMesh->initialized())
        m_currentMesh->cleanupGL(this);

    doneCurrent();
}
