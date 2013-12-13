//Things you should all have anyway.
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tgmath.h>

//Things you gotta get.
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>

//Functions.
#include "physics.h"


//For now, let's use obj-1 and dimensions-1. Hacky, but it works.
static int i;
int obj = 9, width = 1200, height = 600;

//int i, width = 1200, height = 600;
char text[100];

int main() {
	/*	SDL RELATED STUFF. A PAIN TO DEAL WITH.	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = SDL_CreateWindow( "Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
		SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);
		
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_GLContext glcontext = SDL_GL_CreateContext(window);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
		SDL_Event event;
		printf("OpenGL Version %s\n", glGetString(GL_VERSION));
		
		TTF_Init();
		TTF_Font *font;
		font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
		SDL_Rect txtRect ; // Store (x,y) of text for blit
		SDL_Color fColor ; // Font color (R,G,B)
		SDL_Surface *imgTxt;
		
		txtRect.w = 137;
		txtRect.h = 25;
		fColor.r = 255;
		fColor.g = 165;
		fColor.b = 0;
		
		glClearColor(0,0,0,1);
		glPointSize(12.0);
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glViewport( 0,0, width, height );
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( 0.0, width, 0.0, height, 1.0, -1.0 );
		glClear(GL_COLOR_BUFFER_BIT);
	/*	SDL RELATED STUFF. JUST LOOK AT THE AMOUNT OF CODE.	*/
	
	
	
	/*	PHYSICS RELATED STUFF.	*/
		data* object;
		/*	Okay, some explanation is needed. We want to make an array containing object parameters. So fine, we make it as a pointer.
			Then we call a parser program to read in our object data and dump it into the struct of object parameters. We pass the memory
			address to it so it can screw around with it. Then after it does whatever it's supposed to, we run the integration in the main loop.	*/
		initphys(&object);
	
	
	while( 1 ) {
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
				case SDL_WINDOWEVENT:
					if( event.window.event == SDL_WINDOWEVENT_RESIZED ) {
						width = event.window.data1;
						height = event.window.data2;
						glViewport( 0,0, event.window.data1, event.window.data2 );
						glMatrixMode( GL_PROJECTION );
						glLoadIdentity();
						glOrtho( 0.0, event.window.data1, 0.0, event.window.data2, 1.0, -1.0 );
						}
					break;
				case SDL_QUIT:
					free(object);
					TTF_Quit();
					SDL_GL_DeleteContext(glcontext);
					SDL_DestroyWindow(window);
					SDL_Quit();
					return 0;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		
		integrate(object);
		
		
		//MAIN OBJECTS, RECTANCLES, ETC.
		glBegin(GL_POINTS);
			for(i = 1; i < obj + 1; i++) {
				if( object[i].charge > 0 ) glColor3f(255,0,0);
				else glColor3f(0,0,255);
				glVertex3f( object[i].pos[0], object[i].pos[1], 1 );
			}
		glEnd();
		glColor3f(255,255,255);
		glBegin(GL_LINES);
			for(i = 1; i < obj + 1; i++) {
				glVertex3f( object[i].pos[0], object[i].pos[1], 1 );
				glVertex3f( (object[i].pos[0] + object[i].vel[0]), (object[i].pos[1] + object[i].vel[1]), 1 );
			}
		glEnd();
		/*for(i = 1; i < obj + 1; i++) {
				txtRect.x = object[i].pos[0];
				txtRect.y = object[i].pos[1];
				sprintf(text, "X: %0.2f, Y: %0.2f", object[i].pos[0], object[i].pos[1]);
				imgTxt = TTF_RenderText_Solid( font , text , fColor );
				SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, imgTxt);
				SDL_FreeSurface(imgTxt);
				SDL_RenderCopyEx(renderer, texture, NULL, &txtRect, 0, NULL, SDL_FLIP_VERTICAL);
		}*/
		
		
		
	//MORE SDL RELATED STUFF.
		SDL_RenderPresent(renderer);
		SDL_Delay(5);
	//MORE SDL RELATED STUFF.
	
	}
return 0;
}
