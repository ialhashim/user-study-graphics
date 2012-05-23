#include "userstudy.h"
#include <phonon/phonon>
#include <QRadioButton>
#include <QFileInfo>
#include <QFileDialog>
#include <QUuid>
#include <QHostInfo>
#include <QMessageBox>

#include "Screens/MyDesigner.h"
MyDesigner * designer = NULL;

#include "Screens/project/GUI/MeshBrowser/QuickMeshViewer.h"

#include "Screens/Client.h"

// Theora player
#include "Screens/videoplayer/gui_player/VideoWidget.h"
#include "Screens/videoplayer/gui_player/VideoToolbar.h"
VideoWidget * v = NULL;

bool isVideo = false;

class SleeperThread : public QThread
{public:static void msleep(unsigned long msecs){QThread::msleep(msecs);}};

UserStudyApp::UserStudyApp(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	// Disable all tabs
	for(int i = 0; i < ui.screens->count(); i++)
		ui.screens->setTabEnabled(i, false);
	
	// Connections
	connect(ui.screens, SIGNAL(currentChanged(int)), SLOT(screenChanged(int)));

	connect(ui.nextButtonWelcome, SIGNAL(clicked()), SLOT(nextButtonWelcome()));
	connect(ui.nextButtonTutorial, SIGNAL(clicked()), SLOT(nextButtonTutorial()));
	connect(ui.nextButtonDesign, SIGNAL(clicked()), SLOT(nextButtonDesign()));
	connect(ui.nextButtonEvaluate, SIGNAL(clicked()), SLOT(nextButtonEvaluate()));

//	connect(ui.sendResultButton, SIGNAL(clicked()), SLOT(sendResultButton()));
	connect(ui.saveResultButton, SIGNAL(clicked()), SLOT(saveResultButton()));

	// Create custom widgets
	designWidget = new Ui::DesignWidget();
	evalWidget = new Ui::EvaluateWidget();

	// Add custom widgets to each screen
	designWidget->setupUi(ui.designFrame);
	evalWidget->setupUi(ui.evaluateFrame);

	// Prepare tasks
	QStringList filenames;
	QMap<QString, double> targets;	
	std::ifstream inF("tasks.txt", std::ios::in);
	while (inF)
	{
		std::string filename;
		double target = 0.0;
		inF >> filename >> target;

		if (filename.empty()) continue;

		filenames << filename.c_str();		
		targets[filename.c_str()] = target;
	}

	// IKEA-cup, cup-handle, table-skirt, chair, one from [ stool, table-middlebar, chair-armrest ]
	srand ( time(NULL) );
	std::vector<int> idx;
	idx.push_back(0);
	idx.push_back(1);
	idx.push_back(2);
	idx.push_back(3);
	idx.push_back(rand() % 3 + 4);

	foreach (int i, idx)
	{
		QString selectedFile = filenames[i];
		tasksFiles.push_back(selectedFile);
		tasksTarget[selectedFile] = targets[selectedFile];
	}
	tasksFileName = tasksFiles; // Will be modified

	// Show welcome screen
	setScreen(WELCOME_SCREEN);
	//nextButtonTutorial(); // to test Designer
	//nextButtonEvaluate(); // test Send data
}

UserStudyApp::~UserStudyApp()
{

}

void UserStudyApp::nextButtonWelcome()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	ui.screens->setTabEnabled(TUTORIAL_SCREEN, true);
	ui.screens->setCurrentWidget(ui.screens->widget(TUTORIAL_SCREEN));

	// Load tutorial video
	if(!v)
	{
		v = new VideoWidget;
		VideoToolbar * vt = new VideoToolbar;

		isVideo = QFileInfo("data/tutorial.ogm").exists();

		if(isVideo)
		{
			v->loadVideo("data/tutorial.ogm"); 

			// Connect
			connect(vt->ui->playButton, SIGNAL(clicked()), v, SLOT(togglePlay()));
			connect(vt, SIGNAL(valueChanged(int)), v, SLOT(seekVideo(int)));
			connect(v, SIGNAL(setSliderValue(int)), vt->ui->slider, SLOT(setValue(int)));
		}

		ui.tutorialLayout->addWidget(v);
		ui.tutorialLayout->addWidget(vt);
	}

	printf("Video widget size: %d x %d \n", v->size().width(), v->size().height());

	QApplication::restoreOverrideCursor();
	QCoreApplication::processEvents();
}

void UserStudyApp::nextButtonTutorial()
{
	if(isVideo && v) v->stop();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// Hack: avoid dealing with Unicode here..
	ui.checkmarkLabel->hide();

	// Add viewer
	clearLayoutItems(designWidget->viewerAreaLayout);
	designer = new MyDesigner(designWidget);
	designWidget->viewerAreaLayout->addWidget(designer);

	// Connect selection buttons
	designer->connect(designWidget->selectPrimitiveButton, SIGNAL(clicked()), SLOT(selectPrimitiveMode()));
	designer->connect(designWidget->selectCurveButton, SIGNAL(clicked()), SLOT(selectCurveMode()));
	designer->connect(designWidget->selectCameraButton, SIGNAL(clicked()), SLOT(selectCameraMode()));
	designer->connect(designWidget->selectStackingButton, SIGNAL(clicked()), SLOT(selectStackingMode()));

	// Connect transformation tools buttons
	designer->connect(designWidget->moveButton, SIGNAL(clicked()), SLOT(moveMode()));
	designer->connect(designWidget->rotateButton, SIGNAL(clicked()), SLOT(rotateMode()));
	designer->connect(designWidget->scaleButton, SIGNAL(clicked()), SLOT(scaleMode()));
	
	// Connect deformers buttons
	designer->connect(designWidget->ffdButton, SIGNAL(clicked()), SLOT(setActiveFFDDeformer()));
	designer->connect(designWidget->voxelButton, SIGNAL(clicked()), SLOT(setActiveVoxelDeformer()));
	designer->connect(designWidget->undoButton, SIGNAL(clicked()), SLOT(Undo()));

	int numTasks = tasksFileName.size();

	for(int i = 0; i < numTasks; i++)
	{
		QString shortName = QFileInfo(tasksFileName.at(i)).baseName();
		tasksLabel.push_back(new QLabel(shortName));

		// Mark as unfinished
		tasksLabel[i]->setStyleSheet(taskStyle(0));

		ui.tasksLayout->addWidget(tasksLabel[i], 0, i);
	}

	QApplication::restoreOverrideCursor();

	loadNextTask();

	ui.screens->setTabEnabled(DESIGN_SCREEN, true);
	setScreen(DESIGN_SCREEN);
}

void UserStudyApp::loadNextTask()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// Mark as in progress
	int curr = tasksLabel.size() - tasksFileName.size();

	if(curr < tasksLabel.size())
		tasksLabel[curr]->setStyleSheet(taskStyle(1));

	// Mark as done
	if(curr > 0  && curr < tasksLabel.size())
	{
		tasksLabel[curr-1]->setStyleSheet(taskStyle(2));
		tasksLabel[curr-1]->setText(tasksLabel[curr-1]->text() + ui.checkmarkLabel->text());
	}

	if(curr > 0)
		taskResults[tasksFiles[curr-1]] = designer->collectData();
	
	if(tasksFileName.size())
	{
		QFileInfo info(tasksFileName.front());

		if(info.exists())
		{
			designer->loadMesh(tasksFileName.front(), tasksTarget[tasksFileName.front()]);
		}

		tasksFileName.pop_front();
	}

	QApplication::restoreOverrideCursor();
}

QString UserStudyApp::taskStyle( int state )
{
	QString padding = "padding: 5px;";

	switch(state)
	{
		case 0: return padding + "border: 2px solid grey; border-radius: 5px; background: rgb(200, 200, 200); color: LightGray;";
		case 1: return padding + "border: 3px solid Gold; border-radius: 5px; background: rgb(255, 255, 127);";
		case 2: return padding + "border: 2px solid green; border-radius: 5px; background: rgb(170, 255, 127);";
	}

	return "";
}

void UserStudyApp::nextButtonDesign()
{
	// Don't proceed while there are still tasks
	if(tasksFileName.size() > 0){
		loadNextTask();
		return;
	}
	else
	{
		// Last task
		taskResults[tasksFiles.last()] = designer->collectData();

		tasksLabel.last()->setStyleSheet(taskStyle(2));
		tasksLabel.last()->setText(tasksLabel.last()->text() + ui.checkmarkLabel->text());
		QCoreApplication::processEvents();
	}

	// Skip evaluation for now
	ui.screens->setTabEnabled(FINISH_SCREEN, true);
	setScreen(FINISH_SCREEN);
	return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	ui.screens->setTabEnabled(EVALUATE_SCREEN, true);
	setScreen(EVALUATE_SCREEN);

	int numSpecimen = 3;
	int numQuestions = 4;

	// Display specimens
	clearLayoutItems(evalWidget->specimenLayout);
	for(int i = 0; i < numSpecimen; i++)
	{
		// Add viewer
		QuickMeshViewer * mv = new QuickMeshViewer(this);
		mv->loadMesh("data/stool.obj");
		evalWidget->specimenLayout->addWidget(mv);
	}

	// Add questions
	clearLayoutItems(evalWidget->quesitonLayout);
	clearLayoutItems(evalWidget->answerLayout);
	for(int q = 0; q < numQuestions; q++)
	{
		QLabel * questionLabel = new QLabel("Question "+QString::number(q+1));

		evalWidget->quesitonLayout->addWidget(questionLabel, q,0,1,1);

		for(int i = 0; i < numSpecimen; i++)
		{
			QFrame * stars = new QFrame;
			QHBoxLayout *layout = new QHBoxLayout;

			stars->setStyleSheet("QFrame{border:1px solid black}");

			for(int g = 1; g < 6; g++){
				QRadioButton * radioButton = new QRadioButton(QString::number(g), 0);
				layout->addWidget(radioButton);
				if(g == 3) radioButton->setChecked(true);
			}

			stars->setLayout(layout);

			evalWidget->answerLayout->addWidget(stars, q,i,1,1);
		}
	}

	QApplication::restoreOverrideCursor();
}

void UserStudyApp::nextButtonEvaluate()
{
	ui.screens->setTabEnabled(FINISH_SCREEN, true);
	setScreen(FINISH_SCREEN);
}

void UserStudyApp::sendResultButton()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	Client * client = new Client(resultsString(), this);
}

void UserStudyApp::saveResultButton()
{
	QString filename = QFileDialog::getSaveFileName(this, "Data file", "", "Data file (*.data)");

	QFile File(filename);

	if ( File.open( QIODevice::WriteOnly | QIODevice::Text ) ) 
	{
		QTextStream TextStream(&File);
		TextStream << resultsString();
	}

	File.close();
}

QString UserStudyApp::resultsString()
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
	QMapIterator<QString, QMap<QString, QString> > i(taskResults);
	while (i.hasNext()) {
		i.next();

		QDomElement task = doc.createElement(i.key());
		root.appendChild(task);

		// Items of each task
		QMapIterator<QString, QString> j(i.value());
		while (j.hasNext()) {
			j.next();

			QDomElement taskItem = doc.createElement(j.key());
			task.appendChild(taskItem);

			taskItem.appendChild(doc.createTextNode(j.value()));
		}

	}

	return doc.toString();
}

QWidget * UserStudyApp::getScreen( SCREENS screenIndex )
{
	return ui.screens->widget(screenIndex);
}

void UserStudyApp::setScreen( SCREENS screenIndex )
{
	ui.screens->setTabEnabled(screenIndex, true);
	ui.screens->setCurrentWidget(getScreen(screenIndex));
}

void UserStudyApp::clearLayoutItems(QLayout * layout)
{
	if ( layout != NULL )
	{
		QLayoutItem* item;
		while ( ( item = layout->takeAt( 0 ) ) != NULL )
		{
			delete item->widget();
			delete item;
		}
		//delete w->layout();
	}
}

void UserStudyApp::screenChanged(int newScreenIndex)
{
	// Stop video in all cases
	if(v) v->stop();

	// Check if we changed to tutorial screen, if so enable video
	if(newScreenIndex == TUTORIAL_SCREEN)
	{
		if(v && v->stopped()) 
			v->start();
	}
}
