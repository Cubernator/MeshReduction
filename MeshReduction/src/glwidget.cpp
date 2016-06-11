#include "glwidget.hpp"
#include "mesh.hpp"

#define GLWIDGET_TIMER_INTERVAL 17

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent), m_currentMesh(nullptr)
{

}

GLWidget::~GLWidget()
{
    cleanupGL();
}

void GLWidget::setCurrentMesh(Mesh* mesh)
{
    if (mesh != m_currentMesh) {
        makeCurrent();

        if (m_currentMesh)
            m_currentMesh->cleanupGL();

        m_currentMesh = mesh;

        if (m_currentMesh)
            m_currentMesh->initGL();

        doneCurrent();

        update();
    }
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
    m_colorLoc = m_program->uniformLocation("color");

    m_program->release();

    if (m_currentMesh && !m_currentMesh->initialized()) {
        m_currentMesh->initGL();
    }

    m_world.setToIdentity();

    m_view.setToIdentity();
    m_view.translate(0.0f, 0.0f, -10.0f);

    m_color = QVector4D(1.0f, 1.0f, 1.0f, 1.0f);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_program->bind();

    m_program->setUniformValue(m_mvpLoc, m_proj * m_view * m_world);
    m_program->setUniformValue(m_colorLoc, m_color);

    if (m_currentMesh && m_currentMesh->initialized()) {
        m_currentMesh->draw(this);
    }

    m_program->release();
}

void GLWidget::resizeGL(int width, int height)
{
    m_proj.setToIdentity();
    m_proj.perspective(45.0f, float(width) / float(height), 0.1f, 100.0f);
}

void GLWidget::cleanupGL()
{
    makeCurrent();

    delete m_program;

    if (m_currentMesh && m_currentMesh->initialized())
        m_currentMesh->cleanupGL();

    doneCurrent();
}
