#include "meshreduction.hpp"
#include <QtWidgets/QApplication>
#include <QGLFormat>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MeshReduction w;
	w.show();
	return a.exec();
}
