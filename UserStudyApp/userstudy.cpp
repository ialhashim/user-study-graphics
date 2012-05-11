#include "userstudy.h"
#include <phonon/phonon>
#include <QRadioButton>
#include <QFileInfo>

#include "Screens/MyDesigner.h"
MyDesigner * designer = NULL;

#include "Screens/project/GUI/MeshBrowser/QuickMeshViewer.h"

#include "Screens/Client.h"

UserStudyApp::UserStudyApp(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	// Disable all tabs
	for(int i = 0; i < ui.screens->count(); i++)
		ui.screens->setTabEnabled(i, false);
	
	// Connections
	connect(ui.nextButtonWelcome, SIGNAL(clicked()), this, SLOT(nextButtonWelcome()));
	connect(ui.nextButtonTutorial, SIGNAL(clicked()), this, SLOT(nextButtonTutorial()));
	connect(ui.nextButtonDesign, SIGNAL(clicked()), this, SLOT(nextButtonDesign()));
	connect(ui.nextButtonEvaluate, SIGNAL(clicked()), this, SLOT(nextButtonEvaluate()));

	connect(ui.sendResultButton, SIGNAL(clicked()), this, SLOT(sendResultButton()));
	connect(ui.saveResultButton, SIGNAL(clicked()), this, SLOT(saveResultButton()));

	// Create custom widgets
	designWidget = new Ui::DesignWidget();
	evalWidget = new Ui::EvaluateWidget();

	// Add custom widgets to each screen
	designWidget->setupUi(ui.designFrame);
	evalWidget->setupUi(ui.evaluateFrame);

	// Prepare tasks
	tasksFileName << "data/stool.obj";
	tasksFileName << "data/knot01.obj";

	// Show welcome screen
	//setScreen(WELCOME_SCREEN);
	nextButtonTutorial(); // to test Designer
	//nextButtonEvaluate(); // test Send data

	// Hack: avoid dealing with Unicode here..
	ui.checkmarkLabel->hide();
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
	Phonon::MediaSource media(":/UserStudyApp/tutorial.mp4");
	
	QApplication::restoreOverrideCursor();
	QCoreApplication::processEvents();

	ui.videoPlayer->play(media);
}

void UserStudyApp::nextButtonTutorial()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// Add viewer
	clearLayoutItems(designWidget->viewerAreaLayout);
	designer = new MyDesigner(designWidget);
	designWidget->viewerAreaLayout->addWidget(designer);

	// Connect selection buttons
	designer->connect(designWidget->selectPrimitiveButton, SIGNAL(clicked()), SLOT(selectPrimitiveMode()));
	designer->connect(designWidget->selectCurveButton, SIGNAL(clicked()), SLOT(selectCurveMode()));
	designer->connect(designWidget->selectCameraButton, SIGNAL(clicked()), SLOT(selectCameraMode()));

	// Connect transformation tools buttons
	designer->connect(designWidget->moveButton, SIGNAL(clicked()), SLOT(moveMode()));
	designer->connect(designWidget->rotateButton, SIGNAL(clicked()), SLOT(rotateMode()));
	designer->connect(designWidget->scaleButton, SIGNAL(clicked()), SLOT(scaleMode()));
	
	// Connect deformers buttons
	designer->connect(designWidget->ffdButton, SIGNAL(clicked()), SLOT(setActiveFFDDeformer()));
	designer->connect(designWidget->voxelButton, SIGNAL(clicked()), SLOT(setActiveVoxelDeformer()));
	designer->connect(designWidget->resetButton, SIGNAL(clicked()), SLOT(reloadActiveMesh()));

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
	tasksLabel[curr]->setStyleSheet(taskStyle(1));

	// Mark as done
	if(curr > 0)
	{
		tasksLabel[curr-1]->setStyleSheet(taskStyle(2));
		tasksLabel[curr-1]->setText(tasksLabel[curr-1]->text() + ui.checkmarkLabel->text());
	}

	designer->loadMesh(tasksFileName.front());
	tasksFileName.pop_front();
	
	QApplication::restoreOverrideCursor();
}

QString UserStudyApp::taskStyle( int state )
{
	QString padding = "padding: 5px;";

	switch(state)
	{
	case 0: return padding + "border: 2px solid grey; border-radius: 5px; background: rgb(200, 200, 200); color: LightGray;";
	case 1: return padding + "border: 3px solid red; border-radius: 5px; background: rgb(255, 170, 127);";
	case 2: return padding + "border: 2px solid green; border-radius: 5px; background: rgb(170, 255, 127);";
	}

	return "";
}

void UserStudyApp::nextButtonDesign()
{
	// Don't proceed while there are still tasks
	if(tasksFileName.size() > 0)
	{
		loadNextTask();
		return;
	}
	else
	{
		tasksLabel.last()->setStyleSheet(taskStyle(2));
		tasksLabel.last()->setText(tasksLabel.last()->text() + ui.checkmarkLabel->text());
		QCoreApplication::processEvents();
	}

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
	Client * client = new Client(this);
}

void UserStudyApp::saveResultButton()
{

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
