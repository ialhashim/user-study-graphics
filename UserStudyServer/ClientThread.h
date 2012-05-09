#pragma once

#include <QThread>
#include <QTcpSocket>

class ClientThread : public QThread
{
	Q_OBJECT

public:
	ClientThread(int socketDescriptor, QObject *parent);

	void run();

signals:
	void error(QTcpSocket::SocketError socketError);

private:
	int socketDescriptor;
};
