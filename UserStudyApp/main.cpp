#include "userstudy.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	UserStudyApp w;
	w.show();
	return a.exec();
}
