#include "project/GraphicsLibrary/Basic/Circle.h"
#include "project/GraphicsLibrary/Mesh/QSegMesh.h"
#include "project/GL/VBO/VBO.h"
#include "project/Stacker/Controller.h"
#include "project/Stacker/Primitive.h"
#include "project/Stacker/QManualDeformer.h"

#include "MyDesigner.h"
#define GL_MULTISAMPLE 0x809D

#include <QKeyEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
QFontMetrics * fm;

// Misc.
#include "sphereDraw.h"
#include "drawRoundRect.h"

MyDesigner::MyDesigner( QWidget * parent /*= 0*/ ) : QGLViewer(parent)
{
	selectMode = SELECT_NONE;
	skyRadius = 1.0;
	activeMesh = NULL;
	fm = new QFontMetrics(QFont());

	connect(this, SIGNAL(objectInserted()), SLOT(updateGL()));

	activeFrame = new ManipulatedFrame();
	setManipulatedFrame(activeFrame);

	// TEXT ON SCREEN
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(dequeueLastMessage()));
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

	// Revolve Around Point, line when camera rolls, zoom region
	drawVisualHints();

	drawOSD();
}

void MyDesigner::drawOSD()
{
	QStringList selectModeTxt;
	selectModeTxt << "None" << "Mesh" << "Vertex" << "Edge" 
		<< "Face" << "Primitive" << "Curve" << "FFD" << "Voxel";

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

	// Textual log messages
	for(int i = 0; i < osdMessages.size(); i++){
		int margin = 20; //px
		int x = margin;
		int y = (i * QFont().pointSize() * 1.5f) + margin;

		qglColor(Qt::white);
		renderText(x, y, osdMessages.at(i));
	}
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
	fileName.chop(3);fileName += ".ctrl";

	if(QFileInfo(fileName).exists())
	{
		std::ifstream inF(qPrintable(fileName), std::ios::in);
		ctrl->load(inF);
	}

	setActiveObject(loadedMesh);
}

void MyDesigner::mousePressEvent( QMouseEvent* e )
{
	QGLViewer::mousePressEvent(e);
}

void MyDesigner::mouseReleaseEvent( QMouseEvent* e )
{
	QGLViewer::mouseReleaseEvent(e);
}

void MyDesigner::mouseMoveEvent( QMouseEvent* e )
{
	QGLViewer::mouseMoveEvent(e);
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
		{
			selection.push_back(selected); // to start from 0

			if (selectMode == CONTROLLER)
				print( QString("Selected primitive: %1").arg( qPrintable(ctrl()->getPrimitive(selected)->id) ) );

			if (selectMode == CONTROLLER_ELEMENT)
				print( QString("Selected curve: %1").arg( selected ) );
		}
	}

	// Selection mode cases
	Controller * c = ctrl();
	switch (selectMode)
	{
	case CONTROLLER:
		if (!isEmpty() && c)
		{
			c->selectPrimitive(selected);

			if(selected != -1)
			{
				defCtrl = new QManualDeformer(c);
				this->connect(defCtrl, SIGNAL(objectModified()), SLOT(updateActiveObject()));
				//this->connect(defCtrl, SIGNAL(objectModified()), sp, SLOT(updateActiveObject()));

				emit(objectInserted());

				setManipulatedFrame( defCtrl->getFrame() );
				Vec3d q = c->getSelectedCurveCenter();
				manipulatedFrame()->setPosition( Vec(q.x(), q.y(), q.z()) );
			}
		}
		break;

	case CONTROLLER_ELEMENT:
		if (!isEmpty() && c)
		{
			if(c->selectPrimitiveCurve(selected))
			{
				defCtrl = new QManualDeformer(c);
				this->connect(defCtrl, SIGNAL(objectModified()), SLOT(updateActiveObject()));
				//this->connect(defCtrl, SIGNAL(objectModified()), sp, SLOT(updateActiveObject()));

				emit(objectInserted());

				setManipulatedFrame( defCtrl->getFrame() );
				Vec3d q = c->getSelectedCurveCenter();
				manipulatedFrame()->setPosition( Vec(q.x(), q.y(), q.z()) );
			}

			if(selected == -1)
			{
				setSelectMode(CONTROLLER);
				setManipulatedFrame( activeFrame );
				c->selectPrimitive(selected);
			}
		}
		break;
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
	setSelectMode(CONTROLLER);
	updateGL();
}

void MyDesigner::selectCurveMode()
{
	setSelectMode(CONTROLLER_ELEMENT);
	updateGL();
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
