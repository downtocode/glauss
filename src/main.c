//Things you should all have anyway.
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tgmath.h>

//Things you gotta get.
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

//Functions.
#include "physics.h"
#include "parser.h"


static int i, j;
int obj, width = 1200, height = 600;
int mousex, mousey;

char text[100];

v4sf vectemp;

int main() {
	/*	SDL.	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = SDL_CreateWindow( "Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL);
		SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);
		
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_GLContext glcontext = SDL_GL_CreateContext(window);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
		SDL_Event event;
		printf("OpenGL Version %s\n", glGetString(GL_VERSION));
	/*	SDL.	*/
		
	/*	OpenGL.	*/
		glClearColor(0,0,0,1);
		glPointSize(12.0);
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glViewport( 0,0, width, height );
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( 0.0, width, 0.0, height, 1.0, -1.0 );
		glClear(GL_COLOR_BUFFER_BIT);
	/*	OpenGL.	*/
	
	/*	PHYSICS.	*/
		data* object;
		/*	Okay, some explanation is needed. We want to make an array containing object parameters. So fine, we make it as a pointer.
			Then we call a parser program to read in our object data and dump it into the struct of object parameters. We pass the memory
			address to it so it can screw around with it. Then after it does whatever it's supposed to, we run the integration in the main loop.	*/
		obj = preparser();
		initphys(&object);
		parser(&object);
	/*	PHYSICS.	*/
	
	
	while( 1 ) {
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
				case SDL_MOUSEMOTION:
					SDL_GetMouseState(&mousex , &mousey);
					object[9].pos[0] = (float)mousex;
					object[9].pos[1] = ((float)height/2 - (float)mousey) + (float)height/2;
					object[9].vel = (v4sf){0, 0};
					break;
				case SDL_QUIT:
					free(object);
					SDL_GL_DeleteContext(glcontext);
					SDL_DestroyWindow(window);
					SDL_Quit();
					return 0;
			}
		}
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		integrate(object);

		glBegin(GL_POINTS);
			for(i = 1; i < obj + 1; i++) {
				if( object[i].charge < 0 ) glColor3f(0,0,255);
				if( object[i].charge == 0 ) glColor3f(255,255,255);
				if( object[i].charge > 0 ) glColor3f(255,0,0);
				glVertex3f( object[i].pos[0], object[i].pos[1], 1 );
			}
		glEnd();
		glColor3f(255,255,255);
		glBegin(GL_LINES);
			for(i = 1; i < obj + 1; i++) {
				for(j = 1; j < obj + 1; j++) {
					if( object[i].linkwith[j] == 1 ) { 
						vectemp =  object[j].pos - object[i].pos;
						glVertex3f( object[i].pos[0], object[i].pos[1], 1 );
						glVertex3f( (object[i].pos[0] + vectemp[0]), (object[i].pos[1] + vectemp[1]), 1 );
					}
				}
			}
		glEnd();
		
		
		SDL_RenderPresent(renderer);
		SDL_Delay(5);
	
	}
return 0;
}
