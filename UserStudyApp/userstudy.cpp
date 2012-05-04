#include "userstudy.h"
#include <phonon/phonon>

// Screen widgets

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
	ui.screens->setTabEnabled(DESIGN_SCREEN, true);
	setScreen(DESIGN_SCREEN);
}

void UserStudyApp::nextButtonDesign()
{
	ui.screens->setTabEnabled(EVALUATE_SCREEN, true);
	setScreen(EVALUATE_SCREEN);
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
