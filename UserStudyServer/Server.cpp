#include "Server.h"
#include "ClientThread.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{

}

void Server::incomingConnection(int socketDescriptor)
{
	ClientThread *thread = new ClientThread(socketDescriptor, this);
	connect(thread, SIGNAL(clientAddress(QString)), this, SLOT(displayAddress(QString)));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

	emit(message("Incoming connection.."));

	thread->start();
}

void Server::displayAddress(QString clientAddress)
{
	emit(message(clientAddress));
}
