#include "userstudy.h"
#include <QtGui/QApplication>
#include <QGLFormat>
#include <QTextCodec>
#include <QTime>

QTime * globalTimer = NULL;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

	// Anti-aliasing
	QGLFormat glf = QGLFormat::defaultFormat();
	glf.setSamples(8);
	QGLFormat::setDefaultFormat(glf);

	// Timing
	globalTimer = new QTime;
	globalTimer->start();

	UserStudyApp w;
	w.show();
	return a.exec();
}
