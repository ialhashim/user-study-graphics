#include "ClientThread.h"

#include <QBuffer>
#include <QtNetwork>
#include <QDomDocument>
#include <QDomElement>
#include <iostream>

ClientThread::ClientThread(int socketDescriptor, QObject *parent) : QThread(parent), socketDescriptor(socketDescriptor)
{

}

int ClientThread::waitForInput( QTcpSocket *socket )
{
	int bytesAvail = -1;
	while (socket->state() == QAbstractSocket::ConnectedState && bytesAvail < 0) {
		
		bytesAvail = 0;

		if (socket->waitForReadyRead(100)) {
			bytesAvail = socket->bytesAvailable();
		}
	}
	return bytesAvail;
}

QString ClientThread::readLine(QTcpSocket *socket )
{
	QString line = "";
	int bytesAvail = waitForInput( socket );   
	if (bytesAvail > 0) {
		int cnt = 0;
		bool endOfLine = false;
		bool endOfStream = false;
		while (cnt < bytesAvail && (!endOfLine) && (!endOfStream)) {
			char ch;
			int bytesRead = socket->read(&ch, sizeof(ch));
			if (bytesRead == sizeof(ch)) {
				cnt++;
				line.append( ch );
				if (ch == '\r') {
					endOfLine = true;
				}
			}
			else {
				endOfStream = true;
			}
		}
	}
	return line;
}

void ClientThread::run()
{
	QTcpSocket tcpSocket;
	if (!tcpSocket.setSocketDescriptor(socketDescriptor)) {
		emit error(tcpSocket.error());
		return;
	}

	//tcpSocket.waitForReadyRead();

	// Read into a buffer
	/*QByteArray b = tcpSocket.readAll();
	QDomDocument doc;
	doc.setContent(QString(b));

	// File name from XML content
	QDomElement elt = doc.firstChild().firstChildElement("submit-id");
	QString filename = elt.text();
	filename += "0.xml";

	emit(clientAddress(tcpSocket.peerAddress().toString()));*/

	QString unique = QUuid::createUuid().toString() + QString::number(QDateTime().toTime_t());
	QString id = QString::number(qHash(unique));

	QString filename = "submission-" + id + ".xml";

	QFile File(filename);

	if ( File.open( QIODevice::WriteOnly | QIODevice::Text ) ) 
	{
		QTextStream TextStream(&File);
		
		std::cout << "\n== Start of packet: == \n\n";

		int bytesAvail = waitForInput( &tcpSocket );   

		while( bytesAvail > 0 )
		{
			QString data = QString(tcpSocket.readAll());

			TextStream << data;
			
			std::cout << qPrintable(data) << "\n\n=== END PACKET ===\n\n" << std::endl;

			bytesAvail = waitForInput( &tcpSocket );
		}

		File.close();
	}

	// Disconnect
	tcpSocket.disconnectFromHost();
	//tcpSocket.waitForDisconnected();
}
