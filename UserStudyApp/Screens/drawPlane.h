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
