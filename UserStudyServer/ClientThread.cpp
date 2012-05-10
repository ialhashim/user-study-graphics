#include "ClientThread.h"

#include <QBuffer>
#include <QtNetwork>
#include <QDomDocument>
#include <QDomElement>

ClientThread::ClientThread(int socketDescriptor, QObject *parent) : QThread(parent), socketDescriptor(socketDescriptor)
{

}

void ClientThread::run()
{
	QTcpSocket tcpSocket;
	if (!tcpSocket.setSocketDescriptor(socketDescriptor)) {
		emit error(tcpSocket.error());
		return;
	}

	tcpSocket.waitForReadyRead();

	// Read into a buffer
	QByteArray b = tcpSocket.readAll();
	QDomDocument doc;
	doc.setContent(QString(b));

	QDomElement elt = doc.firstChild().firstChildElement("submit-id");
	QString filename = elt.text();
	filename += ".xml";

	QFile File(filename);
	if ( File.open( QIODevice::WriteOnly | QIODevice::Text ) ) 
	{
		QTextStream TextStream(&File);
		TextStream << doc.toString() ;
		File.close();
	}

	// Disconnect
	tcpSocket.disconnectFromHost();
	tcpSocket.waitForDisconnected();

	emit(clientAddress(tcpSocket.peerAddress().toString()));
}
