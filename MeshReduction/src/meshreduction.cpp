#include "meshreduction.hpp"

#include "scenefile.hpp"
#include "mesh.hpp"

#include <QtWidgets\QFileDialog>
#include <QtWidgets\QMessageBox>
#include <QSettings>
#include <QProgressDialog>

MeshReduction::MeshReduction(QWidget *parent) : QMainWindow(parent)
{
    QCoreApplication::setApplicationName("MeshReduction");
    QCoreApplication::setOrganizationName("VectorSmash");
    QCoreApplication::setApplicationVersion("v0.1");

    QGuiApplication::setApplicationDisplayName("MeshReduction");

	ui.setupUi(this);

	m_glWidget = ui.openGLWidget;

    ui.actionDraw_Faces->setChecked(m_glWidget->drawFaces());
    ui.actionDraw_Wireframe->setChecked(m_glWidget->drawWireframe());

    connect(ui.meshList, SIGNAL(currentRowChanged(int)), this, SLOT(handleMeshSelection(int)));

    connect(this, SIGNAL(selectedMeshChanged(Mesh*)), m_glWidget, SLOT(setCurrentMesh(Mesh*)));
    connect(this, SIGNAL(selectedMeshChanged(Mesh*)), this, SLOT(updateMeshProperties()));

    connect(this, SIGNAL(meshChanged()), m_glWidget, SLOT(reinitMesh()));
    connect(this, SIGNAL(meshChanged()), this, SLOT(updateMeshProperties()));

    connect(ui.actionReset_View, SIGNAL(triggered(bool)), m_glWidget, SLOT(resetView()));
    connect(ui.actionDraw_Faces, SIGNAL(toggled(bool)), m_glWidget, SLOT(setDrawFaces(bool)));
    connect(ui.actionDraw_Wireframe, SIGNAL(toggled(bool)), m_glWidget, SLOT(setDrawWireframe(bool)));

    connect(ui.actionOpen, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(ui.actionExit, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(ui.actionClose, SIGNAL(triggered(bool)), this, SLOT(closeFile()));
    connect(ui.actionClear_List, SIGNAL(triggered(bool)), this, SLOT(clearRecentFiles()));

    connect(ui.actionReset_Mesh, SIGNAL(triggered(bool)), this, SLOT(resetMesh()));

    connect(ui.decimateButton, SIGNAL(clicked(bool)), this, SLOT(decimateMesh()));

    QAction* firstAction = nullptr;
    if (!ui.menuRecent_Files->actions().isEmpty())
        firstAction = ui.menuRecent_Files->actions().first();

    for (int i = 0; i < MAX_RECENTFILES; ++i) {
        m_recentFileActions[i] = new QAction(this);
        m_recentFileActions[i]->setVisible(false);
        connect(m_recentFileActions[i], SIGNAL(triggered(bool)), this, SLOT(openRecentFile()));
        ui.menuRecent_Files->insertAction(firstAction, m_recentFileActions[i]);
    }

    ui.sceneSideBar->setEnabled(false);
    ui.meshSideBar->setEnabled(false);

    updateRecentFileActions();
}

MeshReduction::~MeshReduction()
{
}

QString MeshReduction::getFormattedMeshName(const Mesh *mesh)
{
    QString name = tr("<none>");
    if (mesh) {
        name = mesh->name();
        if (name.isEmpty())
            name = tr("<unnamed mesh>");
    }
    return name;
}

void MeshReduction::updateRecentFileActions()
{
    QSettings settings;
    QStringList fileList = settings.value("recentFileList").toStringList();

    int rfCount = qMin(fileList.size(), MAX_RECENTFILES);

    for (int i = 0; i < rfCount; ++i) {
        QString t = tr("&%1. %2").arg(i + 1).arg(fileList[i]);
        m_recentFileActions[i]->setText(t);
        m_recentFileActions[i]->setData(fileList[i]);
        m_recentFileActions[i]->setVisible(true);
    }

    for (int j = rfCount; j < MAX_RECENTFILES; ++j) {
        m_recentFileActions[j]->setVisible(false);
    }

    ui.actionClear_List->setEnabled(rfCount > 0);
}

void MeshReduction::clearRecentFiles()
{
    QSettings settings;
    settings.setValue("recentFileList", QStringList());
    updateRecentFileActions();
}

void MeshReduction::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        openFile(action->data().toString());
    }
}

void MeshReduction::setCurrentFile(SceneFile* file)
{
	if (file != m_currentFile.get()) {
		m_currentFile.reset(file);
		emit currentFileChanged(file);

        populateMeshList();

        QString fileName = "";
        QString meshCount = "";

        if (file) {
            fileName = file->fileName();
            meshCount = tr("%1").arg(file->numMeshes());

            QSettings settings;
            QStringList fileList = settings.value("recentFileList").toStringList();

            fileList.removeAll(fileName);
            fileList.prepend(fileName);
            while (fileList.size() > MAX_RECENTFILES)
                fileList.removeLast();

            settings.setValue("recentFileList", fileList);

            updateRecentFileActions();

            if (file->numMeshes() > 0) {
                ui.meshList->setCurrentRow(0);
            }

            QFileInfo fi(fileName);
            ui.actionClose->setText(tr("&Close \"%1\"").arg(fi.fileName()));
        } else {
            ui.actionClose->setText(tr("&Close"));
        }

        setWindowFilePath(fileName);
        ui.fileNameLabel->setText(fileName);
        ui.fileNameLabel->setToolTip(fileName);
        ui.meshCountLabel->setText(meshCount);

        bool isFile = file != nullptr;

        ui.sceneSideBar->setEnabled(isFile);
        ui.actionClose->setEnabled(isFile);
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

void MeshReduction::updateMeshProperties()
{
    QString meshName = "";
    QString vertexCount = "";
    QString edgeCount = "";
    QString faceCount = "";
    QString origFaceCount = "";

    unsigned int fc = 0, ofc = 0;

    if (m_selectedMesh) {
        fc = m_selectedMesh->faceCount();
        ofc = m_selectedMesh->importedFaceCount();

        meshName = getFormattedMeshName(m_selectedMesh);
        vertexCount = tr("%1").arg(m_selectedMesh->vertexCount());
        edgeCount = tr("%1").arg(m_selectedMesh->edgeCount());
        faceCount = tr("%1").arg(fc);
        origFaceCount = tr("%1").arg(ofc);
    }

    ui.meshNameLabel->setText(meshName);
    ui.meshNameLabel->setToolTip(meshName);
    ui.vertexCountLabel->setText(vertexCount);
    ui.edgeCountLabel->setText(edgeCount);
    ui.faceCountLabel->setText(faceCount);
    ui.originalFaceCountLabel->setText(origFaceCount);

    ui.targetFaceCount->setMaximum(ofc);
    ui.targetFaceCount->setValue(fc);
}

void MeshReduction::resetMesh()
{
    if (m_selectedMesh != nullptr) {
        m_selectedMesh->reset();

        emit meshChanged();
    }
}

void MeshReduction::decimateMesh()
{
    if (m_selectedMesh != nullptr) {
        unsigned int targetFaceCount = ui.targetFaceCount->value();
        if (targetFaceCount > m_selectedMesh->faceCount()) {
            m_selectedMesh->reset();
        }

        const int maxProgress = 100;

        QProgressDialog progress("Decimating mesh.", "Cancel", 0, maxProgress, this);
        progress.setWindowModality(Qt::WindowModal);

        auto fun = [&progress] (float p) {
            progress.setValue(int(maxProgress * p));
            return progress.wasCanceled();
        };

        progress.setValue(0);

        try {
            m_selectedMesh->decimate(targetFaceCount, fun);

            progress.setValue(maxProgress);

        } catch (std::runtime_error& e) {
            QMessageBox msgBox;
            msgBox.critical(nullptr, tr("Operation failed!"),
                            tr("An exception was thrown during execution:\n\"%1\"").arg(e.what()));
        }

        emit meshChanged();
    }
}

void MeshReduction::selectMesh(Mesh *mesh)
{
    if (mesh != m_selectedMesh) {
        m_selectedMesh = mesh;
        emit selectedMeshChanged(mesh);

        bool isMesh = mesh != nullptr;
        ui.meshSideBar->setEnabled(isMesh);
    }
}

void MeshReduction::populateMeshList()
{
    ui.meshList->clear();
    if (m_currentFile) {
        for (unsigned int i = 0; i < m_currentFile->numMeshes(); ++i) {
            ui.meshList->addItem(getFormattedMeshName(m_currentFile->getMesh(i)));
        }
    }
}

void MeshReduction::openFile()
{
    QSettings settings;
    QString lastPath = settings.value("lastOpenPath").toString();

    QString filter = tr("3D Models (%1);;All Files (*.*)").arg(SceneFile::getImportExtensions());

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), lastPath, filter);

	if (!fileName.isNull()) {
        QFileInfo fi(fileName);
        settings.setValue("lastOpenPath", fi.path());

        openFile(fileName);
    }
}

void MeshReduction::openFile(const QString &fileName)
{
    SceneFile* newFile = new SceneFile(fileName);

    if (newFile->hasError()) {
        QMessageBox msgBox;
        msgBox.critical(nullptr, tr("Failed to open file!"), newFile->errorString());

        delete newFile;
    } else {
        statusBar()->showMessage(tr("Opened file") + ": \"" + newFile->fileName() + "\"");
        setCurrentFile(newFile);
    }
}

void MeshReduction::closeFile()
{
    setCurrentFile(nullptr);
}
