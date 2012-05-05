#include "userstudy.h"
#include <QtGui/QApplication>
#include <QGLFormat>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	// Anti-aliasing
	QGLFormat glf = QGLFormat::defaultFormat();
	glf.setSamples(8);
	QGLFormat::setDefaultFormat(glf);

	UserStudyApp w;
	w.show();
	return a.exec();
}
