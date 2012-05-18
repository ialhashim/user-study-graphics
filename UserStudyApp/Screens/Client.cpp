#include "Client.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include <QMessageBox>
#include <iostream>
#include <QMetaEnum>
#include <QDomDocument>
#include <QUuid>
#include <QDateTime>
#include <QHostInfo>
#include <QApplication>


Client::Client(QString dataSend, QObject * parent) : QTcpSocket(parent)
{
	serverStatus = "Not connected.";

	connect(this, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(manageError(QAbstractSocket::SocketError)));

	connect(this, SIGNAL(gotServerIP()), SLOT(tryConnect()));
	connect(this, SIGNAL(connected()), SLOT(sendData()));

	getServerIP();

	this->data = dataSend;

	isSentData = false;
}

void Client::getServerIP()
{
	QString externalUrl = "http://www.cs.sfu.ca/~iaa7/personal/";

	QNetworkAccessManager * am = new QNetworkAccessManager(this);
	QNetworkReply *reply = am->get(QNetworkRequest(QUrl(externalUrl + "getip.php")));

	connect(am, SIGNAL(finished(QNetworkReply*)), SLOT(receiveServerIP(QNetworkReply*)));
}

void Client::receiveServerIP( QNetworkReply *reply )
{
	if(reply->error() == QNetworkReply::NoError)
	{
		QString ipResponse = reply->readAll().trimmed();
		QStringList ipInfo = ipResponse.split(" ");

		if(ipInfo.size() > 1){

			serverIP = ipInfo[0];
			serverPort = ipInfo[1];

			serverStatus = "Got server IP: " + serverIP + " at port: " + serverPort;

			std::cout << "Got server IP :" << qPrintable(serverIP) << " \t port:" << qPrintable(serverPort) << "\n";

			emit(gotServerIP());
			return;
		}
	}

	QMessageBox::information(0, "Network error", "Could not get server IP address or port.");
	serverStatus = "Cannot get server IP.";

	reply->deleteLater();
}

void Client::tryConnect()
{
	std::cout << "Connecting to: " << qPrintable(serverIP) << "\n";

	connectToHost(serverIP, serverPort.toInt());
	
	waitForConnected(5000);
	
	if(state() != QAbstractSocket::ConnectedState)
	{
		QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

		QMessageBox::information(0, "Network warning", "If you got 'Successfully sent to server' ignore this warning. Otherwise, try again one more time or use the method below.");
		this->isSentData = false;
		this->close();
	}
}

void Client::sendData()
{
	this->isSentData = true;

	if(state() == QAbstractSocket::ConnectedState)
	{
		write(qPrintable(data));

		this->waitForBytesWritten();
		this->waitForDisconnected(5000);

		QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

		QMessageBox::information(0, "Thank you", "Successfully sent to server!");
	}
}

void Client::manageError( QAbstractSocket::SocketError socketError )
{
	QMetaObject meta = QAbstractSocket::staticMetaObject;
	for (int i=0; i < meta.enumeratorCount(); ++i){
		QMetaEnum m = meta.enumerator(i);
		if(QString(m.name()) == "SocketError"){
			for(int j = 0; j < m.keyCount(); j++){
				if(m.value(j) == socketError){
					printf("%s\n", qPrintable(QString(m.valueToKey(socketError))));
					//QMessageBox::information(0, "Network error", "Socket Error: " + QString(m.valueToKey(m.value(j))));
				}
			}
		}
	}
}
