#include "Server.h"
#include "ClientThread.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{

}

void Server::incomingConnection(int socketDescriptor)
{
	ClientThread *thread = new ClientThread(socketDescriptor, this);
	connect(thread, SIGNAL(clientAddress(QString)), this, SLOT(displayAddress(QString)));
	connect(thread, SIGNAL(finished()), this, SLOT(connectionDone()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(thread, SIGNAL(clientAddress(QString)), this, SLOT(message(QString)));

	emit(message("Incoming connection.."));

	thread->start();
}

void Server::displayAddress(QString clientAddress)
{
	emit(message(clientAddress));
}

void Server::connectionDone()
{
	emit(message("Done."));
}
