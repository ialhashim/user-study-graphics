#include "userstudyserver.h"
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QDomDocument>
#include <iostream>

UserStudyServer::UserStudyServer(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	externalUrl = "http://www.cs.sfu.ca/~iaa7/personal/";

	connect(ui.startButton, SIGNAL(clicked()), SLOT(startServer()));
	connect(this, SIGNAL(gotIP()), SLOT(sendIP()));
	connect(this, SIGNAL(sentIP()), SLOT(startedServer()));
	connect(&server, SIGNAL(message(QString)), SLOT(Log(QString)));

	connect(ui.processResultButton, SIGNAL(clicked()), SLOT(processResult2()));

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

void UserStudyServer::processResult()
{
	// XML file
	QStringList fileNames =  QFileDialog::getOpenFileNames(0, "Result file", "", "Results Files (*.xml)"); 

	QFileInfo fi(fileNames.front());
	QDir parentDir(fi.absoluteDir().path());

	foreach(QString fileName, fileNames)
	{
		QFile xmlFile(fileName); xmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
		QString xmlString = xmlFile.readAll();
		xmlString.replace("data/Tasks/","");
		QDomDocument doc; doc.setContent(xmlString);

		// Submitter name
		QString submitName = fi.baseName();
		if(submitName.contains("_")){
			QStringList sl = submitName.split("_");
			submitName = sl.last();
		}

		// Create folder and file
		parentDir.mkdir(submitName);
		QString outDirPath = parentDir.path() + "/" + submitName + "/";
		QFile csv(outDirPath + submitName + "_data.csv");
		csv.open(QIODevice::WriteOnly | QIODevice::Text);
		csv.write("Mesh,Time,Stackability,Ratio,Edits,Shift\n");


		// Pick first object result
		QDomElement e = doc.firstChildElement().firstChildElement();
		while(!e.isNull())
		{
			if(e.tagName().endsWith(".obj"))
			{
				// Save controller for this object
				QDomElement controller = e.elementsByTagName("controller").at(0).toElement();
				QString controller_data = controller.text();
				QFile cfile(outDirPath + e.tagName() + ".txt"); 
				cfile.open(QIODevice::WriteOnly | QIODevice::Text);
				cfile.write(qPrintable(controller_data));
				cfile.close();

				QDomElement name = e.elementsByTagName("mesh-name").at(0).toElement();
				QDomElement time = e.elementsByTagName("edit-time").at(0).toElement();
				QDomElement stackability = e.elementsByTagName("stackability").at(0).toElement();
				QDomElement ratio = e.elementsByTagName("stackability-ratio").at(0).toElement();
				QDomElement num_edits = e.elementsByTagName("edit-num-operations").at(0).toElement();
				QDomElement stacking_shift = e.elementsByTagName("stacking-shift").at(0).toElement();

				QString line = QString("%1,%2,%3,%4,%5,%6\n").arg(name.text()).arg(time.text()).arg(stackability.text()).
					arg(ratio.text()).arg(num_edits.text()).arg(stacking_shift.text());

				csv.write(qPrintable(line));
			}

			e = e.nextSiblingElement();
		}

		// Close 
		csv.close();
	}
}

void UserStudyServer::processResult2()
{
	// XML file
	QStringList fileNames =  QFileDialog::getOpenFileNames(0, "Result file", "", "Results Files (*.xml)"); 

	QFileInfo fi(fileNames.front());
	QDir parentDir(fi.absoluteDir().path());

	// Save (time, stackability) pair for each shape
	QMap<QString, QFile*> time_stackability;
	QStringList shapes;
	shapes << "IKEA-cup.obj" << "chair-armrest.obj" << "chair.obj" << "cup-handle.obj" 
		<< "table-skirt.obj" << "table-middlebar.obj" << "stool.obj";
	parentDir.mkdir("time-stackability");
	QString outputDir = parentDir.path() + "/time-stackability/";
	foreach (QString shapeID, shapes)
	{
		time_stackability[shapeID] =  new QFile(outputDir + shapeID + ".csv");
		time_stackability[shapeID]->open(QIODevice::WriteOnly | QIODevice::Text);
		time_stackability[shapeID]->write("time,stackability\n");
	}


	foreach(QString fileName, fileNames)
	{
		QFile xmlFile(fileName); xmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
		QString xmlString = xmlFile.readAll(); xmlFile.close();
		xmlString.replace("data/Tasks/","");
		QDomDocument doc; doc.setContent(xmlString);

		// Submitter name
		QFileInfo fi_foo(fileName);
		QString submitName = fi_foo.baseName();
		if(submitName.contains("_")){
			QStringList sl = submitName.split("_");
			submitName = sl.last();
		}

		std::cout << qPrintable(submitName) << ": ";
		// Pick first object result
		QDomElement e = doc.firstChildElement().firstChildElement();
		while(!e.isNull())
		{
			if(e.tagName().endsWith(".obj"))
			{
				QDomElement name = e.elementsByTagName("mesh-name").at(0).toElement();
				QDomElement time = e.elementsByTagName("edit-time").at(0).toElement();
				QDomElement stackability = e.elementsByTagName("stackability").at(0).toElement();
				QDomElement ratio = e.elementsByTagName("stackability-ratio").at(0).toElement();
				QDomElement num_edits = e.elementsByTagName("edit-num-operations").at(0).toElement();
				QDomElement stacking_shift = e.elementsByTagName("stacking-shift").at(0).toElement();

				QString meshName = name.text();
				if (time_stackability.contains(meshName))
				{
					QString line = time.text() + ',' + stackability.text() + "\n";
					time_stackability[meshName]->write(qPrintable(line));

					std::cout << qPrintable(meshName) << ",";		
				}

			}

			e = e.nextSiblingElement();
		}

		std::cout << '\n';		

	}

	// Close 
	foreach (QString shapeID, shapes)
		time_stackability[shapeID]->close();
}
