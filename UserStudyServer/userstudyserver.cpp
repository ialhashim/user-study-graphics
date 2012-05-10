#include "userstudyserver.h"
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QMessageBox>

UserStudyServer::UserStudyServer(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	externalUrl = "http://www.cs.sfu.ca/~iaa7/personal/";

	connect(ui.startButton, SIGNAL(clicked()), SLOT(startServer()));
	connect(this, SIGNAL(gotIP()), SLOT(sendIP()));
	connect(this, SIGNAL(sentIP()), SLOT(startedServer()));
	connect(&server, SIGNAL(message(QString)), SLOT(Log(QString)));

	Log("Click start");
	ui.serverNameLabel->setText("Server not started.");
}

UserStudyServer::~UserStudyServer()
{

}

void UserStudyServer::startServer()
{
	ui.logList->clear();

	// Port 5900 is VNC, which is only other open one here..
	if (!server.listen(QHostAddress::Any, 5900)) 
	{
		Log("Cannot start: " + server.errorString());
		return;
	}

	port = QString::number(server.serverPort());

	ui.serverNameLabel->setText("Starting..");
	Log("Started server.");

	getIP();
	Log("Getting IP address..");
}

void UserStudyServer::Log( QString message )
{
	ui.logList->addItem(message);
}

void UserStudyServer::getIP()
{
	QNetworkAccessManager * am = new QNetworkAccessManager(this);
	QNetworkReply *reply = am->get(QNetworkRequest(QUrl(externalUrl + "ip.php")));
	connect(am, SIGNAL(finished(QNetworkReply*)), SLOT(assignIP(QNetworkReply*)));
}

void UserStudyServer::sendIP()
{
	QNetworkAccessManager * am = new QNetworkAccessManager(this);
	QString ip_port = externalUrl + "setip.php?ip=" + ipAddress + "&port="+port;
	QNetworkReply *reply = am->get(QNetworkRequest(QUrl(ip_port)));
	connect(am, SIGNAL(finished(QNetworkReply*)), SLOT(broadcastIP(QNetworkReply*)));
}

void UserStudyServer::assignIP(QNetworkReply *reply)
{
	if(reply->error() == QNetworkReply::NoError)
		ipAddress = reply->readAll().trimmed();
	else
		QMessageBox::information(this, "Network error", "Could not get your external IP address.");
	reply->deleteLater();

	Log("Your address:  " + ipAddress);

	emit(gotIP());
}

void UserStudyServer::broadcastIP( QNetworkReply *reply )
{
	QString replay;

	if(reply->error() == QNetworkReply::NoError)
		replay = reply->readAll().trimmed();
	else
		QMessageBox::information(this, "Network error", "Could not send your external IP address.");
	reply->deleteLater();

	Log("Send IP:  " + replay);

	emit(sentIP());
}

void UserStudyServer::startedServer()
{
	Log("All OK, now listening..");
	ui.serverNameLabel->setText("Listening at " + ipAddress + "...");
}
