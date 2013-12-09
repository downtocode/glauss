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
int obj = 4;
int dimensions = 1;
double forceconst[2] = {0, -9.81};

int i, j, width = 1200, height = 480;
float dt = 0.01;



int integrate(double pos[][obj], double vel[][obj], double acc[][obj], double Ftot[][obj], double Fgrv[][obj], double Fele[][obj], double mass[obj]) {
	double *accprev = malloc(dimensions * sizeof *accprev);
	double *diff = malloc(dimensions * sizeof *diff);
	for(i = 1; i < obj + 1; i++) {
		//setting the bounds for a box. Do want a periodic boundary sometime down the road though...
		if(pos[i][0] > width || pos[i][0] < 0) {
			vel[i][0] = -vel[i][0];
		}
		if(pos[i][1] < 0 || pos[i][1] > height) {
			vel[i][1] = -vel[i][1];
		}
		for(j = 0; j < dimensions + 1; j++) {
			pos[i][j] = pos[i][j] + (vel[i][j]*dt) + (acc[i][j])*((dt*dt)/2);
		}
	}
	for(i = 1; i < obj + 1; i++) {
		for(j = 0; j < dimensions + 1; j++) {
			Ftot[i][j] = forceconst[j];
			//lets just naively assume this for now, k?
		}
	}
	for(i = 1; i < obj + 1; i++) {
		for(j = 0; j < dimensions + 1; j++) {
			accprev[j] = acc[i][j];
			acc[i][j] = (Ftot[i][j])/(mass[i]);
			vel[i][j] = vel[i][j] + ((dt)/2)*(acc[i][j] + accprev[j]);
			//Originally velocity was calculated in a loop below, but it's fine here and we save an extra dimension in accprev array.
		}
	}
	return 0;
}


int main() {
	double (*pos)[obj] = malloc(100*(obj*dimensions) * sizeof *pos);
	double (*vel)[obj] = malloc(100*(obj*dimensions) * sizeof *vel);
	double (*acc)[obj] = malloc(100*(obj*dimensions) * sizeof *acc);
	double (*Ftot)[obj] = malloc(100*(obj*dimensions) * sizeof *Ftot);
	double (*Fgrv)[obj] = malloc(100*(obj*dimensions) * sizeof *Fgrv);
	double (*Fele)[obj] = malloc(100*(obj*dimensions) * sizeof *Fele);
	double *mass = malloc(obj * sizeof *mass);
	
	mass[1] = 1;
	pos[1][0] = 11;
	pos[1][1] = 31;
	vel[1][0] = 12;
	vel[1][1] = 44;
	
	mass[2] = 1;
	pos[2][0] = 22;
	pos[2][1] = 222;
	vel[2][0] = 22;
	vel[2][1] = 5;
	
	mass[3] = 1;
	pos[3][0] = 41;
	pos[3][1] = 112;
	vel[3][0] = 122;
	vel[3][1] = 23;
	
	
	
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
		SDL_Surface *imgTxt ; // Store image of the text for blit
		SDL_Rect txtRect ; // Store (x,y) of text for blit
		SDL_Color fColor ; // Font color (R,G,B)
		
		
		txtRect.w = 200;
		txtRect.h = 20;
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
					SDL_FreeSurface(imgTxt);
					SDL_GL_DeleteContext(glcontext);
					SDL_DestroyWindow(window);
					SDL_Quit();
					return 0;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		integrate(pos, vel, acc, Ftot, Fgrv, Fele, mass);
		
		
		
		//MAIN OBJECTS, RECTANCLES, ETC.
		glBegin(GL_POINTS);
			for(i = 1; i < obj + 1; i++) {
				glVertex3f( pos[i][0], pos[i][1], 1 );
			}
		glEnd();
		glBegin(GL_LINES);
			for(i = 1; i < obj + 1; i++) {
				glVertex3f( pos[i][0], pos[i][1], 1 );
				glVertex3f( (pos[i][0] + vel[i][0]), (pos[i][1] + vel[i][1]), 1 );
			}
		glEnd();
		
		
		//TEXT N STUFF GOES IN HERE.
		txtRect.x = pos[1][0];
		txtRect.y = pos[1][1];
		printf("Obj1x = %f, Obj1y = %f\n", pos[1][0], pos[1][1]);
		
		imgTxt = TTF_RenderText_Blended( font , "HEAVY METAL" , fColor );
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, imgTxt);
		
		
		
		
	//MORE SDL RELATED STUFF.
		SDL_RenderCopyEx(renderer, texture, NULL, &txtRect, 0, NULL, SDL_FLIP_VERTICAL);
		SDL_RenderPresent(renderer);
		SDL_Delay(6);
	//MORE SDL RELATED STUFF.
	
	}
return 0;
}
