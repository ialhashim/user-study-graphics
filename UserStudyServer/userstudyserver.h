#ifndef USERSTUDYSERVER_H
#define USERSTUDYSERVER_H

#include <QtGui/QMainWindow>
#include "ui_userstudyserver.h"

#include "Server.h"
#include "ClientThread.h"
class QNetworkReply;

class UserStudyServer : public QMainWindow
{
	Q_OBJECT

public:
	UserStudyServer(QWidget *parent = 0, Qt::WFlags flags = 0);
	~UserStudyServer();

	QString externalUrl;

	Server server;

	QString ipAddress, port;

public slots:
	void Log(QString message);
	void getIP();
	void sendIP();
	void assignIP(QNetworkReply *reply);
	void broadcastIP(QNetworkReply *reply);
	void startServer();
	void startedServer();
	void processResult();
	void processResult2();

signals:
	void gotIP();
	void sentIP();

private:
	Ui::UserStudyServerClass ui;
};

#endif // USERSTUDYSERVER_H
