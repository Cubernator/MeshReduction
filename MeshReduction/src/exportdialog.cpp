#include "exportdialog.hpp"
#include "ui_exportdialog.h"
#include "mesh.hpp"
#include "meshreduction.hpp"

#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

ExportDialog::ExportDialog(SceneFile *scene, Mesh *selectedMesh, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportDialog),
    m_scene(scene),
    m_error(false)
{
    ui->setupUi(this);

    m_formats = SceneFile::getExportFormats();

    for (ExportFormat& format : m_formats) {
        ui->formatSelector->addItem(format.m_desc);
    }

    unsigned int n = m_scene->numMeshes();
    for (unsigned int i = 0; i < n; ++i) {
        Mesh * mesh = m_scene->getMesh(i);
        QListWidgetItem * item = new QListWidgetItem(MeshReduction::getFormattedMeshName(mesh), ui->includedMeshList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        bool checked = mesh == selectedMesh;
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);

        ui->includedMeshList->addItem(item);
    }

    connect(ui->browseButton, SIGNAL(clicked(bool)), this, SLOT(onBrowseButton()));
    connect(ui->syncExtension, SIGNAL(clicked(bool)), this, SLOT(onSyncExtensionChanged()));
    connect(ui->formatSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(onFormatSelected(int)));

    QFileInfo fi(scene->fileName());
    QString importedPath = fi.path();

    QSettings settings;
    QString lastPath = settings.value("lastExportPath", importedPath).toString();
    QString fileName = QDir::cleanPath(lastPath + QDir::separator() + fi.fileName());

    setCurrentFileName(fileName);
    onFormatSelected(selectedFormatIndex());
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

unsigned int ExportDialog::findFittingFormat(const QString &ext) const
{
    QString e = ext.toLower();
    for (unsigned int i = 0; i < m_formats.size(); ++i) {
        if (m_formats[i].m_extension == e) {
            return i;
        }
    }

    return 0;
}

QString ExportDialog::currentFileName() const
{
    return ui->filePathField->text();
}

unsigned int ExportDialog::selectedFormatIndex() const
{
    return ui->formatSelector->currentIndex();
}

bool ExportDialog::syncFileExtension() const
{
    return ui->syncExtension->isChecked();
}

void ExportDialog::setCurrentFileName(QString fileName)
{
    ui->filePathField->setText(fileName);

    QFileInfo fi(fileName);
    unsigned int fIndex = findFittingFormat(fi.suffix());
    ui->formatSelector->setCurrentIndex(fIndex);
}

void ExportDialog::onBrowseButton()
{
    QFileInfo fi(currentFileName());

    QString filter;

    for (unsigned int i = 0; i < m_formats.size(); ++i) {
        const ExportFormat& format = m_formats[i];
        filter += tr("%1 (*.%2);;").arg(format.m_desc, format.m_extension);
    }

    filter += tr("All Files (*.*)");

    QString newFileName = QFileDialog::getSaveFileName(this, tr("Save File As"), fi.path(), filter);
    if (!newFileName.isNull()) {
        setCurrentFileName(newFileName);
    }
}

void ExportDialog::onFormatSelected(int index)
{
    if (syncFileExtension()) {
        QFileInfo fi(currentFileName());
        QString newFileName = fi.path() + QDir::separator() + fi.completeBaseName() + "." + m_formats[index].m_extension;

        setCurrentFileName(QDir::cleanPath(newFileName));
    }
}

void ExportDialog::onSyncExtensionChanged()
{
    onFormatSelected(selectedFormatIndex());
}

void ExportDialog::accept()
{
    QString fn = currentFileName();
    QString formatId = m_formats[selectedFormatIndex()].m_id;

    std::vector<bool> meshMask(m_scene->numMeshes());
    for (unsigned int i = 0; i < meshMask.size(); ++i) {
        Qt::CheckState cs = ui->includedMeshList->item(i)->checkState();
        meshMask[i] = cs != Qt::Unchecked;
    }

    QString errorString = m_scene->exportToFile(fn, formatId, meshMask);
    if (!errorString.isEmpty()) {
        m_error = true;
        QMessageBox::critical(this, tr("Failed to export scene!"), errorString);
    }

    QFileInfo fi(fn);

    QSettings settings;
    settings.setValue("lastExportPath", fi.path());

    QDialog::accept();
}
