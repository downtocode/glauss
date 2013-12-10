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
int obj = 6;
float posytemp;

int i, width = 1200, height = 600;
char text[25];

int main() {
	//SDL RELATED STUFF. A PAIN TO DEAL WITH.
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = SDL_CreateWindow( "Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
		SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);
		
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_GLContext glcontext = SDL_GL_CreateContext(window);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
		SDL_Event event;
		printf("OpenGL Version %s\n", glGetString(GL_VERSION));
		
		
		TTF_Init();
			atexit(TTF_Quit);
		TTF_Font *font;
		font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
		SDL_Rect txtRect ; // Store (x,y) of text for blit
		SDL_Color fColor ; // Font color (R,G,B)
		SDL_Surface *imgTxt;
		
		txtRect.w = 100;
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
	//SDL RELATED STUFF. JUST LOOK AT THE AMOUNT OF CODE.
	initphys();
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
					SDL_GL_DeleteContext(glcontext);
					SDL_DestroyWindow(window);
					SDL_Quit();
					return 0;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		posytemp = integrate();
		
		
		
		//MAIN OBJECTS, RECTANCLES, ETC.
		/*glBegin(GL_POINTS);
			for(i = 1; i < obj + 1; i++) {
				glVertex3f( object[i].pos[0], object[i].pos[1], 1 );
			}
		glEnd();
		glBegin(GL_LINES);
			for(i = 1; i < obj + 1; i++) {
				glVertex3f( object[i].pos[0], object[i].pos[1], 1 );
				glVertex3f( (object[i].pos[0] + object[i].vel[0]), (object[i].pos[1] + object[i].vel[1]), 1 );
			}
		glEnd();
		*/
		for(i = 1; i < obj + 1; i++) {
			//TEXT N STUFF GOES IN HERE.
			txtRect.x = 222;
			txtRect.y = posytemp;
			sprintf(text, "Posy: %f", posytemp);
			imgTxt = TTF_RenderText_Blended( font , text , fColor );
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, imgTxt);
			SDL_FreeSurface(imgTxt);
			SDL_RenderCopyEx(renderer, texture, NULL, &txtRect, 0, NULL, SDL_FLIP_VERTICAL);
		}
		
		
		
	//MORE SDL RELATED STUFF.
		SDL_RenderPresent(renderer);
		SDL_Delay(6);
	//MORE SDL RELATED STUFF.
	
	}
return 0;
}
