//Things you should all have anyway.
#include <stdio.h>
#include <tgmath.h>
#include <time.h>

//Things you gotta get.
#include <ft2build.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include FT_FREETYPE_H

//Functions.
#include "physics.h"
#include "parser.h"
#include "glfun.h"


FT_Library  library;
FT_Face face;


static int i, j, linkcount;

//Default settings
int obj = 0, width = 1200, height = 600;
int boxsize = 15;
char fontname[200] = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
char filename[200] = "posdata.dat";
float dt = 0.008, radius = 12.0;
long double elcharge = 0, gconst = 0, epsno = 0;
bool novid = 0, vsync = 1, quiet = 0, stop = 0;


time_t start, end;
double fps;
unsigned int counter = 0;
unsigned int sec;

GLint u_matrix = -1;
GLint attr_pos = 0, attr_color = 1;
GLfloat view_rotx = 0.0, view_roty = 0.0;


GLfloat colors[9] = {
  1.0f, 0.0f,  0.0f,
  0.0f, 1.0f,  0.0f,
  0.0f, 0.0f,  1.0f
};

int main( int argc, char *argv[] )
{
	/*	ARGUMENT SETTING	*/
	if( argc > 1 ) {
		for( i=1; i < argc; i++ ) {
			if( !strcmp( "--novid", argv[i] ) ) {
				novid = 1;
			}
			if( !strcmp( "--quiet", argv[i] ) ) {
				quiet = 1;
			}
			if( !strcmp( "-f", argv[i] ) ) {
				strcpy( filename, argv[i+1] );
			}
			if( !strcmp( "--nosync", argv[i] ) ) {
				vsync = 0;
			}
			if( !strcmp( "--help", argv[i] ) ) {
				printf("Usage:\n");
				printf("	-f (filename)		Specify a posdata file. Takes priority over configfile.\n");
				printf("	--novid 		Disable video output, do not initialize any graphical libraries.\n");
				printf("	--nosync		Disable vsync, render everything as fast as possible.\n"); 
				printf("	--quiet 		Disable any terminal output except errors.\n"); 
				printf("	--help  		What you're reading.\n");
				return 0;
			}
		}
	}
	obj = preparser(&dt, &elcharge, &gconst, &epsno, &width, &height, &boxsize, fontname, filename);
	/*	ARGUMENT SETTING	*/
	
	/*	Error handling.	*/
		if( obj == 0 ) {
			printf("ERROR! NO OBJECTS!\n");
			return 1;
		}
	/*	Error handling.	*/
	
	/*	OGL && EGL	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if( novid == 0 ) {
			window = SDL_CreateWindow( "Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL );
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetSwapInterval(vsync);
			SDL_GL_CreateContext(window);
		}
		SDL_Event event;
		if( quiet == 0 ) {
			printf("OpenGL Version %s\n", glGetString(GL_VERSION));
		}
		glViewport(0, 0, (GLint) width, (GLint) height);
		create_shaders();
	/*	OGL && EGL	*/
	
	/*	Freetype.	*/
		if(FT_Init_FreeType(&library)) {
			fprintf(stderr, "Could not init freetype library\n");
			return 1;
		}
		if(FT_New_Face(library, fontname, 0, &face)) {
			fprintf(stderr, "Could not open font\n");
		} else {
			FT_Set_Pixel_Sizes(face, 0, 12);
			for (unsigned long u = 32; u < 128; u++) {
				if (FT_Load_Char(face, u, FT_LOAD_RENDER)) {
					fprintf(stderr, "Could not load character %lu\n", u);
					continue;
				}
			}
		}
	/*	Freetype.	*/
	
	/*	PHYSICS.	*/
		data* object;
		if( quiet == 0 ) {
			printf("Objects: %i\n", obj);
			printf("Settings: dt=%f, widith=%i, height=%i, boxsize=%i, fontname=%s\n", dt, width, height, boxsize, fontname);
			printf("Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n", elcharge, gconst, epsno);
		}
		
		//Malloc the objects
		initphys(&object);
		
		parser(&object, filename);
	/*	PHYSICS.	*/
	
	time(&start);
	
	linkcount = obj*2;
	
	while( 1 ) {
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
				case SDL_QUIT:
					goto quit;
					break;
			}
		}
		if( stop == 0 ) integrate(object);
		if( quiet == 0 ) {
			//FPS calculation.
			time(&end);
			++counter;
			sec = difftime (end, start);
			fps = (((float)counter)/((float)sec));
			printf("FPS = %.2f\r", fps);
		}
		if( novid == 1 ) continue;
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		draw();
		
		/*	Link drawing	*/
		GLfloat link[linkcount][2];
		linkcount = 0;
		
		for(i = 1; i < obj + 1; i++) {
			for(j = 1; j < obj + 1; j++) {
				if( j==i ) continue;
				if( object[i].linkwith[j] != 0 ) {
					link[linkcount][0] = object[i].pos[0];
					link[linkcount][1] = object[i].pos[1];
					linkcount++;
					link[linkcount][0] = object[j].pos[0];
					link[linkcount][1] = object[j].pos[1];
					linkcount++;
				}
			}
		}
		
		
		glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, link);
		glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
		glEnableVertexAttribArray(attr_pos);
		glEnableVertexAttribArray(attr_color);
		glDrawArrays(GL_LINES, 0, (int)((linkcount+1)/2));
		glDisableVertexAttribArray(attr_pos);
		glDisableVertexAttribArray(attr_color);
		/*	Link drawing	*/
		
		/*
		//Drawing choseon object's red box.
		glColor3f(255,0,0);
		glBegin(GL_LINE_LOOP);
			for(i = 1; i < obj + 1; i++) {
				if(chosen==i) {
					glVertex3f( object[i].pos[0] - boxsize, object[i].pos[1] - boxsize, 1 );
					glVertex3f( object[i].pos[0] - boxsize, object[i].pos[1] + boxsize, 1 );
					glVertex3f( object[i].pos[0] + boxsize, object[i].pos[1] + boxsize, 1 );
					glVertex3f( object[i].pos[0] + boxsize, object[i].pos[1] - boxsize, 1 );
				}
			}
			glColor3f(255,255,255);
		glEnd();*/
		
		/*	Point/object drawing	*/
		GLfloat points[obj+1][2];
		for(i = 1; i < obj + 1; i++) {
			points[i-1][0] = object[i].pos[0];
			points[i-1][1] = object[i].pos[1];
		}
		
		glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, points);
		glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
		glEnableVertexAttribArray(attr_pos);
		glEnableVertexAttribArray(attr_color);
		glDrawArrays(GL_POINTS, 0, obj);
		glDisableVertexAttribArray(attr_pos);
		glDisableVertexAttribArray(attr_color);
		/*	Point/object drawing	*/
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		free(object);
		SDL_Quit();
		FT_Done_Face( face );
		FT_Done_FreeType( library );
		printf("\nQuitting!\n");
		return 0;
}
