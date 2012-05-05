#pragma once

#include "project/GUI/Viewer/libQGLViewer/QGLViewer/qglviewer.h"

using namespace qglviewer;
class QSegMesh;
class VBO;

class MyDesigner : public QGLViewer{
	Q_OBJECT

public:
	MyDesigner(QWidget * parent = 0);

	void init();
	void setupCamera();
	void setupLights();
	void preDraw();
	void draw();
	void postDraw();
	void resetView();
	void beginUnderMesh();
	void endUnderMesh();

	// VBOS
	QMap<QString, VBO*> vboCollection;
	void updateVBOs();
	void drawObject();
	void drawObjectOutline();

	// Mouse & Keyboard stuff
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	void wheelEvent(QWheelEvent* e);
	void keyPressEvent(QKeyEvent *e);

	// Object in the scene
	QSegMesh * activeMesh;
	QSegMesh * activeObject();
	void updateActiveObject();
	bool isEmpty();
	void setActiveObject(QSegMesh* newMesh);

	void loadMesh(QString fileName);

private:
	void drawCircleFade(double * color, double radius = 1.0);

	double skyRadius;

signals:
	void objectInserted();
	void objectDiscarded( QString );
};
