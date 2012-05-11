#pragma once

#include "project/GraphicsLibrary/Mesh/QSegMesh.h"

QSurfaceMesh planeX(Vec3d center = Vec3d(0,0,0), double len = 1.0)
{
	QSurfaceMesh m;

	Vec3d x(len,0,0), y(0,-len,0), z(0,0,len), zero(center);
	Surface_mesh::Vertex v0 = m.add_vertex(zero);
	Surface_mesh::Vertex v1 = m.add_vertex(zero + y);
	Surface_mesh::Vertex v2 = m.add_vertex(zero + y + z);
	Surface_mesh::Vertex v3 = m.add_vertex(zero + z);

	m.add_triangle(v2,v1,v0);
	m.add_triangle(v0,v3,v2);
	m.buildUp();
	return m;
}

QSurfaceMesh planeY(Vec3d center = Vec3d(0,0,0), double len = 1.0)
{
	QSurfaceMesh m;
	
	Vec3d x(len,0,0), y(0,-len,0), z(0,0,len), zero(center);
	Surface_mesh::Vertex v0 = m.add_vertex(zero);
	Surface_mesh::Vertex v1 = m.add_vertex(zero + x);
	Surface_mesh::Vertex v2 = m.add_vertex(zero + x + z);
	Surface_mesh::Vertex v3 = m.add_vertex(zero + z);

	m.add_triangle(v0,v1,v2);
	m.add_triangle(v2,v3,v0);
	m.buildUp();
	return m;
}

QSurfaceMesh planeZ(Vec3d center = Vec3d(0,0,0), double len = 1.0)
{
	QSurfaceMesh m;

	Vec3d x(len,0,0), y(0,-len,0), z(0,0,len), zero(center);
	Surface_mesh::Vertex v0 = m.add_vertex(zero);
	Surface_mesh::Vertex v1 = m.add_vertex(zero + x);
	Surface_mesh::Vertex v2 = m.add_vertex(zero + x + y);
	Surface_mesh::Vertex v3 = m.add_vertex(zero + y);

	m.add_triangle(v2,v1,v0);
	m.add_triangle(v0,v3,v2);
	m.buildUp();
	return m;
}

#include "project/GraphicsLibrary/Basic/Triangle.h"
Vec3d rayMeshIntersect(Vec3d origin, Vec3d direction, QSurfaceMesh & mesh)
{
	Surface_mesh::Vertex_property<Point>  points = mesh.vertex_property<Point>("v:point");
	Surface_mesh::Face_iterator fit, fend = mesh.faces_end();
	Surface_mesh::Vertex_around_face_circulator fvit;
	Surface_mesh::Vertex v0, v1, v2;

	Ray r(origin, direction);

	for(fit = mesh.faces_begin(); fit != fend; ++fit){
		fvit = mesh.vertices(fit);

		v0 = fvit; v1 = ++fvit; v2 = ++fvit;
		Point p0 = points[v0], p1 = points[v1], p2 = points[v2];

		HitResult res;

		Triangle tri(p0,p1,p2);
		tri.intersectionTest(r, res, true);

		if(res.hit)
			return tri.getBarycentric(res.u, res.v);
	}

	return(Vec3d(DBL_MAX,DBL_MAX,DBL_MAX));
}

enum {X, Y, Z, W};
enum {A, B, C, D};

/* Find the plane equation given 3 points. */
void findPlane(GLfloat plane[4], GLfloat v0[3], GLfloat v1[3], GLfloat v2[3])
{
	GLfloat vec0[3], vec1[3];

	/* Need 2 vectors to find cross product. */
	vec0[X] = v1[X] - v0[X];
	vec0[Y] = v1[Y] - v0[Y];
	vec0[Z] = v1[Z] - v0[Z];

	vec1[X] = v2[X] - v0[X];
	vec1[Y] = v2[Y] - v0[Y];
	vec1[Z] = v2[Z] - v0[Z];

	/* find cross product to get A, B, and C of plane equation */
	plane[A] = vec0[Y] * vec1[Z] - vec0[Z] * vec1[Y];
	plane[B] = -(vec0[X] * vec1[Z] - vec0[Z] * vec1[X]);
	plane[C] = vec0[X] * vec1[Y] - vec0[Y] * vec1[X];

	plane[D] = -(plane[A] * v0[X] + plane[B] * v0[Y] + plane[C] * v0[Z]);
}

void shadowMatrix(GLfloat shadowMat[4][4], GLfloat groundplane[4], GLfloat lightpos[4])
{
	GLfloat dot;
	/* Find dot product between light position vector and ground plane normal. */
	dot = groundplane[X] * lightpos[X] + groundplane[Y] * lightpos[Y] +
		groundplane[Z] * lightpos[Z] + groundplane[W] * lightpos[W];

	shadowMat[0][0] = dot - lightpos[X] * groundplane[X];
	shadowMat[1][0] = 0.f - lightpos[X] * groundplane[Y];
	shadowMat[2][0] = 0.f - lightpos[X] * groundplane[Z];
	shadowMat[3][0] = 0.f - lightpos[X] * groundplane[W];

	shadowMat[X][1] = 0.f - lightpos[Y] * groundplane[X];
	shadowMat[1][1] = dot - lightpos[Y] * groundplane[Y];
	shadowMat[2][1] = 0.f - lightpos[Y] * groundplane[Z];
	shadowMat[3][1] = 0.f - lightpos[Y] * groundplane[W];

	shadowMat[X][2] = 0.f - lightpos[Z] * groundplane[X];
	shadowMat[1][2] = 0.f - lightpos[Z] * groundplane[Y];
	shadowMat[2][2] = dot - lightpos[Z] * groundplane[Z];
	shadowMat[3][2] = 0.f - lightpos[Z] * groundplane[W];

	shadowMat[X][3] = 0.f - lightpos[W] * groundplane[X];
	shadowMat[1][3] = 0.f - lightpos[W] * groundplane[Y];
	shadowMat[2][3] = 0.f - lightpos[W] * groundplane[Z];
	shadowMat[3][3] = dot - lightpos[W] * groundplane[W];
}
