#ifndef USERSTUDYSERVER_H
#define USERSTUDYSERVER_H

#include <QtGui/QMainWindow>
#include "ui_userstudyserver.h"

class UserStudyServer : public QMainWindow
{
	Q_OBJECT

public:
	UserStudyServer(QWidget *parent = 0, Qt::WFlags flags = 0);
	~UserStudyServer();

private:
	Ui::UserStudyServerClass ui;
};

#endif // USERSTUDYSERVER_H
