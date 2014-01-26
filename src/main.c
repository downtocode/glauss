//Things you should all have anyway.
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <time.h>

//Things you gotta get.
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

//Functions.
#include "physics.h"
#include "parser.h"

FT_Library  library;
FT_Face face;


static int i, j;
int mousex, mousey, chosen;

//Default settings
int obj, width = 1200, height = 600;
int boxsize = 15;
char fontname[100] = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
float dt = 0.008, radius = 12.0;
long double elcharge = 1.602176565;
bool novid = 0;


bool pause;

v4sf vectemp;

time_t start, end;
double fps;
unsigned int counter = 0;
unsigned int sec;

int main( int argc, char *argv[] ) {
	obj = preparser(&dt, &elcharge, &width, &height, &boxsize, fontname);
	
	/*	ARGUMENT SETTING	*/
	if( argc > 1 ) {
		printf("Arguments: ");
		for( i=1; i < argc; i++ ) {
			printf("%s ", argv[i]);
			if( !strcmp( "--no-vid", argv[i] ) ) {
				novid = 1;
			}
		}
		printf("\n");
	}
	/*	ARGUMENT SETTING	*/
	
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
		printf("OpenGL Version %s\n", glGetString(GL_VERSION));
	/*	SDL.	*/
	
	/*	Freetype.	*/
		if(FT_Init_FreeType(&library)) {
			fprintf(stderr, "Could not init freetype library\n");
			return 1;
		}
		
		if(FT_New_Face(library, fontname, 0, &face)) {
			fprintf(stderr, "Could not open font\n");
		}
		
		FT_Set_Pixel_Sizes(face, 0, 12);
		for (unsigned long u = 32; u < 128; u++) {
			if (FT_Load_Char(face, u, FT_LOAD_RENDER)) {
				fprintf(stderr, "Could not load character %lu\n", u);
				continue;
			}
		}
	/*	Freetype.	*/
		
	/*	OpenGL.	*/
		glClearColor(0.1,0.1,0.1,1);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glShadeModel(GL_SMOOTH);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glOrtho( 0.0, width, 0.0, height, 1.0, -1.0 );
	/*	OpenGL.	*/
	
	/*	PHYSICS.	*/
		data* object;
		/*	Okay, some explanation is needed. We want to make an array containing object parameters. So fine, we make it as a pointer.
			Then we call a parser program to read in our object data and dump it into the struct of object parameters. We pass the memory
			address to it so it can screw around with it. Then after it does whatever it's supposed to, we run the integration in the main loop.	*/
		printf("Settings: dt=%f, widith=%i, height=%i, boxsize=%i, fontname=%s\n", dt, width, height, boxsize, fontname);
		printf("Constants: elcharge=%Lf\n", elcharge);
		initphys(&object);
		parser(&object);
	/*	PHYSICS.	*/
	
	time(&start);
	
	while( 1 ) {
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_SPACE) {
						if( pause == 0 ) pause = 1;
						else if( pause == 1 ) pause = 0;
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
				case SDL_QUIT:
					goto quit;
					break;
			}
		}
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		if(pause == 0 ) integrate(object);
		//FPS calculation.
		time(&end);
		++counter;
		sec = difftime (end, start);
		fps = (((float)counter)/((float)sec));
		printf("FPS = %.2f\r", fps);
		if( novid == 1 ) continue;
		
		glColor3f(255,255,255);
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
		glEnd();
		
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
		glEnd();
		glColor3f(255,255,255);
		
		for(i = 1; i < obj + 2; i++) {
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
				glVertex3f(object[i].pos[0] + sin((double)r) * object[i].radius, object[i].pos[1] + cos((double)r) * object[i].radius, object[i].pos[2]);
			}
			glEnd();
		}
		
		glColor3f(255,255,255);
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		free(object);
		FT_Done_Face( face );
		FT_Done_FreeType( library );
		SDL_DestroyWindow( window );
		SDL_Quit();
		printf("\nQuitting!\n");
		return 0;
}
