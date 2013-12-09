//Things you should all have anyway.
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tgmath.h>

//Things you gotta get.
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>

#define obj 4

double posx[obj], velx[obj], accx[obj], forcex[obj];
double posy[obj], vely[obj], accy[obj], forcey[obj];
double posz[obj], velz[obj], accz[obj], forcez[obj];
double accprevx[obj], forcecouloumbx[obj], forcegravx[obj], diffx[obj];
double accprevy[obj], forcecouloumby[obj], forcegravy[obj], diffy[obj];
double accprevz[obj], forcecouloumbz[obj], forcegravz[obj], diffz[obj];
double mass[obj], dist[obj], mag[obj];
const double forceconstx = 0;
const double forceconsty = -9.8067;
const double gravconst = 6.67384;

float dt = 0.01;





int i, j, width = 1200, height = 480;



int verlet() {
	for(i = 1; i < obj; i++) {
		if(posx[i] > width || posx[i] < 0) {
			velx[i] = -velx[i];
		}
		if(posy[i] < 0 || posy[i] > height) {
			vely[i] = -vely[i];
		}
		posx[i] = posx[i] + (velx[i]*dt) + (accx[i])*((dt*dt)/2);
		posy[i] = posy[i] + (vely[i]*dt) + (accy[i])*((dt*dt)/2);
	}
	for(i = 1; i < obj; i++) {
		forcex[i] = forceconstx;
		forcey[i] = forceconsty;
	}
	//printf("Obj 0's Force: %f || Obj 1's Force: %f || Obj 2's Force: %f\n", (forcex[1]+forcey[1]), (forcex[2]+forcey[2]), (forcex[3]+forcey[3]));
	for(i = 1; i < obj; i++) {
		accprevx[i] = accx[i];
		accprevy[i] = accy[i];
		accx[i] = (forcex[i])/(mass[i]);
		accy[i] = (forcey[i])/(mass[i]);
	}
	for(i = 1; i < obj; i++) {
		velx[i] = velx[i] + ((dt)/2)*(accx[i] + accprevx[i]);
		vely[i] = vely[i] + ((dt)/2)*(accy[i] + accprevy[i]);
	}
	return 0;
}


int main() {
	//double (*pos)[dimensions] = malloc((dimensions) * sizeof *pos);
	//double (*vel)[dimensions] = malloc((dimensions) * sizeof *vel);
	posx[1] = 123;
	posy[1] = 321;
	posx[2] = 22;
	posy[2] = 22;
	posx[3] = 456;
	posy[3] = 112;
	velx[1] = 12;
	vely[1] = 44;
	velx[2] = 50;
	vely[2] = 42;
	velx[3] = 56;
	vely[3] = 21;
	mass[1] = 1;
	mass[2] = 1;
	mass[3] = 1;
	//res[0] = 0.02;
	//res[1] = 0.02;
	
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
		verlet();
		
		
		
		//MAIN OBJECTS, RECTANCLES, ETC.
		glBegin(GL_POINTS);
			for(i = 1; i < obj; i++) {
				glVertex3f( posx[i], posy[i], 1 );
			}
		glEnd();
		glBegin(GL_LINES);
			for(i = 1; i < obj; i++) {
				glVertex3f( posx[i], posy[i], 1 );
				glVertex3f( (posx[i] + velx[i]), (posy[i] + vely[i]), 1 );
			}
		glEnd();
		
		
		//TEXT N STUFF GOES IN HERE.
		txtRect.x = posx[1];
		txtRect.y = posy[1];
		
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


/*for(j = 1; j < obj; j++) {
			if(i != j) {
				diffx[i] = (posx[j] - posy[i]);
				diffy[i] = (posy[j] - posy[i]);
				dist[i] = sqrt(diffx[i]*diffx[i] + diffy[i]*diffy[i]);
				mag[i] = (gravconst/(pow(1, 10)))*((mass[i]*mass[j])/(dist[i]*dist[i]));
				forcegravx[i] = -(mag[i])*((diffx[i])/(dist[i]));
				forcegravy[i] = -(mag[i])*((diffy[i])/(dist[i]));
				forcex[i] = forceconstx + forcegravx[i];
				forcey[i] = forceconsty + forcegravy[i];
			}
		}*/
