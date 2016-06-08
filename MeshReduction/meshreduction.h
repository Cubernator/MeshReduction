#ifndef MESHREDUCTION_H
#define MESHREDUCTION_H

#include <QtWidgets/QMainWindow>
#include "ui_meshreduction.h"

class MeshReduction : public QMainWindow
{
	Q_OBJECT

public:
	MeshReduction(QWidget *parent = 0);
	~MeshReduction();

private:
	Ui::MeshReductionClass ui;
};

#endif // MESHREDUCTION_H
