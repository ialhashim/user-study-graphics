#include "project/GraphicsLibrary/Basic/Circle.h"
#include "project/GraphicsLibrary/Mesh/QSegMesh.h"
#include "project/GL/VBO/VBO.h"

#include "MyDesigner.h"
#define GL_MULTISAMPLE 0x809D

#include <QKeyEvent>
#include <QFileDialog>
#include <QFileInfo>

// Misc.
#include "sphereDraw.h"
#include "drawRoundRect.h"

MyDesigner::MyDesigner( QWidget * parent /*= 0*/ ) : QGLViewer(parent)
{
	selectMode = SELECT_NONE;

	skyRadius = 1.0;

	activeMesh = NULL;
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
	drawCircleFade(Vec4d(0,0,0,0.3), 4); // Floor
	if(!isEmpty()){
		double r = 0.75 * Max(activeMesh->bbmax.x() - activeMesh->bbmin.x(),
			activeMesh->bbmax.y() - activeMesh->bbmin.y());
		glClear(GL_DEPTH_BUFFER_BIT);
		drawCircleFade(Vec4d(0,0,0,0.4), r); // Shadow
	}
	endUnderMesh();
	glClear(GL_DEPTH_BUFFER_BIT);
	Q_EMIT drawNeeded();
}

void MyDesigner::draw()
{
	// No object to draw
	if (isEmpty()) return;

	// The main object
	drawObject(); 

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
		for (QMap<QString, VBO*>::iterator i = vboCollection.begin(); i != vboCollection.end(); ++i)
			(*i)->render();

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
	for (QMap<QString, VBO*>::iterator i = vboCollection.begin(); i != vboCollection.end(); ++i)
		(*i)->render_depth();
	
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

	/* Mode text */
	this->startScreenCoordinatesSystem();
	glEnable(GL_BLEND);
	glColor4dv(Vec4d(0,0,0,0.25));
	drawRoundRect(50,50,100,100);
	this->stopScreenCoordinatesSystem();

	glColor4dv(Vec4d(1,1,1,1));
	drawText(15,15, "Select mode: " + selectModeTxt[selectMode]);

	QFontMetrics fm(font);
	int pixelsWide = fm.width("What's the width of this text?");
	int pixelsHigh = fm.height();
}

void MyDesigner::drawCircleFade(double * color, double radius)
{
	glDisable(GL_LIGHTING);
	Circle c(Vec3d(0,0,0), Vec3d(0,0,1), radius);
	std::vector<Point> pnts = c.getPoints();
	glBegin(GL_TRIANGLE_FAN);
	glColor4d(color[0],color[1],color[2],color[3]); glVertex3dv(c.getCenter());
	glColor4d(color[0],color[1],color[2],0); 
	foreach(Point p, c.getPoints())	glVertex3dv(p); 
	glVertex3dv(c.getPoints().front());
	glEnd();
	glEnable(GL_LIGHTING);
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
				vboCollection[objId] = new VBO( seg->n_vertices(), points.data(), vnormals.data(), vcolors.data(), seg->triangles );		
			}
		}
	}
}

void MyDesigner::updateActiveObject()
{
	foreach(VBO* vbo, vboCollection) delete vbo;

	vboCollection.clear();
	updateGL();
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
	activeMesh->ptr["controller"] = NULL;

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
	QGLViewer::wheelEvent(e);
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

}

void MyDesigner::postSelection( const QPoint& point )
{

}

void MyDesigner::setSelectMode( SelectMode toMode )
{
	this->selectMode = toMode;
}
