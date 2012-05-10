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

Client::Client(QObject * parent) : QTcpSocket(parent)
{
	serverStatus = "Not connected.";

	connect(this, SIGNAL(gotServerIP()), SLOT(tryConnect()));
	connect(this, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(manageError(QAbstractSocket::SocketError)));
	connect(this, SIGNAL(connected()), SLOT(sendData()));

	getServerIP();
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

			std::cout << "Got server IP :" << qPrintable(serverIP) << "\n";

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
	if(!waitForConnected(3000)){
		QMessageBox::information(0, "Network error", "Sorry, cannot connect to server.");
		this->close();
	}
}

void Client::sendData()
{
	if(state() == QAbstractSocket::ConnectedState)
	{
		QDomDocument doc("Submission");
		QDomElement root = doc.createElement("Submission");
		doc.appendChild(root);

		// Submission ID
		QDomElement submitId = doc.createElement("submit-id");
		root.appendChild(submitId);
		QString unique = QUuid::createUuid().toString() + QString::number(QDateTime().toTime_t());
		QString id = QString::number(qHash(unique));
		submitId.appendChild(doc.createTextNode("submission-" + id));

		// Machine name
		QDomElement fromAddress = doc.createElement("host-name");
		root.appendChild(fromAddress);
		fromAddress.appendChild(doc.createTextNode(QHostInfo::localHostName() + "." + QHostInfo::localDomainName()));

		// Results..

		// Send stream
		QTextStream resultStream(this);
		resultStream << doc.toString();

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
