#ifndef EXPORTDIALOG_HPP
#define EXPORTDIALOG_HPP

#include "scenefile.hpp"

#include <QDialog>

namespace Ui {
class ExportDialog;
}

class Mesh;

class ExportDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::ExportDialog *ui;

    std::vector<ExportFormat> m_formats;
    SceneFile* m_scene;
    bool m_error;

public:
    explicit ExportDialog(SceneFile* scene, Mesh* selectedMesh, QWidget *parent = 0);
    ~ExportDialog();

    unsigned int findFittingFormat(const QString& ext) const;

    QString currentFileName() const;
    unsigned int selectedFormatIndex() const;
    bool syncFileExtension() const;

    bool hasError() const { return m_error; }

private slots:
    void setCurrentFileName(QString fileName);
    void onBrowseButton();
    void onFormatSelected(int index);
    void onSyncExtensionChanged();

public slots:
    void accept() Q_DECL_OVERRIDE;
};

#endif // EXPORTDIALOG_HPP
