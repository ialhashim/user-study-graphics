#pragma once

#include <QThread>
#include <QTcpSocket>

class ClientThread : public QThread
{
	Q_OBJECT

public:
	ClientThread(int socketDescriptor, QObject *parent);

	void run();

	QString address;

private:
	int socketDescriptor;

signals:
	void error(QTcpSocket::SocketError socketError);
	void clientAddress(QString);
};
