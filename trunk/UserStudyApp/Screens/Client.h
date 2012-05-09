#pragma once

#include <QTcpSocket>
#include <QNetworkReply>

class Client : public QTcpSocket{
	Q_OBJECT
public:
	Client(QObject * parent = 0);

	QString serverIP, serverPort;
	QString serverStatus;

public slots:
	void getServerIP();
	void receiveServerIP(QNetworkReply *reply);
	void tryConnect();
	void manageError( QAbstractSocket::SocketError socketError );

signals:
	void gotServerIP();

};
