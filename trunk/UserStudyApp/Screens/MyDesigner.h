#pragma once

#include <QQueue>

#include "project/GL/VBO/VBO.h"
#include "project/GUI/Viewer/libQGLViewer/QGLViewer/qglviewer.h"

using namespace qglviewer;
class QSegMesh;
class Controller;
class QManualDeformer;

enum ViewMode { VIEW, SELECTION, MODIFY };
enum SelectMode { SELECT_NONE, MESH, VERTEX, EDGE, FACE,
	CONTROLLER, CONTROLLER_ELEMENT, FFD_DEFORMER, VOXEL_DEFORMER };

class MyDesigner : public QGLViewer{
	Q_OBJECT

public:
	MyDesigner(QWidget * parent = 0);

	void init();
	void setupCamera();
	void setupLights();
	void preDraw();
	void draw();
	void drawWithNames();
	void postDraw();
	void resetView();
	void beginUnderMesh();
	void endUnderMesh();
	void drawOSD();

	// VBOS
	QMap<QString, VBO> vboCollection;
	void updateVBOs();
	void drawObject();
	void drawObjectOutline();

	// Mouse & Keyboard stuff
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	void wheelEvent(QWheelEvent* e);
	void keyPressEvent(QKeyEvent *e);

	// SELECTION
	SelectMode selectMode;
	QVector<int> selection;
	void setSelectMode(SelectMode toMode);	
	void postSelection(const QPoint& point);

	// Object in the scene
	QSegMesh * activeMesh;
	QSegMesh * activeObject();
	bool isEmpty();
	void setActiveObject(QSegMesh* newMesh);

	Controller * ctrl();

	// Deformer
	ManipulatedFrame * activeFrame;
	QManualDeformer * defCtrl;

	void loadMesh(QString fileName);

	// TEXT ON SCREEN
	QQueue<QString> osdMessages;
	QTimer *timer;

public slots:
	void selectPrimitiveMode();
	void selectCurveMode();

	void updateActiveObject();

	void print(QString message, long age = 1000);
	void dequeueLastMessage();

private:
	void drawCircleFade(double * color, double radius = 1.0);
	void drawMessage(QString message, int x = 10, int y = 10, Vec4d backcolor = Vec4d(0,0,0,0.25));
	void drawShadows();
	double skyRadius;

signals:
	void objectInserted();
	void objectDiscarded( QString );

};
