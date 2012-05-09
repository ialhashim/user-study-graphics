#pragma once

static int cubeIds[6][4] = 
{
	1, 2, 6, 5,
	0, 4, 7, 3,
	4, 5, 6, 7,
	0, 3, 2, 1,
	0, 1, 5, 4,
	2, 3, 7, 6
};

#include "project/GraphicsLibrary/Mesh/QSegMesh.h"

QSurfaceMesh Box(double width=1, double length=1, double height=1, Vec3d center = Vec3d(0,0,0))
{
	QSurfaceMesh mesh;

	width *= 0.5; length *= 0.5; height *= 0.5;

	// Corner points
	std::vector<Vec3d> pnts;

	pnts.push_back( Vec3d (width, length, height) + center );
	pnts.push_back( Vec3d (-width, length, height) + center);
	pnts.push_back( Vec3d (-width, -length, height) + center);
	pnts.push_back( Vec3d (width, -length, height) + center);

	pnts.push_back( Vec3d (width, length, -height) + center);
	pnts.push_back( Vec3d (-width, length, -height) + center);
	pnts.push_back( Vec3d (-width, -length, -height) + center);
	pnts.push_back( Vec3d (width, -length, -height) + center);

	for(uint i = 0; i < pnts.size(); i++)
		mesh.add_vertex(pnts[i]);

	for (int i = 0; i < 6; i++){
		Surface_mesh::Vertex v0(cubeIds[i][0]);
		Surface_mesh::Vertex v1(cubeIds[i][1]);
		Surface_mesh::Vertex v2(cubeIds[i][2]);
		Surface_mesh::Vertex v3(cubeIds[i][3]);

		mesh.add_triangle(v2, v1, v0);
		mesh.add_triangle(v0, v3, v2);
	}

	mesh.buildUp();

	return mesh;
}
