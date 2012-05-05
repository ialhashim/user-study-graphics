#pragma once 

#include <math.h>

static void fghCircleTable(double **sint,double **cost, const int n)
{
    int i;

    /* Table size, the sign of n flips the circle direction */

    const int size = abs(n);

    /* Determine the angle between samples */

    const double angle = 2*M_PI/(double)( ( n == 0 ) ? 1 : n );

    /* Allocate memory for n samples, plus duplicate of first entry at the end */

    *sint = (double *) calloc(sizeof(double), size+1);
    *cost = (double *) calloc(sizeof(double), size+1);

    /* Bail out if memory allocation fails, fgError never returns */

    if (!(*sint) || !(*cost))
    {
        free(*sint);
        free(*cost);
    }

    /* Compute cos and sin around the circle */

    (*sint)[0] = 0.0;
    (*cost)[0] = 1.0;

    for (i=1; i<size; i++)
    {
        (*sint)[i] = sin(angle*i);
        (*cost)[i] = cos(angle*i);
    }

    /* Last sample is duplicate of the first */

    (*sint)[size] = (*sint)[0];
    (*cost)[size] = (*cost)[0];
}

/*
 * Draws a solid sphere
 */
void drawSolidSphere(double radius, int slices, int stacks, bool half = false, bool magical = false)
{
    int i,j;

    /* Adjust z and radius as stacks are drawn. */
    double z0,z1;
    double r0,r1;

    /* Pre-computed circle */
    double *sint1,*cost1;
    double *sint2,*cost2;

    fghCircleTable(&sint1,&cost1,-slices);
    fghCircleTable(&sint2,&cost2,stacks*2);

    /* The top stack is covered with a triangle fan */

    z0 = 1.0;
    z1 = cost2[(stacks>0)?1:0];
    r0 = 0.0;
    r1 = sint2[(stacks>0)?1:0];

	if(magical) {
		glDisable(GL_LIGHTING);
		glColor4d(1,1,1,1);

		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
	}

	glShadeModel(GL_SMOOTH);

    glBegin(GL_TRIANGLE_FAN);

        glNormal3d(0,0,1);
        glVertex3d(0,0,radius);

        for (j=slices; j>=0; j--)
        {
            glNormal3d(cost1[j]*r1,        sint1[j]*r1,        z1       );
            glVertex3d(cost1[j]*r1*radius, sint1[j]*r1*radius, z1*radius);
        }

    glEnd();

	double s = 1.4;

    /* Cover each stack with a quad strip, except the top and bottom stacks */
    for( i=1; i<stacks-1; i++ )
    {
        z0 = z1; z1 = cost2[i+1];
        r0 = r1; r1 = sint2[i+1];

		if(half && i >= stacks / 2) break;

        glBegin(GL_QUAD_STRIP);

            for(j=0; j<=slices; j++)
            {
				if(half && magical) {glColor4d(1,1,1, Min(1,s * (1 - (double(i+1) / (double(stacks) / 2.0)))));}
                glNormal3d(cost1[j]*r1,        sint1[j]*r1,        z1       );
                glVertex3d(cost1[j]*r1*radius, sint1[j]*r1*radius, z1*radius);


				if(half && magical) {glColor4d(1,1,1, Min(1,s * (1 - (double(i) / (double(stacks) / 2.0)))));}
                glNormal3d(cost1[j]*r0,        sint1[j]*r0,        z0       );
                glVertex3d(cost1[j]*r0*radius, sint1[j]*r0*radius, z0*radius);
            }

        glEnd();
    }

    /* The bottom stack is covered with a triangle fan */
    z0 = z1;
    r0 = r1;

    glBegin(GL_TRIANGLE_FAN);

        glNormal3d(0,0,-1);
        glVertex3d(0,0,-radius);

        for (j=0; j<=slices; j++)
        {
			if(half) break;

            glNormal3d(cost1[j]*r0,        sint1[j]*r0,        z0       );
            glVertex3d(cost1[j]*r0*radius, sint1[j]*r0*radius, z0*radius);
        }

    glEnd();

	if(magical)
	{
		glCullFace(GL_BACK);
		glDisable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
	}

    /* Release sin and cos tables */

    free(sint1);
    free(cost1);
    free(sint2);
    free(cost2);
}
