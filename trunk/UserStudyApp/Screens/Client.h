#pragma once

#include <QTcpSocket>
#include <QNetworkReply>
#include <QMap>
#include <QString>

class Client : public QTcpSocket{
	Q_OBJECT
public:
	Client(QString dataSend, QObject * parent = 0);

	QString serverIP, serverPort;
	QString serverStatus;

	QString data;

public slots:
	void getServerIP();
	void receiveServerIP(QNetworkReply *reply);
	void tryConnect();
	void manageError( QAbstractSocket::SocketError socketError );
	void sendData();

signals:
	void gotServerIP();
};
