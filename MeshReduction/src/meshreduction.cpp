#include "meshreduction.hpp"

#include "scenefile.hpp"
#include "mesh.hpp"

#include <QtWidgets\QFileDialog>
#include <QtWidgets\QMessageBox>
#include <QSettings>

MeshReduction::MeshReduction(QWidget *parent) : QMainWindow(parent)
{
    QCoreApplication::setApplicationName("MeshReduction");
    QCoreApplication::setOrganizationName("VectorSmash");
    QCoreApplication::setApplicationVersion("v0.1");

	ui.setupUi(this);

	m_glWidget = ui.openGLWidget;

    connect(ui.meshList, SIGNAL(currentRowChanged(int)), this, SLOT(handleMeshSelection(int)));

    connect(this, SIGNAL(selectedMeshChanged(Mesh*)), m_glWidget, SLOT(setCurrentMesh(Mesh*)));

    connect(ui.actionOpen, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(ui.actionExit, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(ui.actionClose, SIGNAL(triggered(bool)), this, SLOT(closeFile()));
    connect(ui.actionClear_List, SIGNAL(triggered(bool)), this, SLOT(clearRecentFiles()));

    QAction* firstAction = nullptr;
    if (!ui.menuRecent_Files->actions().isEmpty())
        firstAction = ui.menuRecent_Files->actions().first();

    for (int i = 0; i < MAX_RECENTFILES; ++i) {
        m_recentFileActions[i] = new QAction(this);
        m_recentFileActions[i]->setVisible(false);
        connect(m_recentFileActions[i], SIGNAL(triggered(bool)), this, SLOT(openRecentFile()));
        ui.menuRecent_Files->insertAction(firstAction, m_recentFileActions[i]);
    }

    updateRecentFileActions();
}

MeshReduction::~MeshReduction()
{
	
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

        if (file) {
            QString fileName = file->fileName();
            setWindowFilePath(fileName);

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
            setWindowFilePath("");
            ui.actionClose->setText(tr("&Close"));
        }

        ui.actionClose->setEnabled(file != nullptr);
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

void MeshReduction::selectMesh(Mesh *mesh)
{
    if (mesh != m_selectedMesh) {
        m_selectedMesh = mesh;
        emit selectedMeshChanged(mesh);
    }
}

void MeshReduction::populateMeshList()
{
    ui.meshList->clear();
    if (m_currentFile) {
        for (unsigned int i = 0; i < m_currentFile->numMeshes(); ++i) {
            ui.meshList->addItem(m_currentFile->getMesh(i)->name());
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
        msgBox.setText(tr("Failed to open file!"));
        msgBox.setInformativeText(newFile->errorString());
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();

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
