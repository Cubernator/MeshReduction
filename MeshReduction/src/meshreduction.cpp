#include "meshreduction.hpp"

#include "scenefile.hpp"
#include "mesh.hpp"
#include "mesh_decimator.hpp"
#include "exportdialog.hpp"

#include <QtWidgets\QFileDialog>
#include <QtWidgets\QMessageBox>
#include <QSettings>
#include <QThread>
#include <QProgressDialog>

MeshReduction::MeshReduction(QWidget *parent) : QMainWindow(parent), m_isDecimating(false)
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
    connect(ui.actionRefresh, SIGNAL(triggered(bool)), this, SIGNAL(meshChanged()));
    connect(ui.actionDraw_Faces, SIGNAL(toggled(bool)), m_glWidget, SLOT(setDrawFaces(bool)));
    connect(ui.actionDraw_Wireframe, SIGNAL(toggled(bool)), m_glWidget, SLOT(setDrawWireframe(bool)));

    connect(ui.actionOpen, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(ui.actionExport, SIGNAL(triggered(bool)), this, SLOT(showExportDialog()));
    connect(ui.actionExit, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(ui.actionClose, SIGNAL(triggered(bool)), this, SLOT(closeFile()));
    connect(ui.actionClear_List, SIGNAL(triggered(bool)), this, SLOT(clearRecentFiles()));

    connect(ui.actionReset_Mesh, SIGNAL(triggered(bool)), this, SLOT(resetMesh()));

    connect(ui.decimateButton, SIGNAL(clicked(bool)), this, SLOT(decimateMesh()));
    connect(ui.resetButton, SIGNAL(clicked(bool)), this, SLOT(resetMesh()));

    connect(ui.targetFaceCount, SIGNAL(editingFinished()), this, SLOT(onSetTargetFaceCount()));
    connect(ui.percentageBox, SIGNAL(editingFinished()), this, SLOT(onSetPercentageBox()));
    connect(ui.percentageSlider, SIGNAL(sliderMoved(int)), this, SLOT(onSetPercentageSlider()));

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
        ui.actionExport->setEnabled(isFile);
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
    QString meshName;
    QString vertexCount, edgeCount, faceCount;
    QString origVertexCount, origEgdeCount, origFaceCount;

    unsigned int fc = 0, ofc = 0;

    if (m_selectedMesh) {
        meshName = getFormattedMeshName(m_selectedMesh);

        fc = m_selectedMesh->faceCount();
        ofc = m_selectedMesh->importedFaceCount();

        vertexCount = tr("%1").arg(m_selectedMesh->vertexCount());
        edgeCount = tr("%1").arg(m_selectedMesh->edgeCount());
        faceCount = tr("%1").arg(fc);

        origVertexCount = tr("%1").arg(m_selectedMesh->importedVertexCount());
        origEgdeCount = tr("%1").arg(m_selectedMesh->importedEdgeCount());
        origFaceCount = tr("%1").arg(ofc);
    }

    ui.primitivesTable->setItem(0, 0, new QTableWidgetItem(vertexCount));
    ui.primitivesTable->setItem(0, 1, new QTableWidgetItem(origVertexCount));
    ui.primitivesTable->setItem(1, 0, new QTableWidgetItem(edgeCount));
    ui.primitivesTable->setItem(1, 1, new QTableWidgetItem(origEgdeCount));
    ui.primitivesTable->setItem(2, 0, new QTableWidgetItem(faceCount));
    ui.primitivesTable->setItem(2, 1, new QTableWidgetItem(origFaceCount));

    ui.targetFaceCount->setMaximum(ofc);
    ui.targetFaceCount->setValue(fc);

    onSetTargetFaceCount();
}

void MeshReduction::resetMesh()
{
    if ((m_selectedMesh != nullptr) && !m_isDecimating) {
        m_selectedMesh->reset();

        emit meshChanged();
    }
}

void MeshReduction::decimateMesh()
{
    if ((m_selectedMesh != nullptr) && !m_isDecimating) {
        unsigned int targetFaceCount = ui.targetFaceCount->value();
        unsigned int currentFaceCount = m_selectedMesh->faceCount();
        if (targetFaceCount > currentFaceCount) {
            m_selectedMesh->reset();
        }

        m_progressDialog.reset(new QProgressDialog(tr("Decimating Mesh..."), tr("Abort"), 0, 100, this));
        m_progressDialog->setWindowModality(Qt::WindowModal);
        m_progressDialog->setMinimumDuration(2000);
        m_progressDialog->setValue(0);

        QThread* thread = new QThread(this);
        MeshDecimator* decimator = new MeshDecimator(m_selectedMesh, targetFaceCount);

        decimator->moveToThread(thread);

        connect(thread, SIGNAL(started()), decimator, SLOT(start()));
        connect(decimator, SIGNAL(finished()), thread, SLOT(quit()));
        connect(decimator, SIGNAL(progressChanged(float)), this, SLOT(onDecimateProgress(float)));
        connect(thread, SIGNAL(finished()), decimator, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), this, SLOT(onFinishDecimating()));

        connect(m_progressDialog.get(), SIGNAL(canceled()), decimator, SLOT(abort()), Qt::DirectConnection);

        connect(thread, SIGNAL(started()), this, SLOT(onStartDecimating()));
        connect(thread, SIGNAL(finished()), this, SLOT(onFinishDecimating()));

        thread->start();
    }
}

void MeshReduction::onDecimateProgress(float value)
{
    if (m_progressDialog && (value >= 0.0f && value <= 1.0f)) {
        int p = m_progressDialog->maximum() * value;
        m_progressDialog->setValue(p);
    }
}

void MeshReduction::onStartDecimating()
{
    setIsDecimating(true);
}

void MeshReduction::onFinishDecimating()
{
    setIsDecimating(false);

    m_progressDialog.reset();

    emit meshChanged();
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

void MeshReduction::onSetTargetFaceCount()
{
    Mesh* mesh = selectedMesh();
    if (mesh) {
        int value = ui.targetFaceCount->value();

        unsigned int original = mesh->importedFaceCount();

        double percentage = 100.0 * (double(value) / double(original));

        ui.percentageBox->setValue(percentage);
        ui.percentageSlider->setValue(int(percentage));
    }
}

void MeshReduction::onSetPercentageBox()
{
    Mesh* mesh = selectedMesh();
    if (mesh) {
        double value = ui.percentageBox->value();

        unsigned int original = mesh->importedFaceCount();

        double factor = value * 0.01;

        ui.targetFaceCount->setValue(int(original * factor));
        ui.percentageSlider->setValue(int(value));
    }
}

void MeshReduction::onSetPercentageSlider()
{
    Mesh* mesh = selectedMesh();
    if (mesh) {
        int value = ui.percentageSlider->value();

        unsigned int original = mesh->importedFaceCount();

        double factor = value * 0.01;

        ui.targetFaceCount->setValue(int(original * factor));
        ui.percentageBox->setValue(double(value));
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

void MeshReduction::setIsDecimating(bool value)
{
    m_isDecimating = value;

    ui.decimateButton->setEnabled(!value);
    ui.resetButton->setEnabled(!value);
    ui.actionReset_Mesh->setEnabled(!value);
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

void MeshReduction::showExportDialog()
{
    ExportDialog dialog(m_currentFile.get(), m_selectedMesh, this);
    dialog.exec();
}
