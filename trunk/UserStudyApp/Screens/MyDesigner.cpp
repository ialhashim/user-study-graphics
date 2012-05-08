#include "project/GraphicsLibrary/Basic/Circle.h"
#include "project/GraphicsLibrary/Mesh/QSegMesh.h"
#include "project/GL/VBO/VBO.h"
#include "project/Stacker/Controller.h"
#include "project/Stacker/Primitive.h"
#include "project/Stacker/QManualDeformer.h"
#include "project/Utility/SimpleDraw.h"

#include "MyDesigner.h"
#define GL_MULTISAMPLE 0x809D

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

MyDesigner::MyDesigner( QWidget * parent /*= 0*/ ) : QGLViewer(parent)
{
	defCtrl = NULL;
	selectMode = SELECT_NONE;
	transformMode = NONE_MODE;
	skyRadius = 1.0;
	activeMesh = NULL;
	isMousePressed = false;
	fm = new QFontMetrics(QFont());

	connect(this, SIGNAL(objectInserted()), SLOT(updateGL()));

	activeFrame = new ManipulatedFrame();
	setManipulatedFrame(activeFrame);

	// TEXT ON SCREEN
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(dequeueLastMessage()));

	this->setMouseTracking(true);
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
	setShortcut( HELP, Qt::CTRL+Qt::Key_H);
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
	if(!isEmpty()){
		double r = 0.75 * Max(activeMesh->bbmax.x() - activeMesh->bbmin.x(),
			activeMesh->bbmax.y() - activeMesh->bbmin.y());
		glClear(GL_DEPTH_BUFFER_BIT);
		drawShadows();
	}
	drawCircleFade(Vec4d(0,0,0,0.25), 4); // Floor
	endUnderMesh();
	glClear(GL_DEPTH_BUFFER_BIT);
}

void MyDesigner::drawShadows()
{
	double objectHeight = activeMesh->bbmax.z() - activeMesh->bbmin.z();

	glDisable(GL_DEPTH_TEST);
	/* Draw shadows */
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
		double opacity = 0.5 * (1 - ((z + (objectHeight*0.5)) / objectHeight));
		drawCircleFade(Vec4d(0,0,0,Max(0,Min(1,opacity))), 1); // Shadow
		glPopMatrix();
	}
	glEnable(GL_DEPTH_TEST);
}

void MyDesigner::draw()
{
	// No object to draw
	if (isEmpty()) return;

	// The main object
	drawObject();

	// Draw controller and primitives
	if(ctrl() && (selectMode == CONTROLLER || selectMode == CONTROLLER_ELEMENT)) 
		ctrl()->draw();
	
	// Draw debug geometries
	activeObject()->drawDebug();

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
			glMultMatrixd(manipulatedFrame()->matrix());
			glColor3f(1,1,0);
			glRotated(-90,0,0,1);
			drawAxis(toolScale);
			SimpleDraw::DrawSolidBox(Vec3d(0),0.1,0.1,0.1);
			glPopMatrix();
			break;
		}
	case ROTATE_MODE:
		{
			glDisable(GL_LIGHTING);
			glPushMatrix();
			glMultMatrixd(manipulatedFrame()->matrix());

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
			glMultMatrixd(manipulatedFrame()->matrix());

			Vec3d delta = scaleDelta;

			toolScale *= 0.5;

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
			i->render();

		/* GL_POLYGON_BIT */
		glPopAttrib ();
	} 
	else{// Fall back
		std::cout << "VBO is not supported." << std::endl;
		activeObject()->simpleDraw();
	}
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
	glLineWidth (skyRadius / 2.0);

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
	glColor4d(0,0,0,0.1);
	glLineWidth(1.0);
	drawGrid(2, Min(10, Max(100, camera()->position().x / skyRadius)));

	// Draw axis
	glPushMatrix();
	glTranslated(-2,-2,0);
	drawAxis(0.8);
	glPopMatrix();

	endUnderMesh();

	glClear(GL_DEPTH_BUFFER_BIT);
	drawTool();

	drawViewChanger();

	// Revolve Around Point, line when camera rolls, zoom region
	drawVisualHints();
	
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

	glColor4d(1,1,1,1); SimpleDraw::DrawCube(Vec3d(0,0,0));
	glColor4d(1,0,0,1);

	double scale = 1.0;
	Vec3d corner (0.5,0.5,0.5);SimpleDraw::DrawArrowDirected(corner, corner, scale,true,true);
	Vec3d top (0,0,0.5);SimpleDraw::DrawArrowDirected(top, top, scale,true,true);
	Vec3d front (0,0.5,0);SimpleDraw::DrawArrowDirected(front, front, scale,true,true);
	Vec3d side (0.5,0,0);SimpleDraw::DrawArrowDirected(side, side, scale,true,true);

	glMatrixMode(GL_PROJECTION);glPopMatrix();
	glMatrixMode(GL_MODELVIEW);glPopMatrix();

	glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
	glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
}

void MyDesigner::drawOSD()
{
	QStringList selectModeTxt, toolModeTxt;
	selectModeTxt << "Camera" << "Mesh" << "Vertex" << "Edge" 
		<< "Face" << "Primitive" << "Curve" << "FFD" << "Voxel";
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

	// Textual log messages
	for(int i = 0; i < osdMessages.size(); i++){
		int margin = 20; //px
		int x = margin;
		int y = (i * QFont().pointSize() * 1.5f) + margin;

		qglColor(Qt::white);
		renderText(x, y, osdMessages.at(i));
	}

	setForegroundColor(QColor(0,0,0));
	QGLViewer::postDraw();
}

void MyDesigner::drawMessage(QString message, int x, int y, Vec4d backcolor)
{
	int pixelsWide = fm->width(message);
	int pixelsHigh = fm->height() * 1.5;

	int margin = 20;

	this->startScreenCoordinatesSystem();
	glEnable(GL_BLEND);
	drawRoundRect(x, y,pixelsWide + margin, pixelsHigh * 1.5, backcolor, 5);
	this->stopScreenCoordinatesSystem();

	glColor4dv(Vec4d(1,1,1,1));
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

	// Stack panel 
	emit(objectInserted());
}

void MyDesigner::loadMesh( QString fileName )
{
	if(fileName.isEmpty() || !QFileInfo(fileName).exists()) return;

	selection.clear();

	QString newObjId = QFileInfo(fileName).fileName();
	newObjId.chop(4);

	QSegMesh * loadedMesh = new QSegMesh();

	// Reading QSegMesh
	loadedMesh->read(fileName);

	// Set global ID for the mesh and all its segments
	loadedMesh->setObjectName(newObjId);

	// Load controller
	loadedMesh->ptr["controller"] = new Controller(loadedMesh);
	Controller * ctrl = (Controller *)loadedMesh->ptr["controller"];

	// Setup controller file name
	fileName.chop(3);fileName += "ctrl";

	if(QFileInfo(fileName).exists())
	{
		std::ifstream inF(qPrintable(fileName), std::ios::in);
		ctrl->load(inF);
		inF.close();

		fileName.chop(4);fileName += "grp";
		if(QFileInfo(fileName).exists())
		{
			std::ifstream inF(qPrintable(fileName), std::ios::in);
			ctrl->loadGroups(inF);
			inF.close();
		}
	}

	setActiveObject(loadedMesh);
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
		if(transformMode == ROTATE_MODE)
		{
			AxisPlaneConstraint * r = new AxisPlaneConstraint;
			r->setTranslationConstraintType(AxisPlaneConstraint::FORBIDDEN);
			defCtrl->getFrame()->setConstraint(r);
		}

		if(transformMode == SCALE_MODE)
		{
			// Forbid everything
			AxisPlaneConstraint * c = new AxisPlaneConstraint;
			c->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
			c->setTranslationConstraintType(AxisPlaneConstraint::FORBIDDEN);
			defCtrl->getFrame()->setConstraint(c);

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

			startScalePos = p;
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
		updateGL();
	}

	if(transformMode == TRANSLATE_MODE)
	{
		defCtrl->saveOriginal();
	}

	if(transformMode == SCALE_MODE)
	{
		scaleDelta = Vec3d(1.0);
		updateGL();
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

		if(top.contains(p)){
			Frame f(Vec(0,0,3*meshHeight), Quaternion());
			camera()->interpolateTo(f,0.25);
		}

		if(front.contains(p)){
			Frame f(Vec(3*meshLength,0,0), Quaternion(Vec(0,0,1),Vec(1,0,0)));
			f.rotate(Quaternion(Vec(0,0,1), M_PI / 2.0));
			camera()->interpolateTo(f,0.25);
		}

		if(side.contains(p)){
			Frame f(Vec(0,-3*meshWidth,0), Quaternion(Vec(0,0,1),Vec(0,-1,0)));
			camera()->interpolateTo(f,0.25);
		}

		if(corner.contains(p)){
			Frame f(Vec(2*meshWidth,2*-meshLength,2*meshHeight), Quaternion());
			f.rotate(Quaternion(Vec(0,0,1), M_PI / 4.0));
			f.rotate(Quaternion(Vec(1,0,0), M_PI / 4.0));
			f.translate(-meshWidth * 0.8, meshLength * 0.5,0);
			camera()->interpolateTo(f,0.25);
		}
	}
}

void MyDesigner::mouseMoveEvent( QMouseEvent* e )
{
	QGLViewer::mouseMoveEvent(e);
	currMousePos2D = e->pos();
	camera()->convertClickToLine(currMousePos2D, currMouseOrigin, currMouseDir);

	if(isMousePressed && defCtrl && e->buttons() & Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier))
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
			currScalePos = p;

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

			updateGL();
		}

		if(transformMode == TRANSLATE_MODE)
		{
			// Should constraint to closest axis line..
		}
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

			double s = 0.1 * (e->delta() / 120.0);
			defCtrl->scaleUp(1 + s);
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

	QGLViewer::keyPressEvent(e);
}

void MyDesigner::beginUnderMesh()
{
	if(isEmpty()) return;

	glPushMatrix();
	glTranslated(0,0,(activeMesh->bbmax.z() - activeMesh->bbmin.z()) * -0.5);
}

void MyDesigner::endUnderMesh()
{
	if(isEmpty()) return;
	glPopMatrix();
}

void MyDesigner::drawWithNames()
{
	if(!ctrl()) return;

	if(selectMode == CONTROLLER) ctrl()->drawNames(false);
	if(selectMode == CONTROLLER_ELEMENT) ctrl()->drawNames(true);
}

void MyDesigner::postSelection( const QPoint& point )
{
	int selected = selectedName();

	// General selection
	if(selected == -1)
		selection.clear();
	else
	{
		if(selection.contains( selected ))
			selection.remove(selection.indexOf(selected));
		else
			selection.push_back(selected); // to start from 0
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
			clearButtons();
			designWidget->selectPrimitiveButton->setChecked(true);
			return;
		}
	}

	if(!selection.isEmpty())
	{
		defCtrl = new QManualDeformer(c);
		this->connect(defCtrl, SIGNAL(objectModified()), SLOT(updateActiveObject()));
		//this->connect(defCtrl, SIGNAL(objectModified()), sp, SLOT(updateActiveObject()));

		emit(objectInserted());

		setManipulatedFrame( defCtrl->getFrame() );

		if(c->getSelectedPrimitive())
		{
			Vec3d q = c->getSelectedPrimitive()->centerPoint();
			manipulatedFrame()->setPosition( Vec(q.x(), q.y(), q.z()) );
		}
	}
}

void MyDesigner::transformCurve(bool modifySelect)
{
	Controller * c = ctrl();
	
	int selected = selectedName();

	if(modifySelect)
		if(!c->selectPrimitiveCurve(selected)) return;
	
	defCtrl = new QManualDeformer(c);
	this->connect(defCtrl, SIGNAL(objectModified()), SLOT(updateActiveObject()));
	//this->connect(defCtrl, SIGNAL(objectModified()), sp, SLOT(updateActiveObject()));

	emit(objectInserted());

	setManipulatedFrame( defCtrl->getFrame() );
	Vec3d q = c->getSelectedCurveCenter();
	manipulatedFrame()->setPosition( Vec(q.x(), q.y(), q.z()) );
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
	setMouseBinding(Qt::LeftButton, SELECT);
	setMouseBinding(Qt::ShiftModifier | Qt::LeftButton, CAMERA, ROTATE);

	setSelectMode(CONTROLLER_ELEMENT);
	selectTool();
}

void MyDesigner::selectMultiMode()
{

}

void MyDesigner::selectTool()
{
	clearButtons();
	if(selectMode == CONTROLLER) designWidget->selectPrimitiveButton->setChecked(true);
	if(selectMode == CONTROLLER_ELEMENT) designWidget->selectCurveButton->setChecked(true);
	transformMode = NONE_MODE;
	updateGL();
}

void MyDesigner::moveMode()
{
	setMouseBinding(Qt::LeftButton, FRAME, TRANSLATE);
	transformMode = TRANSLATE_MODE;
	toolMode();
}

void MyDesigner::rotateMode()
{
	setMouseBinding(Qt::LeftButton, FRAME, ROTATE);
	transformMode = ROTATE_MODE;
	toolMode();
}

void MyDesigner::scaleMode()
{
	setMouseBinding(Qt::LeftButton, FRAME, NO_MOUSE_ACTION);
	transformMode = SCALE_MODE;
	scaleDelta = Vec3d(1.0);
	toolMode();
}

void MyDesigner::toolMode()
{
	setMouseBinding(Qt::ControlModifier | Qt::LeftButton, SELECT);
	setMouseBinding(Qt::ShiftModifier | Qt::LeftButton, CAMERA, ROTATE);

	clearButtons();

	if(transformMode == TRANSLATE_MODE) designWidget->moveButton->setChecked(true);
	if(transformMode == ROTATE_MODE) designWidget->rotateButton->setChecked(true);
	if(transformMode == SCALE_MODE) designWidget->scaleButton->setChecked(true);

	if(selectMode == CONTROLLER) transformPrimitive(false);
	if(selectMode == CONTROLLER_ELEMENT) transformCurve(false);

	if(selection.empty())
	{
		if(transformMode != NONE_MODE)	transformMode = NONE_MODE;
		clearButtons();
		setMouseBinding(Qt::LeftButton, CAMERA, ROTATE);
		this->displayMessage("Please select something");
	}
	else
	{
		this->displayMessage("Press shift to move camera", 1000);
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
		if(box)
		{
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
	timer->start(age);
	updateGL();
}

void MyDesigner::dequeueLastMessage()
{
	if(!osdMessages.isEmpty()){
		osdMessages.dequeue();
		updateGL();
	}
}
