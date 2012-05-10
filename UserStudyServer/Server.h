#pragma once
#include <QStringList>
#include <QTcpServer>

class Server : public QTcpServer{
	Q_OBJECT

public:
	Server(QObject *parent = 0);

public slots:
	void displayAddress(QString clientAddress);

protected:
	void incomingConnection(int socketDescriptor);

signals:
	void message(QString);
};
