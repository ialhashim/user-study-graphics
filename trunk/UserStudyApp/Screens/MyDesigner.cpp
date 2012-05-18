#include "project/GraphicsLibrary/Basic/Circle.h"
#include "project/GraphicsLibrary/Mesh/QSegMesh.h"
#include "project/GL/VBO/VBO.h"
#include "project/Stacker/Controller.h"
#include "project/Stacker/Primitive.h"
#include "project/Stacker/QManualDeformer.h"
#include "project/MathLibrary/Deformer/QFFD.h"
#include "project/MathLibrary/Deformer/VoxelDeformer.h"
#include "project/Stacker/HiddenViewer.h"
#include "project/Stacker/Offset.h"
#include "project/Utility/SimpleDraw.h"
#include "project/Utility/ColorMap.h"

#include "MyDesigner.h"
#define GL_MULTISAMPLE 0x809D

#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QPushButton>
QFontMetrics * fm;

// Misc.
#include "sphereDraw.h"
#include "drawRoundRect.h"
#include "drawPlane.h"
#include "drawCube.h"

#include "study_global.h"

MyDesigner::MyDesigner( Ui::DesignWidget * useDesignWidget, QWidget * parent /*= 0*/ ) : QGLViewer(parent)
{
	this->designWidget = useDesignWidget;

	defCtrl = NULL;
	activeDeformer = NULL;
	activeVoxelDeformer = NULL;
	activeOffset = NULL;

	selectMode = SELECT_NONE;
	transformMode = NONE_MODE;
	skyRadius = 1.0;
	activeMesh = NULL;
	isMousePressed = false;
	loadedMeshHalfHight = 1.0;

	fm = new QFontMetrics(QFont());

	connect(this, SIGNAL(objectInserted()), SLOT(updateGL()));

	activeFrame = new ManipulatedFrame();
	setManipulatedFrame(activeFrame);

	// TEXT ON SCREEN
	timerScreenText = new QTimer(this);
	connect(timerScreenText, SIGNAL(timeout()), SLOT(dequeueLastMessage()));

	connect(camera()->frame(), SIGNAL(manipulated()), SLOT(cameraMoved()));

	this->setMouseTracking(true);

	viewTitle = "View";

	// Offset computation
	hiddenViewer = new HiddenViewer();
	QDockWidget * hiddenDock = new QDockWidget("HiddenViewer");
	hiddenDock->setWidget (hiddenViewer);
	designWidget->specialWidgetLayout->addWidget(hiddenDock);
	hiddenDock->setFloating(true);
	hiddenDock->setWindowOpacity(0.0);

	int x = qApp->desktop()->availableGeometry().width();
	hiddenDock->move(QPoint(x - hiddenDock->width(),0)); //Move the hidden dock to the top right conner

	isDrawStacking = false;

	// Connect to show / hide stacking
	connect(designWidget->showStacking, SIGNAL(stateChanged(int)), SLOT(drawStackStateChanged(int)));

	// Stacking direction, turn off translation
	AxisPlaneConstraint * c = new AxisPlaneConstraint;
	c->setTranslationConstraintType(AxisPlaneConstraint::FORBIDDEN);
	stackingDir.setConstraint(c);
	stackingDir.setSpinningSensitivity(100.0);
}

void MyDesigner::init()
{
	// Lights
	setupLights();

	// Camera
	setupCamera();

	// Material
	float mat_ambient[] = {0.1745f, 0.01175f, 0.01175f, 1.0f};
	float mat_diffuse[] = {0.65f, 0.045f, 0.045f, 1.0f};
	float mat_specular[] = {0.09f, 0.09f, 0.09f, 1.0f};
	float high_shininess = 100;

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, high_shininess);

	// Redirect keyboard
	setShortcut( HELP, Qt::CTRL+Qt::Key_H );
	setShortcut( STEREO, Qt::SHIFT+Qt::Key_S );

	camera()->frame()->setSpinningSensitivity(100.0);

	if (!VBO::isVBOSupported()) std::cout << "VBO is not supported." << std::endl;
}

void MyDesigner::setupLights()
{
	GLfloat lightColor[] = {0.9f, 0.9f, 0.9f, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
}

void MyDesigner::setupCamera()
{
	camera()->setSceneRadius(10.0);
	camera()->showEntireScene();
	camera()->setUpVector(Vec(0,0,1));
	camera()->setPosition(Vec(2,-2,2));
	camera()->lookAt(Vec());
}

void MyDesigner::cameraMoved()
{
	if(camera()->type() != Camera::PERSPECTIVE)
	{
		Vec camPos = camera()->position();
		double minAxis = Min(abs(camPos.x), Min(abs(camPos.y), abs(camPos.z)));

		if(minAxis > 0.25)
		{
			viewTitle = "View";
			camera()->setType(Camera::PERSPECTIVE);
			updateGL();
		}
	}
}

void MyDesigner::preDraw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	skyRadius = Max(4, Min(20, 1.5*Max(camera()->position().x, Max(camera()->position().y, camera()->position().z))));
	setSceneRadius(skyRadius);

	camera()->loadProjectionMatrix(); // GL_PROJECTION matrix
	camera()->loadModelViewMatrix(); // GL_MODELVIEW matrix

	// Anti aliasing 
	glEnable(GL_MULTISAMPLE);
	glEnable (GL_LINE_SMOOTH);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	// Background
	setBackgroundColor(QColor(208,212,240));

	// Draw fancy effects:
	drawSolidSphere(skyRadius,30,30, true, true); // Sky dome


	beginUnderMesh();
	double floorOpacity = 0.25; 
	if(camera()->position().z < loadedMeshHalfHight) floorOpacity = 0.05;
	drawCircleFade(Vec4d(0,0,0,floorOpacity), 4); // Floor
	drawShadows();
	endUnderMesh();

	glClear(GL_DEPTH_BUFFER_BIT);
}

void MyDesigner::drawShadows()
{
	if(!activeMesh || camera()->position().z < loadedMeshHalfHight || isDrawStacking) return;

	// N64 method :)
	/*glDisable(GL_DEPTH_TEST);
	// Draw shadows 
	QMapIterator<QString, VBO> i(vboCollection);
	while (i.hasNext()) {
		i.next();

		QSurfaceMesh * mesh = activeMesh->getSegment(i.key());
		double scaleX = (mesh->bbmax.x() - mesh->bbmin.x());
		double scaleY = (mesh->bbmax.y() - mesh->bbmin.y());
		double z = mesh->center.z() - (0.5*(mesh->bbmax.z() - mesh->bbmin.z()));

		glPushMatrix();
		glTranslated(mesh->center.x(), mesh->center.y(), 0.01);
		glScaled(scaleX, scaleY, 1.0);
		double opacity = 0.25 * (1 - ((z + (objectHeight*0.5)) / objectHeight));
		drawCircleFade(Vec4d(0,0,0,Max(0,Min(0.5,opacity))), 1); // Shadow
		glPopMatrix();
	}
	glEnable(GL_DEPTH_TEST);*/
		
	// Compute shadow matrix
	GLfloat floorShadow[4][4];
	GLfloat groundplane[4];
	findPlane(groundplane, Vec3f(0,0,0), Vec3f(-1,0,0), Vec3f(-1,-1,0));
	GLfloat lightpos[4] = {0.0,0.0,8,1};
	shadowMatrix(floorShadow,groundplane, lightpos);

	glScaled(1.1,1.1,0);
	glPushMatrix();
	glMultMatrixf((GLfloat *) floorShadow); /* Project the shadow. */
	glColor4d(0,0,0,0.04);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	for (QMap<QString, VBO>::iterator i = vboCollection.begin(); i != vboCollection.end(); ++i)
	{
		i->render_depth();
	}
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	
}

void MyDesigner::draw()
{
	// No object to draw
	if (isEmpty()) return;

	// The main object
	drawObject();
		
	// deformers
	if(activeDeformer) activeDeformer->draw();
	if(activeVoxelDeformer) activeVoxelDeformer->draw();

	// Draw debug geometries
	activeObject()->drawDebug();

	// Draw stacked
	if(selectMode == STACK_DIR_MODE) isDrawStacking = true;
	if(activeOffset) drawStacking();

	// Draw controller and primitives
	if(ctrl() && (selectMode == CONTROLLER || selectMode == CONTROLLER_ELEMENT)) 
		ctrl()->draw();

	glEnable(GL_BLEND);

	drawDebug();
}

void MyDesigner::drawTool()
{
	if(!selection.size()) return;

	double toolScale = 0.4;

	switch(transformMode){
	case NONE_MODE: break;
	case TRANSLATE_MODE: 
		{
			glPushMatrix();
			ManipulatedFrame * m = manipulatedFrame();
			const GLdouble * mat = m->matrix();
			glMultMatrixd(mat);
			glColor3f(1,1,0);
			glRotated(-90,0,0,1);
			drawAxis(toolScale);
			SimpleDraw::DrawSolidBox(Vec3d(0),0.1,0.1,0.1, 1,1,0,1);
			glPopMatrix();
			break;
		}
	case ROTATE_MODE:
		{
			glDisable(GL_LIGHTING);
			glPushMatrix();
			glMultMatrixd(defCtrl->getFrame()->matrix());

			Circle c(Vec3d(0,0,0), Vec3d(0,0,1),toolScale * 0.75);
			c.draw(1,Vec4d(1,1,0,1)); c.draw(3,Vec4d(0,0,0,1));
			glRotated(90,0,1,0);
			c.draw(1,Vec4d(1,1,0,1)); c.draw(3,Vec4d(0,0,0,1));
			glRotated(90,1,0,0);
			c.draw(1,Vec4d(1,1,0,1)); c.draw(3,Vec4d(0,0,0,1));
			
			glDisable(GL_DEPTH_TEST);
			glColor4d(1,1,0,0.1);
			drawSolidSphere(toolScale* 0.75, 20,20);
			glEnable(GL_DEPTH_TEST);

			glPopMatrix();
			glEnable(GL_LIGHTING);
			break;
		}
	case SCALE_MODE:
		{
			glPushMatrix();
			glMultMatrixd(defCtrl->getFrame()->matrix());

			Vec3d delta = scaleDelta;

			toolScale *= 0.25;

			glEnable(GL_LIGHTING);
			glColor4d(1,1,0,1);
			if(delta.x() != 1) SimpleDraw::DrawArrowDirected(Vec3d(0), Vec3d(1,0,0),0.2);
			if(delta.y() != 1) SimpleDraw::DrawArrowDirected(Vec3d(0), Vec3d(0,-1,0),0.2);
			if(delta.z() != 1) SimpleDraw::DrawArrowDirected(Vec3d(0), Vec3d(0,0,1),0.2);
			glDisable(GL_LIGHTING);

			delta.x() = abs(delta.x());
			delta.y() = abs(delta.y());
			delta.z() = abs(delta.z());
			glScaled(delta.x(), delta.y(), delta.z());

			Circle c1(Vec3d(toolScale*0.75,-toolScale*0.75,0), Vec3d(0,0,1),toolScale, 4);
			c1.drawFilled(Vec4d(1,1,0,0.2), 2, Vec4d(0,0,0,1));

			Circle c2(Vec3d(toolScale*0.75,0,toolScale*0.75), Vec3d(0,1,0),toolScale, 4);
			c2.drawFilled(Vec4d(1,1,0,0.2), 2, Vec4d(0,0,0,1));

			Circle c3(Vec3d(0,-toolScale*0.75,toolScale*0.75), Vec3d(1,0,0),toolScale, 4);
			c3.drawFilled(Vec4d(1,1,0,0.2), 2, Vec4d(0,0,0,1));

			glPopMatrix();
			glEnable(GL_LIGHTING);
			break;
		}
	}
}

void MyDesigner::drawDebug()
{
	glDisable(GL_LIGHTING);
	
	// DEBUG: Draw designer debug
	glBegin(GL_POINTS);
		glColor4dv(Vec4d(1,0,0,1));
		foreach(Vec p, debugPoints) glVertex3dv(p);
	glEnd();

	glBegin(GL_LINES);
	glColor4dv(Vec4d(0,0,1,1));
	for(std::vector< std::pair<Vec,Vec> >::iterator 
		line = debugLines.begin(); line != debugLines.end(); line++)
	{glVertex3dv(line->first); glVertex3dv(line->second);}
	glEnd();

	foreach(Plane p, debugPlanes) p.draw();

	glEnable(GL_LIGHTING);
}

void MyDesigner::drawObject()
{
	if (VBO::isVBOSupported()){
		updateVBOs();

		glPushAttrib (GL_POLYGON_BIT);
		glEnable (GL_CULL_FACE);

		drawObjectOutline();

		/** Draw front-facing polygons as filled */
		glPolygonMode (GL_FRONT, GL_FILL);
		glCullFace (GL_BACK);
		
		/* Draw solid object */
		for (QMap<QString, VBO>::iterator i = vboCollection.begin(); i != vboCollection.end(); ++i)
		{
			if(viewTitle != "View")
				i->render_regular(false, true);
			else
				i->render();
		}

		/* GL_POLYGON_BIT */
		glPopAttrib ();
	} 
	else{// Fall back
		activeObject()->simpleDraw();
	}
}

void MyDesigner::drawStacking()
{
	int stackCount = 3;

	double S = activeMesh->val["stackability"];
	Vec3d delta = activeMesh->vec["stacking_shift"];

	double stackingOpacity = 0.3;
	if(isMousePressed) stackingOpacity = 0.1;

	
	// Draw stacking direction
	glEnable(GL_CULL_FACE);

	if(isDrawStacking)
	{
		glPushMatrix();

		// Stacking color
		glColor4dv(Color(0.45,0.72,0.43,stackingOpacity));
	

		// Top
		glTranslated(delta[0],delta[1],delta[2]);
		activeObject()->simpleDraw(false);

		// Bottom
		delta *= -2;
		glTranslated(delta[0],delta[1],delta[2]);
		activeObject()->simpleDraw(false);


		glPopMatrix();


		glDisable(GL_DEPTH_TEST);

		// Stacking direction
		if(selectMode == STACK_DIR_MODE)
			glColor4dv(Color(1, 1, 0, 1));
		else
			glColor4dv(Color(1, 1, 0, stackingOpacity * 0.75));

		Vec dir = stackingDir.rotation() * Vec(0,0,1);
		SimpleDraw::DrawArrowDirected(Vec3d(0.0), Vec3d(dir[0],dir[1],dir[2]));
		glEnable(GL_DEPTH_TEST);
	
	}

	glDisable(GL_CULL_FACE);
}

void MyDesigner::drawObjectOutline()
{
	/** Draw back-facing polygons as red lines	*/
	/* Disable lighting for outlining */
	glPushAttrib (GL_LIGHTING_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable (GL_LIGHTING);

	glPolygonMode(GL_BACK, GL_LINE);
	glCullFace (GL_FRONT);

	glDepthFunc (GL_LEQUAL);
	glLineWidth (Max(0.5, Min(5,skyRadius / 2.0)));

	/* Draw wire object */
	glColor3f (0.0f, 0.0f, 0.0f);
	for (QMap<QString, VBO>::iterator i = vboCollection.begin(); i != vboCollection.end(); ++i)
		i->render_depth();
	
	/* GL_LIGHTING_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT */
	glPopAttrib ();
}

void MyDesigner::postDraw()
{
	beginUnderMesh();

	// Draw grid
	double gridOpacity = 0.1; 
	if(camera()->position().z < loadedMeshHalfHight) gridOpacity = 0.05;
	glColor4d(0,0,0,gridOpacity);
	glLineWidth(1.0);
	drawGrid(2, Min(10, Max(100, camera()->position().x / skyRadius)));

	// Draw axis
	glPushMatrix();
	glTranslated(-2,-2,0);
	drawAxis(0.8);
	glPopMatrix();

	endUnderMesh();

	glClear(GL_DEPTH_BUFFER_BIT);

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	drawTool();
	drawViewChanger();
	drawVisualHints(); // Revolve Around Point, line when camera rolls, zoom region
	glPopAttrib();

	drawOSD();
}

void MyDesigner::drawViewChanger()
{
	int viewport[4];
	int scissor[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetIntegerv(GL_SCISSOR_BOX, scissor);

	const int size = 90;
	glViewport(width() - size, 0,size,size);
	glScissor(width() - size, 0,size,size);
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -10, 10);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRotated(-45,1,0,0);glRotated(45,0,0,1);glRotated(180,0,0,1);

	glColor4d(1,1,0.5,0.75); SimpleDraw::DrawCube(Vec3d(0,0,0),0.75f);

	double scale = 1.0;
	
	std::vector<bool> hover(7, false);

	int x = currMousePos2D.x(), y = currMousePos2D.y();
	if(x > width() - size && y > height() - size)
	{
		QPoint p(abs(x - width() + size), abs(y - height() + size));
		QSize box(QSize(size * 0.25, size * 0.25));

		QRect top(QPoint(30,15), box);
		QRect front(QPoint(52,52), box);
		QRect side(QPoint(12,52), box);
		QRect corner(QPoint(32,45), box);
		QRect backside(QPoint(64,18), box);
		QRect back(QPoint(11,20), box);
		QRect bottom(QPoint(30,60), box);

		if(top.contains(p)) hover[0] = true;
		if(front.contains(p)) hover[1] = true;
		if(side.contains(p)) hover[2] = true;
		if(corner.contains(p)) hover[3] = true;
		if(backside.contains(p)) hover[4] = true;
		if(back.contains(p)) hover[5] = true;
		if(bottom.contains(p)) hover[6] = true;
	}

	glColor4d(1,1,0,1);
	Vec3d top (0,0,0.5);if(hover[0]) glColor4d(1,0,0,1);
	SimpleDraw::DrawArrowDirected(top, top, scale,true,true);glColor4d(1,1,0,1);
	Vec3d front (0,0.5,0);if(hover[1]) glColor4d(1,0,0,1);
	SimpleDraw::DrawArrowDirected(front, front, scale,true,true);glColor4d(1,1,0,1);
	Vec3d side (0.5,0,0);if(hover[2]) glColor4d(1,0,0,1);
	SimpleDraw::DrawArrowDirected(side, side, scale,true,true);glColor4d(1,1,0,1);
	Vec3d corner (0.5,0.5,0.5);if(hover[3]) glColor4d(1,0,0,1);
	SimpleDraw::DrawArrowDirected(corner, corner, scale,true,true);glColor4d(1,1,0,1);
	Vec3d backside (-0.5,0,0);if(hover[4]) glColor4d(1,0,0,1);
	SimpleDraw::DrawArrowDirected(backside, backside, scale,true,true);glColor4d(1,1,0,1);
	Vec3d back (0,-0.5,0);if(hover[5]) glColor4d(1,0,0,1);
	SimpleDraw::DrawArrowDirected(back, back, scale,true,true); glColor4d(1,1,0,1);
	Vec3d bottom (0,0,-0.6);if(hover[6]) glColor4d(1,0,0,1);
	SimpleDraw::DrawArrowDirected(bottom, bottom, scale,true,true);glColor4d(1,1,0,1);

	glMatrixMode(GL_PROJECTION);glPopMatrix();
	glMatrixMode(GL_MODELVIEW);glPopMatrix();

	glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
	glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
}

void MyDesigner::drawOSD()
{
	QStringList selectModeTxt, toolModeTxt;
	selectModeTxt << "Camera" << "Mesh" << "Vertex" << "Edge" 
		<< "Face" << "Primitive" << "Curve" << "FFD" << "Voxel" << "Stacking Direction";
	toolModeTxt << "None" << "Move" << "Rotate" << "Scale";

	int paddingX = 15, paddingY = 5;

	int pixelsHigh = fm->height();
	int lineNum = 0;

	#define padY(l) paddingX, paddingY + (l++ * pixelsHigh*2.5) + (pixelsHigh * 3)

	/* Mode text */
	drawMessage("Select mode: " + selectModeTxt[this->selectMode], padY(lineNum));

	if(selectMode == CONTROLLER || selectMode == CONTROLLER_ELEMENT)
	{
		QString primId = "None";
		if(ctrl()->getSelectedPrimitive())
			primId = ctrl()->getSelectedPrimitive()->id;
		drawMessage("Primitive - " + primId, padY(lineNum), Vec4d(1.0,1.0,0,0.25));
	}

	if(selectMode == CONTROLLER_ELEMENT && ctrl()->getSelectedPrimitive())
	{
		QString curveId = "None";
		if(ctrl()->getSelectedPrimitive()->selectedCurveId >= 0)
			curveId = QString::number(ctrl()->getSelectedPrimitive()->selectedCurveId);
		drawMessage("Curve - " + curveId, padY(lineNum), Vec4d(0,1.0,0,0.25));
	}

	if(transformMode != NONE_MODE)
	{
		drawMessage("Tool: " + toolModeTxt[transformMode], padY(lineNum), Vec4d(1.0,1.0,0,0.25));
	}

	// View changer title
	qglColor(QColor(255,255,180,200));
	renderText(width() - (90*0.5) - (fm->width(viewTitle)*0.5), height() - (fm->height()*0.3),viewTitle);

	// Textual log messages
	for(int i = 0; i < osdMessages.size(); i++){
		int margin = 20; //px
		int x = margin;
		int y = (i * QFont().pointSize() * 1.5f) + margin;

		qglColor(Qt::white);
		renderText(x, y, osdMessages.at(i));
	}

	//if(isDrawStacking)
		drawStackability();

	setForegroundColor(QColor(0,0,0));
	QGLViewer::postDraw();
}

void MyDesigner::drawStackability()
{
	double ratio = stackingRatio;

	QString currStack = QString::number(Min(100, int(ratio * 100)));

	QString message = QString("Stackability target %1%").arg(currStack);

	if(ratio > 0.99)
		message += " - Excellent!";
	else if(ratio > 0.75)
		message += " - Very Good";
	else if(ratio > 0.5)
		message += " - Good";

	uchar rgb[3];	
	ColorMap::jetColorMap(rgb, 3 - ratio, 0.0, 3.0);
	QColor c = QColor::fromRgb(rgb[0],rgb[1],rgb[2]);

	double x = 15;
	double y = height() - fm->height() * 3;

	Vec4d currColor(c.redF(), c.greenF(), c.blueF(), 1.0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	this->startScreenCoordinatesSystem();
	glEnable(GL_BLEND);
	drawRoundRect(x, y, 150, 15, Vec4d(0,0,0,0.05), 0);
	drawRoundRect(x, y, Min(150, ratio * 150), 15, currColor, 0);
	this->stopScreenCoordinatesSystem();

	glColor4dv(Vec4d(0,0,0,0.75));
	drawText( x, y - fm->height()*2, message);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

}

void MyDesigner::drawMessage(QString message, int x, int y, Vec4d backcolor, Vec4d frontcolor)
{
	int pixelsWide = fm->width(message);
	int pixelsHigh = fm->height() * 1.5;

	int margin = 20;

	this->startScreenCoordinatesSystem();
	glEnable(GL_BLEND);
	drawRoundRect(x, y,pixelsWide + margin, pixelsHigh * 1.5, backcolor, 5);
	this->stopScreenCoordinatesSystem();

	glColor4dv(frontcolor);
	drawText( x + margin * 0.5, y - (pixelsHigh * 0.5), message);
}

void MyDesigner::drawCircleFade(double * color, double radius)
{
	//glDisable(GL_LIGHTING);
	Circle c(Vec3d(0,0,0), Vec3d(0,0,1), radius);
	std::vector<Point> pnts = c.getPoints();
	glBegin(GL_TRIANGLE_FAN);
	glColor4d(color[0],color[1],color[2],color[3]); glVertex3dv(c.getCenter());
	glColor4d(color[0],color[1],color[2],0); 
	foreach(Point p, c.getPoints())	glVertex3dv(p); 
	glVertex3dv(c.getPoints().front());
	glEnd();
	//glEnable(GL_LIGHTING);
}

void MyDesigner::updateVBOs()
{
	QSegMesh * mesh = activeObject();

	if(mesh && mesh->isReady)
	{
		// Create VBO for each segment if needed
		for (int i=0;i<(int)mesh->nbSegments();i++)
		{			
			QSurfaceMesh* seg = mesh->getSegment(i);
			QString objId = seg->objectName();

			if (VBO::isVBOSupported() && !vboCollection.contains(objId))
			{
				Surface_mesh::Vertex_property<Point>  points   = seg->vertex_property<Point>("v:point");
				Surface_mesh::Vertex_property<Point>  vnormals = seg->vertex_property<Point>("v:normal");
				Surface_mesh::Vertex_property<Color>  vcolors  = seg->vertex_property<Color>("v:color");

				if(!seg->triangles.size()) 
					seg->fillTrianglesList();

				// Create VBO 
				vboCollection[objId] = VBO( seg->n_vertices(), points.data(), vnormals.data(), vcolors.data(), seg->triangles );		
			}
		}
	}
}

void MyDesigner::updateActiveObject()
{
	vboCollection.clear();

	emit(objectUpdated());
}

QSegMesh * MyDesigner::activeObject()
{
	return activeMesh;
}

bool MyDesigner::isEmpty()
{
	return activeMesh == NULL;
}

void MyDesigner::resetView()
{
	setupCamera();
	camera()->setSceneRadius(activeMesh->radius);
	camera()->showEntireScene();
}

void MyDesigner::updateOffset()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	Vec dir = stackingDir.rotation() * Vec(0,0,1);
	activeOffset->computeStackability(Vec3d(dir[0],dir[1],dir[2]));
	QApplication::restoreOverrideCursor();

	// Compute ratio
	double curr = activeMesh->val["stackability"];
	double target = activeMesh->val["target-stackability"];
	stackingRatio = (curr / target);

	if(stackingRatio > 0.99) 
		designWidget->nextTaskMessage->setVisible(true);
	else
		designWidget->nextTaskMessage->setVisible(false);
}

void MyDesigner::loadMesh( QString fileName, double targetS /*= 0.5*/ )
{
	clearButtons();

	selection.clear();

	QString originalFileName = fileName;

	if(fileName.isEmpty() || !QFileInfo(fileName).exists()) return;

	QString newObjId = QFileInfo(fileName).fileName();
	newObjId.chop(4);

	QSegMesh * loadedMesh = new QSegMesh();

	// Reading QSegMesh
	loadedMesh->read(fileName);

	// Save filename
	loadedMesh->ptr["filename"] = new QString(fileName);

	// Set global ID for the mesh and all its segments
	loadedMesh->setObjectName(newObjId);
	
	// Setup controller file name
	fileName.chop(3);fileName += "ctrl";

	if(QFileInfo(fileName).exists())
	{
		// Load controller
		loadedMesh->ptr["controller"] = new Controller(loadedMesh, true, fileName);
		Controller * ctrl = (Controller *)loadedMesh->ptr["controller"];

		fileName.chop(4);fileName += "grp";
		if(QFileInfo(fileName).exists())
		{
			std::ifstream inF(qPrintable(fileName), std::ios::in);
			ctrl->loadGroups(inF);
			inF.close();
		}
	}
	else
	{
		loadedMesh->ptr["controller"] = new Controller(loadedMesh);
	}

	loadedMesh->val["target-stackability"] = targetS;

	loadedMeshHalfHight = (loadedMesh->bbmax.z() - loadedMesh->bbmin.z()) * -0.5;

	setActiveObject(loadedMesh);

	isDrawStacking = false;
	designWidget->showStacking->setChecked(isDrawStacking);

	stackingDir.setRotation(Quaternion());
	setManipulatedFrame(activeFrame);

	updateOffset();

	updateGL();

	numEdits = 0;

	// Timing
	globalTimer->restart();
}

void MyDesigner::setActiveObject(QSegMesh* newMesh)
{
	// Delete the original object
	if (activeMesh)
		emit( objectDiscarded( activeMesh->objectName() ) );

	// Setup the new object
	activeMesh = newMesh;

	// Change title of scene
	setWindowTitle(activeMesh->objectName());

	// Set camera
	resetView();

	// Update the object
	updateActiveObject();

	// Update offset
	activeOffset = new Offset(hiddenViewer);
	hiddenViewer->setActiveObject(activeMesh);
	updateOffset();
	
	stackingDir.alignWithFrame(activeFrame);
	
	// Undo
	undoStates.clear();
	SaveUndo();

	// Stack panel 
	emit(objectInserted());
}

void MyDesigner::mousePressEvent( QMouseEvent* e )
{
	QGLViewer::mousePressEvent(e);

	if(!isMousePressed)
	{
		this->startMousePos2D = e->pos();
		camera()->convertClickToLine(e->pos(), startMouseOrigin, startMouseDir);
		
		if(!defCtrl) return;

		defCtrl->saveOriginal();

		// Set constraints
		if(transformMode == TRANSLATE_MODE)
		{

		}

		if(transformMode == ROTATE_MODE)
		{
			AxisPlaneConstraint * r = new AxisPlaneConstraint;
			r->setTranslationConstraintType(AxisPlaneConstraint::FORBIDDEN);
			defCtrl->getFrame()->setConstraint(r);
		}

		if(transformMode == SCALE_MODE)
		{
			//// Forbid everything
			//AxisPlaneConstraint * c = new AxisPlaneConstraint;
			//c->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
			//c->setTranslationConstraintType(AxisPlaneConstraint::FORBIDDEN);
			//defCtrl->getFrame()->setConstraint(c);

			//Vec3d o(currMouseOrigin[0],currMouseOrigin[1],currMouseOrigin[2]);
			//Vec3d r(currMouseDir[0],currMouseDir[1],currMouseDir[2]);

			//Vec3d px = rayMeshIntersect(o, r, planeX(defCtrl->pos(), skyRadius));
			//Vec3d py = rayMeshIntersect(o, r, planeY(defCtrl->pos(), skyRadius));
			//Vec3d pz = rayMeshIntersect(o, r, planeZ(defCtrl->pos(), skyRadius));

			//debugPoints.clear();

			//Point p(0,0,0);

			//// If we got a hit
			//if(px.x() < DBL_MAX) p = px;
			//if(py.y() < DBL_MAX) p = py;
			//if(pz.z() < DBL_MAX) p = pz;

			//startScalePos = p;
		}

		if(transformMode != NONE_MODE && selectMode != SELECT_NONE)
		{
			SaveUndo();
		}
	}

	isMousePressed = true;
}

void MyDesigner::mouseReleaseEvent( QMouseEvent* e )
{
	QGLViewer::mouseReleaseEvent(e);

	isMousePressed = false;

	if(transformMode == ROTATE_MODE)
	{
		defCtrl->saveOriginal();
		defCtrl->getFrame()->setOrientation(Quaternion());
	}

	if(transformMode == TRANSLATE_MODE)
	{
		defCtrl->saveOriginal();
	}

	if(transformMode == SCALE_MODE)
	{
		scaleDelta = Vec3d(1.0);
	}

	// View changer box area
	double scale = 90;
	int x = e->pos().x(), y = e->pos().y();
	if(x > width() - scale && y > height() - scale)
	{
		QPoint p(abs(x - width() + scale), abs(y - height() + scale));

		double meshHeight = activeMesh->bbmax.z() - activeMesh->bbmin.z();
		double meshLength = activeMesh->bbmax.y() - activeMesh->bbmin.y();
		double meshWidth = activeMesh->bbmax.x() - activeMesh->bbmin.x();

		QSize box(QSize(scale * 0.25, scale * 0.25));

		QRect top(QPoint(30,15), box);
		QRect front(QPoint(52,52), box);
		QRect side(QPoint(12,52), box);
		QRect corner(QPoint(32,45), box);
		QRect backside(QPoint(64,18), box);
		QRect back(QPoint(11,20), box);
		QRect bottom(QPoint(30,70), box);

		if(top.contains(p)){
			Frame f(Vec(0,0,3*meshHeight), Quaternion());
			camera()->interpolateTo(f,0.25);camera()->setType(Camera::ORTHOGRAPHIC);
			viewTitle = "Top";
		}

		if(bottom.contains(p)){
			Frame f(Vec(0,0,-3*meshHeight), Quaternion());
			f.rotate(Quaternion(Vec(1,0,0), M_PI));
			camera()->interpolateTo(f,0.25);camera()->setType(Camera::ORTHOGRAPHIC);
			viewTitle = "Bottom";
		}

		if(front.contains(p)){
			Frame f(Vec(3*meshLength,0,0), Quaternion(Vec(0,0,1),Vec(1,0,0)));
			f.rotate(Quaternion(Vec(0,0,1), M_PI / 2.0));
			camera()->interpolateTo(f,0.25);camera()->setType(Camera::ORTHOGRAPHIC);
			viewTitle = "Front";
		}

		if(side.contains(p)){
			Frame f(Vec(0,-3*meshWidth,0), Quaternion(Vec(0,0,1),Vec(0,-1,0)));
			camera()->interpolateTo(f,0.25);camera()->setType(Camera::ORTHOGRAPHIC);
			viewTitle = "Side";
		}
		
		if(backside.contains(p)){
			Frame f(Vec(0,3*meshWidth,0), Quaternion(Vec(0,0,1),Vec(0,1,0)));
			f.rotate(Quaternion(Vec(0,0,1), M_PI));
			camera()->interpolateTo(f,0.25);camera()->setType(Camera::ORTHOGRAPHIC);
			viewTitle = "Back-side";
		}
		 
		if(back.contains(p)){
			Frame f(Vec(-3*meshLength,0,0), Quaternion(Vec(0,0,-1),Vec(1,0,0)));
			f.rotate(Quaternion(Vec(0,0,-1), M_PI / 2.0));
			camera()->interpolateTo(f,0.25);camera()->setType(Camera::ORTHOGRAPHIC);
			viewTitle = "Back";
		}

		if(corner.contains(p)){
			double mx = 2*Max(meshLength, Max(meshWidth, meshHeight));
			Frame f(Vec(mx,-mx,mx), Quaternion());
			f.rotate(Quaternion(Vec(0,0,1), M_PI / 4.0));
			f.rotate(Quaternion(Vec(1,0,0), M_PI / 3.3));
			camera()->interpolateTo(f,0.25);camera()->setType(Camera::PERSPECTIVE);
			viewTitle = "View";
		}
	}
	else
	{
		if(activeOffset && selectMode != SELECT_NONE)
		{
			updateOffset();
		}
	}

	updateGL();
}

void MyDesigner::mouseMoveEvent( QMouseEvent* e )
{
	QGLViewer::mouseMoveEvent(e);
	currMousePos2D = e->pos();
	camera()->convertClickToLine(currMousePos2D, currMouseOrigin, currMouseDir);

	if(e->buttons() & Qt::LeftButton)
	{
		isMousePressed = true;
	}

	if(isMousePressed && defCtrl && !(e->modifiers() & Qt::ShiftModifier))
	{
		Vec3d o(currMouseOrigin[0],currMouseOrigin[1],currMouseOrigin[2]);
		Vec3d r(currMouseDir[0],currMouseDir[1],currMouseDir[2]);

		Vec3d px = rayMeshIntersect(o, r, planeX(defCtrl->pos(), skyRadius));
		Vec3d py = rayMeshIntersect(o, r, planeY(defCtrl->pos(), skyRadius));
		Vec3d pz = rayMeshIntersect(o, r, planeZ(defCtrl->pos(), skyRadius));

		debugPoints.clear();

		Point p(0,0,0);

		// If we got a hit
		if(px.x() < DBL_MAX) p = px;
		if(py.y() < DBL_MAX) p = py;
		if(pz.z() < DBL_MAX) p = pz;

		// Used later
		Vec3d x = Vec3d(1, 0, 0);
		Vec3d y = Vec3d(0, 1, 0);
		Vec3d z = Vec3d(0, 0, 1);

		if(transformMode == SCALE_MODE)
		{
			/*currScalePos = p;

			Primitive * prim = ctrl()->getSelectedPrimitive(); if(!prim) return;
		
			Vec3d delta = (currScalePos - startScalePos);

			delta.x() = abs(delta.x());
			delta.y() = abs(delta.y());
			delta.z() = abs(delta.z());
		
			// Project to main axes
			QMultiMap<double,int> proj;
			proj.insert(dot(delta, x), 0);
			proj.insert(dot(delta, y), 1);
			proj.insert(dot(delta, z), 2);

			// Extract sorted values 
			QVector<double> value;
			QVector<int> index;		
			foreach(double key, proj.keys()){
				foreach(int id, proj.values(key)){
					value.push_back(key);
					index.push_back(id);
				}
			}

			// Scaling
			delta = Vec3d(0.0);
			if (value[2] > value[1] * 3)
			{
				// line
				delta[index[2]] = value[2];
			}
			else if (value[2] < value[0] * 3)
			{
				// cube
				delta[index[0]] = value[0];
				delta[index[1]] = value[1];
				delta[index[2]] = value[2];
			}
			else 
			{
				// plane
				delta[index[1]] = value[1];
				delta[index[2]] = value[2];
			}

			bool isExpand = false;
			Vec3d start = defCtrl->pos();
			if((currScalePos - start).norm() > (startScalePos - start).norm())
				isExpand = true;

			if(isExpand) delta = Vec3d(1.0) + delta;
			else delta = Vec3d(1.0) - delta;

			defCtrl->scale(delta);

			scaleDelta = delta;

			updateGL();*/
		}

		if(transformMode == TRANSLATE_MODE)
		{
			// Should constraint to closest axis line..
		}
	}

	double scale = 90;
	int x = e->pos().x(), y = e->pos().y();
	if(x > width() - scale && y > height() - scale)
	{
		updateGL();
	}

	if(isMousePressed && selectMode == STACK_DIR_MODE)
	{
		updateGL();
	}
}

void MyDesigner::wheelEvent( QWheelEvent* e )
{
	if(selectMode != CONTROLLER_ELEMENT)
		QGLViewer::wheelEvent(e);

	switch (selectMode)
	{
	case CONTROLLER_ELEMENT:
		{
			if(!defCtrl) break;

			SaveUndo();

			double s = 0.1 * (e->delta() / 120.0);
			defCtrl->scaleUp(1 + s);		

			updateOffset();
			updateGL();
		}
		break;
	}
}

void MyDesigner::keyPressEvent( QKeyEvent *e )
{
	if(e->key() == Qt::Key_L)
	{
		QString fileName = QFileDialog::getOpenFileName(0, "Import Mesh", "", "Mesh Files (*.obj *.off *.stl)"); 
		this->loadMesh(fileName);
	}

	if(e->key() == Qt::Key_S)
	{
		isDrawStacking = !isDrawStacking;
		if(isDrawStacking) updateOffset();

		designWidget->showStacking->setChecked(isDrawStacking);
	}

	if(e->key() == Qt::Key_Space)	selectPrimitiveMode();
	if(e->key() == Qt::Key_C)		selectCameraMode();
	if(e->key() == Qt::Key_M)		moveMode();
	if(e->key() == Qt::Key_R)		rotateMode();
	if(e->key() == Qt::Key_E)		scaleMode();

	updateGL();

	if(e->key() != Qt::Key_Space) // disable fly mode..
		QGLViewer::keyPressEvent(e);
}

void MyDesigner::beginUnderMesh()
{
	if(isEmpty()) return;

	glPushMatrix();
	glTranslated(0,0, loadedMeshHalfHight);
}

void MyDesigner::endUnderMesh()
{
	if(isEmpty()) return;
	glPopMatrix();
}

void MyDesigner::drawWithNames()
{
	if(activeDeformer) activeDeformer->drawNames();
	if(activeVoxelDeformer) activeVoxelDeformer->drawNames();

	if(!ctrl()) return;

	if(selectMode == CONTROLLER) ctrl()->drawNames(false);
	if(selectMode == CONTROLLER_ELEMENT) ctrl()->drawNames(true);
}

void MyDesigner::postSelection( const QPoint& point )
{
	if(currMousePos2D.x() > width() - 90 && currMousePos2D.y() > height() - 90)
		return;

	int selected = selectedName();

	// General selection
	/*if(selected == -1)
		selection.clear();
	else
	{
		if(selection.contains( selected ))
			selection.remove(selection.indexOf(selected));
		else
		{*/
		if(selectMode != CONTROLLER_ELEMENT)
		{
			selection.clear();
			ctrl()->selectPrimitive(-1);
			selection.push_back(selected); // to start from 0
		}
		/*}
	}*/

	// FFD and such deformers
	if(activeDeformer && selectMode == FFD_DEFORMER){
		activeDeformer->postSelection(selected);

		if(selected >= 0)
			setManipulatedFrame( activeDeformer->getQControlPoint(selected) );
		else
			setManipulatedFrame( activeFrame );
	}

	if(activeVoxelDeformer && selectMode == VOXEL_DEFORMER){
		if(selected >= 0)
			setManipulatedFrame( activeVoxelDeformer->getQControlPoint(selected) );
		else
			setManipulatedFrame( activeFrame );

		activeVoxelDeformer->select(selected);
	}

	// Selection mode cases
	switch (selectMode)
	{
	case CONTROLLER:
		transformPrimitive();
		break;

	case CONTROLLER_ELEMENT:
		transformCurve();
		break;
	}

	updateGL();
}

void MyDesigner::transformPrimitive(bool modifySelect)
{
	Controller * c = ctrl();

	if(modifySelect)
	{
		int selected = selectedName();
		c->selectPrimitive(selected);
		if(selected == -1) 
		{
			transformMode = NONE_MODE;
			designWidget->selectPrimitiveButton->setChecked(true);
		}
		
		return;
	}

	if(!selection.isEmpty())
	{
		if(c->getSelectedPrimitive())
		{
			defCtrl = new QManualDeformer(c);
			setManipulatedFrame( defCtrl->getFrame() );

			Vec3d q = c->getSelectedPrimitive()->centerPoint();
			manipulatedFrame()->setPosition( Vec(q.x(), q.y(), q.z()) );

			this->connect(defCtrl, SIGNAL(objectModified()), SLOT(updateActiveObject()));
		}
	}
}

void MyDesigner::transformCurve(bool modifySelect)
{
	Controller * c = ctrl();
	
	if(!c || !c->getSelectedPrimitive())
		return;

	if(!selection.isEmpty())
	{
		int selected = selectedName();

		if(modifySelect){
			c->selectPrimitiveCurve(selected);
			return;
		}
	
		defCtrl = new QManualDeformer(c);
		setManipulatedFrame( defCtrl->getFrame() );

		Vec3d q = c->getSelectedCurveCenter();
		manipulatedFrame()->setPosition( Vec(q.x(), q.y(), q.z()) );

		this->connect(defCtrl, SIGNAL(objectModified()), SLOT(updateActiveObject()));
	}
}

void MyDesigner::setSelectMode( SelectMode toMode )
{
	this->selectMode = toMode;
}

Controller * MyDesigner::ctrl()
{
	if(!activeObject()) return NULL;
	return (Controller *) activeObject()->ptr["controller"];
}

void MyDesigner::selectPrimitiveMode()
{
	clearButtons();
	setManipulatedFrame(activeFrame);

	if(activeMesh == NULL) return;

	setMouseBinding(Qt::LeftButton, SELECT);
	setMouseBinding(Qt::ShiftModifier | Qt::LeftButton, CAMERA, ROTATE);

	// clear existing selection of curves
	Controller * c = ctrl();
	if(c && c->getSelectedPrimitive()) c->getSelectedPrimitive()->selectedCurveId = -1;

	// clear selection of primitives
	selection.clear();
	c->selectPrimitive(-1);

	setSelectMode(CONTROLLER);
	selectTool();
}

void MyDesigner::selectCurveMode()
{
	clearButtons();
	setManipulatedFrame(activeFrame);

	if(activeMesh == NULL) return;

	if(ctrl()->getSelectedPrimitive() == NULL)
	{
		selectMode = CONTROLLER;
		selectTool();
		displayMessage("Select a part first!");
		return;
	}

	setMouseBinding(Qt::LeftButton, SELECT);
	setMouseBinding(Qt::ShiftModifier | Qt::LeftButton, CAMERA, ROTATE);

	setSelectMode(CONTROLLER_ELEMENT);
	selectTool();
}

void MyDesigner::selectStackingMode()
{
	clearButtons();

	if(activeMesh == NULL) return;

	designWidget->selectStackingButton->setChecked(true);

	setMouseBinding(Qt::LeftButton, FRAME, ROTATE);
	setMouseBinding(Qt::ShiftModifier | Qt::LeftButton, CAMERA, ROTATE);

	selection.clear();
	ctrl()->selectPrimitive(-1);

	selectMode = STACK_DIR_MODE;
	transformMode = NONE_MODE;

	this->displayMessage("You can now rotate the stacking direction", 1000);

	isDrawStacking = true;
	designWidget->showStacking->setChecked(true);

	setManipulatedFrame(&stackingDir);
}

void MyDesigner::selectCameraMode()
{
	clearButtons();
	setManipulatedFrame(activeFrame);

	if(activeMesh == NULL) return;

	designWidget->selectCameraButton->setChecked(true);
	
	setMouseBinding(Qt::ShiftModifier | Qt::LeftButton, SELECT);
	setMouseBinding(Qt::LeftButton, CAMERA, ROTATE);

	selection.clear();
	ctrl()->selectPrimitive(-1);

	selectMode = SELECT_NONE;
	transformMode = NONE_MODE;

	this->displayMessage("Freely move camera", 1000);

	isDrawStacking = false;
	designWidget->showStacking->setChecked(isDrawStacking);
}

void MyDesigner::selectTool()
{
	if(activeMesh == NULL) return;

	activeDeformer = NULL;
	activeVoxelDeformer = NULL;

	if(selectMode == CONTROLLER) designWidget->selectPrimitiveButton->setChecked(true);
	if(selectMode == CONTROLLER_ELEMENT) designWidget->selectCurveButton->setChecked(true);
	transformMode = NONE_MODE;
	updateGL();

	designWidget->showStacking->setChecked(isDrawStacking);
}

void MyDesigner::moveMode()
{
	clearButtons();

	setMouseBinding(Qt::LeftButton, FRAME, TRANSLATE);
	setMouseBinding(Qt::RightButton, CAMERA, TRANSLATE);
	transformMode = TRANSLATE_MODE;
	toolMode();
}

void MyDesigner::rotateMode()
{
	clearButtons();

	setMouseBinding(Qt::LeftButton, FRAME, ROTATE);
	setMouseBinding(Qt::ControlModifier | Qt::LeftButton, FRAME, SCREEN_ROTATE);

	transformMode = ROTATE_MODE;
	toolMode();
}

void MyDesigner::scaleMode()
{
	clearButtons();

	setMouseBinding(Qt::LeftButton, FRAME, NO_MOUSE_ACTION);
	transformMode = SCALE_MODE;
	scaleDelta = Vec3d(1.0);
	toolMode();
}

void MyDesigner::toolMode()
{
	activeDeformer = NULL;
	activeVoxelDeformer = NULL;

	if(selection.empty())
	{
		this->displayMessage("* Please select a part to transform *");

		transformMode = NONE_MODE;
		selectMode = SELECT_NONE;
		
		setMouseBinding(Qt::LeftButton, CAMERA, ROTATE);
	}
	else
	{
		setMouseBinding(Qt::ShiftModifier | Qt::LeftButton, CAMERA, ROTATE);

		// Deal with buttons
		if(transformMode == TRANSLATE_MODE) designWidget->moveButton->setChecked(true);
		if(transformMode == ROTATE_MODE) designWidget->rotateButton->setChecked(true);
		if(transformMode == SCALE_MODE) designWidget->scaleButton->setChecked(true);

		// Connect to QManualDeformer
		if(selectMode == CONTROLLER) transformPrimitive(false);
		if(selectMode == CONTROLLER_ELEMENT) transformCurve(false);

		this->displayMessage("Press shift to move camera");

		if(transformMode == SCALE_MODE)
			this->displayMessage("Use your mouse wheel to scale a curve", 3000);
	}

	updateGL();
}

void MyDesigner::clearButtons()
{
	// un-check all buttons
	int count = designWidget->dockWidgetContents->layout()->count();
	for(int i = 0; i < count; i++){
		QLayoutItem * item = designWidget->dockWidgetContents->layout()->itemAt(i);
		QGroupBox *box = qobject_cast<QGroupBox*>(item->widget());
		if(box){
			int c = box->layout()->count();
			for(int j = 0; j < c; j++){
				QToolButton *button = qobject_cast<QToolButton*>(box->layout()->itemAt(j)->widget());
				if(button) 
					button->setChecked(false);
			}
		}
	}
}

void MyDesigner::print( QString message, long age )
{
	osdMessages.enqueue(message);
	timerScreenText->start(age);
	updateGL();
}

void MyDesigner::dequeueLastMessage()
{
	if(!osdMessages.isEmpty()){
		osdMessages.dequeue();
		updateGL();
	}
}

void MyDesigner::setActiveFFDDeformer()
{
	clearButtons();

	if(activeMesh == NULL) return;

	designWidget->ffdButton->setChecked(true);
	this->transformMode = NONE_MODE;

	activeVoxelDeformer = NULL;

	Primitive * prim = ctrl()->getSelectedPrimitive(); if(!prim) return;
	QSurfaceMesh* mesh = prim->m_mesh;

	int nx = 2, ny = 2, nz = 2;

	activeDeformer = new QFFD(mesh, BoundingBoxFFD, Vec3i(nx,ny,nz));
	connect(activeDeformer, SIGNAL(meshDeformed()), this, SLOT(updateActiveObject()));

	this->setSelectMode(FFD_DEFORMER);

	setMouseBinding(Qt::RightButton, FRAME, TRANSLATE);	
	setMouseBinding(Qt::LeftButton, SELECT);

	updateGL();
}

void MyDesigner::setActiveVoxelDeformer()
{
	clearButtons();

	if(activeMesh == NULL) return;

	designWidget->voxelButton->setChecked(true);
	this->transformMode = NONE_MODE;

	activeDeformer = NULL;

	Primitive * prim = ctrl()->getSelectedPrimitive(); if(!prim) return;
	QSurfaceMesh* mesh = prim->m_mesh;

	activeVoxelDeformer = new VoxelDeformer(mesh, 0.1);

	this->connect(activeVoxelDeformer, SIGNAL(meshDeformed()), this, SLOT(updateActiveObject()));

	this->setSelectMode(VOXEL_DEFORMER);

	setMouseBinding(Qt::RightButton, FRAME, TRANSLATE);	
	setMouseBinding(Qt::LeftButton, SELECT);

	updateGL();
}

void MyDesigner::drawStackStateChanged( int state )
{
	this->isDrawStacking = state;
	if(isDrawStacking) updateOffset();
	updateGL();
}

void MyDesigner::reloadActiveMesh()
{
	if(activeMesh)
	{
		selectMode = SELECT_NONE;
		transformMode = NONE_MODE;
		isMousePressed = false;
		skyRadius = 1.0;

		defCtrl = NULL;
		activeDeformer = NULL;
		activeVoxelDeformer = NULL;

		QString * myPath = (QString *) activeMesh->ptr["filename"];
		loadMesh(*myPath);
	}
}

void MyDesigner::SaveUndo()
{
	undoStates.push(ctrl()->getShapeState());

	//if(undoStates.size() > 30)
	//	undoStates.pop_front();

	numEdits++;
}

void MyDesigner::Undo()
{
	clearButtons();

	if(undoStates.isEmpty() || activeMesh == NULL) return;

	if(undoStates.size() > 1)
		ctrl()->setShapeState(undoStates.pop());

	selectMode = SELECT_NONE;
	transformMode = NONE_MODE;

	activeMesh->update_face_normals();
	activeMesh->update_vertex_normals();

	updateActiveObject();
	updateGL();
}

int MyDesigner::editTime()
{
	return globalTimer->elapsed();
}

QMap<QString, QString> MyDesigner::collectData()
{
	QMap<QString, QString> data;

	if(activeMesh)
	{
		data["mesh-name"] = *((QString *) activeMesh->ptr["filename"]);
		data["edit-time"] = QString::number(editTime());
		data["edit-num-operations"] = QString::number(numEdits);
		data["stackability-ratio"] = QString::number(stackingRatio);
		Vec3d stackShift = activeMesh->vec["stacking_shift"];
		data["stacking-shift"] = QString("%1 %2 %3").arg(stackShift[0]).arg(stackShift[1]).arg((stackShift[2])); 

		if(activeOffset) activeOffset->computeStackability();

		data["stackability"] = QString::number(activeMesh->val["stackability"]);
		data["controller"] = ctrl()->serialize();
	}

	return data;
}
