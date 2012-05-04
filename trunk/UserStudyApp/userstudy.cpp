#include "userstudy.h"
#include <phonon/phonon>

UserStudyApp::UserStudyApp(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	// Disable all tabs
	for(int i = 0; i < ui.screens->count(); i++)
		ui.screens->setTabEnabled(i, false);
	
	ui.screens->setTabEnabled(WELCOME_SCREEN, true);

	// Connections
	connect(ui.nextButtonWelcome, SIGNAL(clicked()), this, SLOT(nextButtonWelcome()));
	connect(ui.nextButtonTutorial, SIGNAL(clicked()), this, SLOT(nextButtonTutorial()));
	connect(ui.nextButtonDesign, SIGNAL(clicked()), this, SLOT(nextButtonDesign()));
	connect(ui.nextButtonEvaluate, SIGNAL(clicked()), this, SLOT(nextButtonEvaluate()));

	connect(ui.sendResultButton, SIGNAL(clicked()), this, SLOT(sendResultButton()));
	connect(ui.saveResultButton, SIGNAL(clicked()), this, SLOT(saveResultButton()));
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
	ui.screens->setCurrentWidget(ui.screens->widget(DESIGN_SCREEN));
}

void UserStudyApp::nextButtonDesign()
{
	ui.screens->setTabEnabled(EVALUATE_SCREEN, true);
	ui.screens->setCurrentWidget(ui.screens->widget(EVALUATE_SCREEN));
}

void UserStudyApp::nextButtonEvaluate()
{
	ui.screens->setTabEnabled(FINISH_SCREEN, true);
	ui.screens->setCurrentWidget(ui.screens->widget(FINISH_SCREEN));
}

void UserStudyApp::sendResultButton()
{

}

void UserStudyApp::saveResultButton()
{

}
