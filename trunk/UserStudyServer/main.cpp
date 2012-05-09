#include "userstudyserver.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	UserStudyServer w;
	w.show();
	return a.exec();
}
