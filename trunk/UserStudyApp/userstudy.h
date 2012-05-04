#ifndef USERSTUDY_H
#define USERSTUDY_H

#include <QtGui/QMainWindow>
#include "ui_userstudyMainWindow.h"

class UserStudyApp : public QMainWindow
{
	Q_OBJECT

public:
	UserStudyApp(QWidget *parent = 0, Qt::WFlags flags = 0);
	~UserStudyApp();

public slots:
	void nextButtonWelcome();
	void nextButtonTutorial();
	void nextButtonDesign();
	void nextButtonEvaluate();

	void sendResultButton();
	void saveResultButton();

private:
	Ui::UserStudyAppClass ui;

};

enum SCREENS{ WELCOME_SCREEN, TUTORIAL_SCREEN, 
	DESIGN_SCREEN, EVALUATE_SCREEN, FINISH_SCREEN };

#endif // USERSTUDY_H
