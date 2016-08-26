#ifndef MESHREDUCTION_H
#define MESHREDUCTION_H

#include <memory>

#include <QtWidgets/QMainWindow>
#include <QAction>

#include "ui_meshreduction.h"

#define MAX_RECENTFILES 10

class SceneFile;
class Mesh;

class QProgressDialog;

class MeshReduction : public QMainWindow
{
	Q_OBJECT

private:
	Ui::MeshReductionClass ui;

    bool m_isDecimating;

	std::unique_ptr<SceneFile> m_currentFile;
    Mesh* m_selectedMesh;
	GLWidget* m_glWidget;

    std::unique_ptr<QProgressDialog> m_progressDialog;

    QAction* m_recentFileActions[MAX_RECENTFILES];

    void openFile(const QString& fileName);
    void updateRecentFileActions();
    void populateMeshList();

    void setIsDecimating(bool value);

public:
	MeshReduction(QWidget *parent = 0);
	~MeshReduction();

    inline SceneFile* currentFile() const { return m_currentFile.get(); }
    inline Mesh* selectedMesh() const { return m_selectedMesh; }

    static QString getFormattedMeshName(const Mesh* mesh);

signals:
	void currentFileChanged(SceneFile* newFile);
    void selectedMeshChanged(Mesh* mesh);
    void meshChanged();

public slots:
	void setCurrentFile(SceneFile* file);
    void selectMesh(Mesh* mesh);

private slots:
    void openFile();
    void openRecentFile();
    void clearRecentFiles();
    void closeFile();
    void showExportDialog();
    void handleMeshSelection(int index);
    void updateMeshProperties();
    void resetMesh();
    void decimateMesh();
    void onDecimateProgress(float value);
    void onStartDecimating();
    void onFinishDecimating();

    void onSetTargetFaceCount();
    void onSetPercentageBox();
    void onSetPercentageSlider();
};

#endif // MESHREDUCTION_H
