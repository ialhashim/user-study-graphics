#include "userstudy.h"
#include <phonon/phonon>
#include <QRadioButton>

#include "Screens/MyDesigner.h"
MyDesigner * designer = NULL;

#include "Screens/project/GUI/MeshBrowser/QuickMeshViewer.h"

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

	// Show welcome screen
	setScreen(WELCOME_SCREEN);
	//nextButtonTutorial();
}

UserStudyApp::~UserStudyApp()
{

}

void UserStudyApp::nextButtonWelcome()
{
	ui.screens->setTabEnabled(TUTORIAL_SCREEN, true);
	ui.screens->setCurrentWidget(ui.screens->widget(TUTORIAL_SCREEN));

	// Load tutorial video
	Phonon::MediaSource media(":/UserStudyApp/tutorial.mp4");
	ui.videoPlayer->play(media);
}

void UserStudyApp::nextButtonTutorial()
{
	// Add viewer
	designer = new MyDesigner();
	designWidget->viewerAreaLayout->addWidget(designer);

	// Connect
	designer->connect(designWidget->selectPrimitiveButton, SIGNAL(clicked()), SLOT(selectPrimitiveMode()));
	designer->connect(designWidget->selectCurveButton, SIGNAL(clicked()), SLOT(selectCurveMode()));

	ui.screens->setTabEnabled(DESIGN_SCREEN, true);
	setScreen(DESIGN_SCREEN);
}

void UserStudyApp::nextButtonDesign()
{
	ui.screens->setTabEnabled(EVALUATE_SCREEN, true);
	setScreen(EVALUATE_SCREEN);

	int numSpecimen = 3;
	int numQuestions = 4;

	// Display specimens
	for(int i = 0; i < numSpecimen; i++)
	{
		// Add viewer
		QuickMeshViewer * mv = new QuickMeshViewer(this);
		mv->loadMesh("data/stool.obj");
		evalWidget->specimenLayout->addWidget(mv);
	}

	for(int q = 0; q < numQuestions; q++)
	{
		QLabel * questionLabel = new QLabel("Question "+QString::number(q+1));

		evalWidget->quesitonLayout->addWidget(questionLabel, q,0,1,1);

		for(int i = 0; i < numSpecimen; i++)
		{
			QFrame * stars = new QFrame;
			QHBoxLayout *layout = new QHBoxLayout;

			stars->setStyleSheet("QFrame{border:1px solid black}");

			for(int g = 1; g < 6; g++)
				layout->addWidget(new QRadioButton(QString::number(g), 0));

			stars->setLayout(layout);

			evalWidget->answerLayout->addWidget(stars, q,i,1,1);
		}
	}
}

void UserStudyApp::nextButtonEvaluate()
{
	ui.screens->setTabEnabled(FINISH_SCREEN, true);
	setScreen(FINISH_SCREEN);
}

void UserStudyApp::sendResultButton()
{

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
