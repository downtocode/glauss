//Things you should all have anyway.
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tgmath.h>

//Things you gotta get.
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>


//For now, let's use obj-1 and dimensions-1. Hacky, but it works.
int obj = 6;
int dimensions = 1;


int i, j, width = 1200, height = 600;
float dt = 0.01;
char text[25];


typedef float v4sf __attribute__ ((vector_size (16)));
v4sf forceconst = {0, -9.81};


typedef struct{
v4sf pos, vel, acc, Ftot, Fgrv, Fele;
double mass;
}data;

data object[8];
v4sf accprev, diff;


int integrate() {
	for(i = 1; i < obj + 1; i++) {
		//setting the bounds for a box. Do want a periodic boundary sometime down the road though...
		if(object[i].pos[0] > width || object[i].pos[0] < 0) {
			object[i].vel[0] = -object[i].vel[0];
		}
		if(object[i].pos[1] < 0 || object[i].pos[1] > height) {
			object[i].vel[1] = -object[i].vel[1];
		}
		
		object[i].pos = object[i].pos + (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		
		object[i].Ftot = forceconst;
		//lets just naively assume this for now, k?
		
		accprev = object[i].acc;
		object[i].acc = (object[i].Ftot);
		object[i].vel = object[i].vel + ((dt)/2)*(object[i].acc + accprev);
	}
	return 0;
}


int main() {
	object[1].mass = 1;
	object[1].pos[0] = 11;
	object[1].pos[1] = 31;
	object[1].vel[0] = 12;
	object[1].vel[1] = 44;
	
	object[2].mass = 1;
	object[2].pos[0] = 22;
	object[2].pos[1] = 222;
	object[2].vel[0] = 22;
	object[2].vel[1] = 5;
	
	object[3].mass = 1;
	object[3].pos[0] = 41;
	object[3].pos[1] = 112;
	object[3].vel[0] = 12;
	object[3].vel[1] = 23;
	
	object[4].mass = 1;
	object[4].pos[0] = 56;
	object[4].pos[1] = 30;
	object[4].vel[0] = -12;
	object[4].vel[1] = -26;
	
	object[5].mass = 1;
	object[5].pos[0] = 81;
	object[5].pos[1] = 2;
	object[5].vel[0] = -21;
	object[5].vel[1] = 23;
	
	object[6].mass = 1;
	object[6].pos[0] = 221;
	object[6].pos[1] = 222;
	object[6].vel[0] = 52;
	object[6].vel[1] = 53;
	
	
	
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
		integrate();
		
		
		
		//MAIN OBJECTS, RECTANCLES, ETC.
		glBegin(GL_POINTS);
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
		
		for(i = 1; i < obj + 1; i++) {
			//TEXT N STUFF GOES IN HERE.
			txtRect.x = object[i].pos[0];
			txtRect.y = object[i].pos[1];
			sprintf(text, "Ek: %0.2f", object[i].mass*(sqrt(object[i].vel[0]*object[i].vel[0] + object[i].vel[1]*object[i].vel[1]))/2);
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
