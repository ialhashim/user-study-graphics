#pragma once

#include <math.h>

typedef struct
{
	float x;
	float y;

} Vector2f;

#define roundf(x) (floor(x+0.5f))

void drawRoundRect( int x,
	int y,
	int width,
	int height, Vec4d color = Vec4d(0,0,0,0.5),
	int radius = 25,
	int resolution = 64)
{
	float step = ( 2.0f * M_PI ) / resolution,
		angle = 0.0f,
		x_offset,
		y_offset;

	int i = 0;

	unsigned int index = 0,
		segment_count = ( int )( resolution / 4 );

	Vector2f *top_left     = ( Vector2f * ) malloc( segment_count * sizeof( Vector2f ) ), 
		*bottom_left       = ( Vector2f * ) malloc( segment_count * sizeof( Vector2f ) ),
		*top_right         = ( Vector2f * ) malloc( segment_count * sizeof( Vector2f ) ),
		*bottom_right      = ( Vector2f * ) malloc( segment_count * sizeof( Vector2f ) ),
		bottom_left_corner = { x + radius, y - height + radius }; 

	while( i != segment_count )
	{
		x_offset = cosf( angle );
		y_offset = sinf( angle );


		top_left[ index ].x = bottom_left_corner.x - 
			( x_offset * radius );
		top_left[ index ].y = ( height - ( radius * 2.0f ) ) + 
			bottom_left_corner.y - 
			( y_offset * radius );


		top_right[ index ].x = ( width - ( radius * 2.0f ) ) + 
			bottom_left_corner.x + 
			( x_offset * radius );
		top_right[ index ].y = ( height - ( radius * 2.0f ) ) + 
			bottom_left_corner.y -
			( y_offset * radius );


		bottom_right[ index ].x = ( width - ( radius * 2.0f ) ) +
			bottom_left_corner.x + 
			( x_offset * radius );
		bottom_right[ index ].y = bottom_left_corner.y + 
			( y_offset * radius );


		bottom_left[ index ].x = bottom_left_corner.x - 
			( x_offset * radius );
		bottom_left[ index ].y = bottom_left_corner.y +
			( y_offset * radius );


		top_left[ index ].x = roundf( top_left[ index ].x );
		top_left[ index ].y = roundf( top_left[ index ].y );


		top_right[ index ].x = roundf( top_right[ index ].x );
		top_right[ index ].y = roundf( top_right[ index ].y );


		bottom_right[ index ].x = roundf( bottom_right[ index ].x );
		bottom_right[ index ].y = roundf( bottom_right[ index ].y );


		bottom_left[ index ].x = roundf( bottom_left[ index ].x );
		bottom_left[ index ].y = roundf( bottom_left[ index ].y );

		angle -= step;

		++index;

		++i;
	}


	glBegin( GL_TRIANGLE_STRIP );
	{
		// Top
		{
			i = 0;
			while( i != segment_count )
			{
				
				glColor4dv(color);
				glVertex2i( top_left[ i ].x,
					top_left[ i ].y );

				
				glColor4dv(color);
				glVertex2i( top_right[ i ].x,
					top_right[ i ].y );

				++i;
			}
		}


		// In order to stop and restart the strip.
		glColor4dv(color);
		glVertex2i( top_right[ 0 ].x,
			top_right[ 0 ].y );

		glColor4dv(color);
		glVertex2i( top_right[ 0 ].x,
			top_right[ 0 ].y );


		// Center
		{
			
			glColor4dv(color);
			glVertex2i( top_right[ 0 ].x,
				top_right[ 0 ].y );


			
			glColor4dv(color);
			glVertex2i( top_left[ 0 ].x,
				top_left[ 0 ].y );


			
			glColor4dv(color);
			glVertex2i( bottom_right[ 0 ].x,
				bottom_right[ 0 ].y );


			
			glColor4dv(color);
			glVertex2i( bottom_left[ 0 ].x,
				bottom_left[ 0 ].y );
		}


		// Bottom
		i = 0;
		while( i != segment_count )
		{
			
			glColor4dv(color);
			glVertex2i( bottom_right[ i ].x,
				bottom_right[ i ].y );    

			
			glColor4dv(color);
			glVertex2i( bottom_left[ i ].x,
				bottom_left[ i ].y );                                    

			++i;
		}    
	}
	glEnd();



	glBegin( GL_LINE_STRIP );

	
	glColor4dv(color);

	// Border
	{
		i = ( segment_count - 1 );
		while( i > -1 )
		{    
			glVertex2i( top_left[ i ].x,
				top_left[ i ].y );

			--i;
		}


		i = 0;
		while( i != segment_count )
		{    
			glVertex2i( bottom_left[ i ].x,
				bottom_left[ i ].y );

			++i;
		}


		i = ( segment_count - 1 );
		while( i > -1 )
		{    
			glVertex2i( bottom_right[ i ].x,
				bottom_right[ i ].y );

			--i;
		}


		i = 0;
		while( i != segment_count )
		{    
			glVertex2i( top_right[ i ].x,
				top_right[ i ].y );

			++i;
		}


		// Close the border.
		glVertex2i( top_left[ ( segment_count - 1 ) ].x,
			top_left[ ( segment_count - 1 ) ].y );
	}
	glEnd();




	glBegin( GL_LINES );

	
	glColor4f( 0.0f, 0.5f, 1.0f, 1.0f );

	// Separator
	{
		// Top bar
		glVertex2i( top_right[ 0 ].x,
			top_right[ 0 ].y );

		glVertex2i( top_left[ 0 ].x,
			top_left[ 0 ].y );    


		// Bottom bar
		glVertex2i( bottom_left[ 0 ].x,
			bottom_left[ 0 ].y );    

		glVertex2i( bottom_right[ 0 ].x,
			bottom_right[ 0 ].y );    
	}
	glEnd();



	free( top_left );
	free( bottom_left );
	free( top_right );
	free( bottom_right );
}
