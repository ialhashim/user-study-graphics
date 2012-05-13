#pragma once

#include <QQueue>

#include "project/GraphicsLibrary/Basic/Plane.h"
#include "project/GL/VBO/VBO.h"
#include "project/GUI/Viewer/libQGLViewer/QGLViewer/qglviewer.h"

using namespace qglviewer;
class QSegMesh;
class Controller;
class QManualDeformer;
class QFFD;
class VoxelDeformer;
class HiddenViewer;
class Offset;
#include "ui_DesignWidget.h"

enum ViewMode { VIEW, SELECTION, MODIFY };
enum SelectMode { SELECT_NONE, MESH, VERTEX, EDGE, FACE,
	CONTROLLER, CONTROLLER_ELEMENT, FFD_DEFORMER, VOXEL_DEFORMER };
enum TransformMode { NONE_MODE, TRANSLATE_MODE, ROTATE_MODE, SCALE_MODE };

class MyDesigner : public QGLViewer{
	Q_OBJECT

public:
	MyDesigner(Ui::DesignWidget * useDesignWidget, QWidget * parent = 0);

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
	
	// Draw stacking
	void drawStacking();
	bool isDrawStacking;

	// Offset stuff
	HiddenViewer	* hiddenViewer;
	Offset			* activeOffset;

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

	// Tool mode
	TransformMode transformMode;

	// Object in the scene
	QSegMesh * activeMesh;
	QSegMesh * activeObject();
	bool isEmpty();
	void setActiveObject(QSegMesh* newMesh);

	Controller * ctrl();

	// Deformer
	ManipulatedFrame * activeFrame;
	QManualDeformer * defCtrl;
	QFFD * activeDeformer;
	VoxelDeformer * activeVoxelDeformer;

	void loadMesh(QString fileName);

	// TEXT ON SCREEN
	QQueue<QString> osdMessages;
	QTimer *timer;

	// Mouse state
	bool isMousePressed;
	QPoint startMousePos2D;
	QPoint currMousePos2D;
	Vec startMouseOrigin, currMouseOrigin;
	Vec startMouseDir, currMouseDir;

	// For scale tool
	Vec3d startScalePos, currScalePos;
	Vec3d scaleDelta; // for visualization

	// Hack
	Ui::DesignWidget * designWidget;

	QString viewTitle;

	double loadedMeshHalfHight;

public slots:
	
	// Select buttons
	void selectPrimitiveMode();
	void selectCurveMode();
	void selectCameraMode();

	// Transform buttons
	void moveMode();
	void rotateMode();
	void scaleMode();
	void drawTool();

	void updateActiveObject();

	void print(QString message, long age = 1000);
	void dequeueLastMessage();

	void setActiveFFDDeformer();
	void setActiveVoxelDeformer();
	
	void cameraMoved();

	void updateOffset();

	void drawStackStateChanged ( int state );

	void reloadActiveMesh();

private:
	// DEBUG:
	std::vector<Vec> debugPoints;
	std::vector< std::pair<Vec,Vec> > debugLines;
	std::vector< Plane > debugPlanes;
	void drawDebug();

private:
	void drawCircleFade(double * color, double radius = 1.0);
	void drawMessage(QString message, int x = 10, int y = 10, Vec4d backcolor = Vec4d(0,0,0,0.25));
	void drawShadows();
	void drawViewChanger();
	double skyRadius;

	void transformPrimitive(bool modifySelect = true);
	void transformCurve(bool modifySelect = true);

	void toolMode();
	void selectTool();
	void clearButtons();

signals:
	void objectInserted();
	void objectDiscarded( QString );
	void objectUpdated();

};
