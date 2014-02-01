//Things you should all have anyway.
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <time.h>
#include <unistd.h>

//Things you gotta get.
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glut.h>

//Functions.
#include "physics.h"
#include "parser.h"

FT_Library  library;
FT_Face face;


static int i, j;
int mousex, mousey, chosen = 0;

//Default settings
int obj = 0, width = 1200, height = 600;
int boxsize = 15;
char fontname[200] = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
char filename[200] = "posdata.dat";
float dt = 0.008, radius = 12.0;
long double elcharge = 0, gconst = 0, epsno = 0;
bool novid = 0, quiet = 0, stop = 0;


v4sf vectemp;

time_t start, end;
double fps;
unsigned int counter = 0;
unsigned int sec;

int main( int argc, char *argv[] ) {
	/*	ARGUMENT SETTING	*/
	if( argc > 1 ) {
		printf("Arguments: ");
		for( i=1; i < argc; i++ ) {
			printf("%s ", argv[i]);
			if( !strcmp( "--novid", argv[i] ) ) {
				novid = 1;
			}
			if( !strcmp( "--quiet", argv[i] ) ) {
				quiet = 1;
			}
			if( !strcmp( "-f", argv[i] ) ) {
				strcpy( filename, argv[i+1] );
			}
		}
		printf("\n");
	}
	/*	ARGUMENT SETTING	*/
	
	obj = preparser(&dt, &elcharge, &gconst, &epsno, &width, &height, &boxsize, fontname, filename);
	
	/*	Error handling.	*/
		if( obj == 0 ) {
			printf("ERROR! NO OBJECTS!\n");
			return 1;
		}
	/*	Error handling.	*/
	
	/*	SDL.	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if( novid == 0 ) {
			window = SDL_CreateWindow( "Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL );
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetSwapInterval(-1);
			SDL_GL_CreateContext(window);
		}
		SDL_Event event;
		if( quiet == 0 ) {
			printf("OpenGL Version %s\n", glGetString(GL_VERSION));
		}
	/*	SDL.	*/
	
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
		
	/*	OpenGL.	*/
		glClearColor(0.1,0.1,0.1,1);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0, ((float)width/(float)height), 0.0, 10.0);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	/*	OpenGL.	*/
	
	/*	PHYSICS.	*/
		data* object;
		/*	Okay, some explanation is needed. We want to make an array containing object parameters. So fine, we make it as a pointer.
			Then we call a parser program to read in our object data and dump it into the struct of object parameters. We pass the memory
			address to it so it can screw around with it. Then after it does whatever it's supposed to, we run the integration in the main loop.	*/
		if( quiet == 0 ) {
			printf("Settings: dt=%f, widith=%i, height=%i, boxsize=%i, fontname=%s\n", dt, width, height, boxsize, fontname);
			printf("Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n", elcharge, gconst, epsno);
		}
		initphys(&object);
		parser(&object, filename);
	/*	PHYSICS.	*/
	
	time(&start);
	
	glTranslatef(0.0f, 0.0f, -6.0f);
	
	while( 1 ) {
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_SPACE) {
						if( stop == 0 ) stop = 1;
						else if( stop == 1 ) stop = 0;
					}
					if(event.key.keysym.sym==SDLK_RIGHT) {
						dt *= 2;
						printf("dt = %f\n", dt);
					}
					if(event.key.keysym.sym==SDLK_LEFT) {
						dt /= 2;
						printf("dt = %f\n", dt);
					}
					if(event.key.keysym.sym==SDLK_ESCAPE) {
						goto quit;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if( event.button.button == SDL_BUTTON_RIGHT ) {
						chosen = 0;
					}
					if( event.button.button == SDL_BUTTON_LEFT ) {
						SDL_GetMouseState(&mousex , &mousey);
						mousey = ((float)height/2 - (float)mousey) + (float)height/2;
							for(i = 1; i < obj + 1; i++) {
								if( abs( mousex - object[i].pos[0]) < 20 && abs( mousey - object[i].pos[1]) < 20 ) {
									chosen = i;
									printf("OBJ %i HAS BEEN CHOSEN!\n", chosen);
									break;
								}
							}
						if( object[chosen].pos[0] == 0 || object[chosen].pos[1] == 0 ) break;
						else if ( chosen != 0 ) {
							object[chosen].pos[0] = (float)mousex;
							object[chosen].pos[1] = (float)mousey;
						}
					}
					break;
				case SDL_MOUSEWHEEL:
					glTranslatef((float)event.wheel.x, 0.0f, (float)event.wheel.y);
					break;
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
		glRotatef(0.1f, 0.1f, 0.1f, 0.1f);
		
		//Drawing lines for every link.
		glBegin(GL_LINES);
			for(i = 1; i < obj + 1; i++) {
				for(j = 1; j < obj + 1; j++) {
					if( object[i].linkwith[j] != 0 ) {
						vectemp =  object[j].pos - object[i].pos;
						glVertex3f( object[i].pos[0], object[i].pos[1], object[i].pos[2] );
						glVertex3f( (object[i].pos[0] + vectemp[0]), (object[i].pos[1] + vectemp[1]), (object[i].pos[2] + vectemp[2]) );
					}
				}
			}
			glColor3f(255,255,255);
		glEnd();
		
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
		glEnd();
		
		
		//Drawing the objects.
		for(i = 1; i < obj + 1; i++) {
			glBegin(GL_TRIANGLE_FAN);
			radius = 8.0;
			if( object[i].charge < 0 ) glColor3f(0,0,255);
			if( object[i].charge == 0 ) glColor3f(255,255,255);
			if( object[i].charge > 0 ) glColor3f(255,0,0);
			if( object[i].center == 1 ) {
				glColor3f(208,0,144);
			}
			glVertex3f(object[i].pos[0], object[i].pos[1], object[i].pos[2]);
			for( int r = 1; r < 361; r++ ) {
				glVertex3f(object[i].pos[0] + sin((double)r) * (object[i].radius-object[i].pos[2]), object[i].pos[1] + cos((double)r) * (object[i].radius-object[i].pos[2]), object[i].pos[2]);
			}
			glEnd();
			glColor3f(255,255,255);
		}
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		free(object);
		//FT_Done_Face( face );
		//FT_Done_FreeType( library );
		SDL_DestroyWindow( window );
		SDL_Quit();
		printf("\nQuitting!\n");
		return 0;
}
