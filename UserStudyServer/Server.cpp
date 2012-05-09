#include "Server.h"
#include "ClientThread.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{

}

void Server::incomingConnection(int socketDescriptor)
{
	emit(message("Incoming connection.."));

	ClientThread *thread = new ClientThread(socketDescriptor, this);
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	thread->start();
}
