#include "project/GraphicsLibrary/Basic/Circle.h"
#include "project/GraphicsLibrary/Mesh/QSegMesh.h"
#include "project/GL/VBO/VBO.h"

#include "MyDesigner.h"
#define GL_MULTISAMPLE 0x809D

#include "sphereDraw.h"
#include <QKeyEvent>
#include <QFileDialog>
#include <QFileInfo>

MyDesigner::MyDesigner( QWidget * parent /*= 0*/ ) : QGLViewer(parent)
{
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
		QMap<QString, VBO*>::iterator i;
		for (i = vboCollection.begin(); i != vboCollection.end(); ++i)
			(*i)->render();

		drawObjectOutline();
	} 
	else{// Fall back
		std::cout << "VBO is not supported." << std::endl;
		activeObject()->simpleDraw();
	}
}

void MyDesigner::drawObjectOutline()
{
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glEnable( GL_LIGHTING );

	glClearStencil(0);
	glClear( GL_STENCIL_BUFFER_BIT );
	glEnable( GL_STENCIL_TEST );

	glStencilFunc( GL_ALWAYS, 1, 0xFFFF );
	glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glColor3f( 0.0f, 0.0f, 0.0f );
	activeMesh->simpleDraw(true);
	glDisable( GL_LIGHTING );

	glStencilFunc( GL_NOTEQUAL, 1, 0xFFFF );
	glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );

	glLineWidth( 4 );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glColor3f( 0.0f, 0.0f, 0.0f );
	activeMesh->simpleDraw(false);

	glPointSize( 4 );
	activeMesh->simpleDraw(false, true);

	glPopAttrib();
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

	QSegMesh * loadedMesh = new QSegMesh();
	loadedMesh->read(fileName);

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
