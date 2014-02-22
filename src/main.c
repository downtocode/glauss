/*	Standard header files	*/
#include <stdio.h>
#include <tgmath.h>
#include <sys/time.h>

/*	Dependencies	*/
#include <ft2build.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include FT_FREETYPE_H

/*	Functions	*/
#include "physics.h"
#include "parser.h"
#include "glfun.h"


FT_Library  library;
FT_Face face;

static int i, j, linkcount;

/*	Default settings.	*/
int obj = 0, width = 1200, height = 600;
float boxsize = 0.1;
char fontname[200] = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
char filename[200] = "posdata.dat";
float dt = 0.008, radius = 12.0;
long double elcharge = 0, gconst = 0, epsno = 0;
bool novid = 0, vsync = 1, quiet = 0, stop = 0, enforced = 0, nowipe = 0;
unsigned int chosen = 0;
unsigned short int avail_cores = 0;

/*	FPS Measurement	*/
struct timeval t1, t2;
float deltatime;
float totaltime = 0.0f;
unsigned int frames = 0;
char title[40];

GLint u_matrix = -1;
GLint attr_pos = 0, attr_color = 1;
GLfloat view_rotx = 0.0, view_roty = 0.0;
GLfloat chosenbox[4][2];

GLfloat colors[3] = {1.0f, 1.0f,  1.0f};

int main(int argc, char *argv[])
{
	/*	ARGUMENT SETTING	*/
	if(argc > 1) {
		for(i=1; i < argc; i++) {
			if(!strcmp( "--novid", argv[i])) {
				novid = 1;
			}
			if(!strcmp( "--quiet", argv[i])) {
				quiet = 1;
			}
			if(!strcmp( "-f", argv[i] ) ) {
				strcpy( filename, argv[i+1]);
			}
			if(!strcmp( "--nosync", argv[i])) {
				vsync = 0;
			}
			if(!strcmp("--threads", argv[i])) {
				sscanf(argv[i+1], "%hu", &avail_cores);
				if(avail_cores == 0) {
					fprintf(stderr, "WARNING! Running with 0 cores disables all force calculations. Press Enter to continue.");
					while(getchar() != '\n');
					enforced = 1;
				}
			}
			if( !strcmp("--help", argv[i])) {
				printf("Usage:\n");
				printf("	-f (filename)		Specify a posdata file. Takes priority over configfile.\n");
				printf("	--novid 		Disable video output, do not initialize any graphical libraries.\n");
				printf("	--nosync		Disable vsync, render everything as fast as possible.\n"); 
				printf("	--threads (int)		Make the program run with this many threads.\n"); 
				printf("	--quiet 		Disable any terminal output except errors.\n"); 
				printf("	--help  		What you're reading.\n");
				return 0;
			}
		}
	}
	obj = preparser(&dt, &elcharge, &gconst, &epsno, &width, &height, &boxsize, fontname, filename);
	/*	ARGUMENT SETTING	*/
	
	/*	Error handling.	*/
		if(obj == 0) {
			printf("ERROR! NO OBJECTS!\n");
			return 1;
		}
	/*	Error handling.	*/
	
	/*	OGL && EGL	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if(novid == 0) {
			window = SDL_CreateWindow("Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetSwapInterval(vsync);
			SDL_GL_CreateContext(window);
		}
		SDL_Event event;
		if(quiet == 0) {
			printf("OpenGL Version %s\n", glGetString(GL_VERSION));
		}
		glViewport(0, 0, width, height);
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
		if(quiet == 0) {
			printf("Objects: %i\n", obj);
			printf("Settings: dt=%f, widith=%i, height=%i, boxsize=%f, fontname=%s\n", dt, width, height, boxsize, fontname);
			printf("Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n", elcharge, gconst, epsno);
		}
		
		/*	Mallocs and wipes	*/
		initphys(&object);
		
		parser(&object, filename);
	/*	PHYSICS.	*/
	
	gettimeofday (&t1 , NULL);
	
	linkcount = obj*2;
	
	while( 1 ) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_SPACE) {
						if( stop == 0 ) stop = 1;
						else if( stop == 1 ) stop = 0;
					}
					if(event.key.keysym.sym==SDLK_ESCAPE) {
						goto quit;
					}
					if(event.key.keysym.sym==SDLK_RIGHTBRACKET) {
						dt *= 2;
						printf("dt = %f\n", dt);
					}
					if(event.key.keysym.sym==SDLK_LEFTBRACKET) {
						dt /= 2;
						printf("dt = %f\n", dt);
					}
					if(event.key.keysym.sym==SDLK_q) {
						view_rotx += 5.0;
					}
					if(event.key.keysym.sym==SDLK_e) {
						view_rotx -= 5.0;
					}
					if(event.key.keysym.sym==SDLK_n) {
						if(nowipe == 1) nowipe = 0;
						else nowipe = 1;
					}
					if(event.key.keysym.sym==SDLK_2) {
						chosen++;
					}
					if(event.key.keysym.sym==SDLK_1) {
						chosen--;
					}
					break;
				case SDL_QUIT:
					goto quit;
					break;
			}
		}
		if(stop == 0) integrate(object);
		if(quiet == 0) {
			gettimeofday(&t2, NULL);
			deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
			t1 = t2;
			totaltime += deltatime;
			frames++;
			if (totaltime >  2.0f) {
				if(novid == 0) {
					sprintf(title, "Physengine - %3.2f FPS", frames/totaltime);
					SDL_SetWindowTitle(window, title);
				} else {
					printf("Current FPS = %3.2f\n", frames/totaltime);
				}
				totaltime -= 2.0f;
				frames = 0;
			}
		}
		if(novid == 1) continue;
		
		if(nowipe == 0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		draw();
		
		/*	Link drawing	*/
		GLfloat link[linkcount][3];
		linkcount = 0;
		
		for(i = 1; i < obj + 1; i++) {
			for(j = 1; j < obj + 1; j++) {
				if( j==i || j > i ) continue;
				if( object[i].linkwith[j] != 0 ) {
					link[linkcount][0] = object[i].pos[0];
					link[linkcount][1] = object[i].pos[1];
					link[linkcount][2] = object[i].pos[2];
					linkcount++;
					link[linkcount][0] = object[j].pos[0];
					link[linkcount][1] = object[j].pos[1];
					link[linkcount][2] = object[j].pos[2];
					linkcount++;
				}
			}
		}
		
		
		glVertexAttribPointer(attr_pos, 3, GL_FLOAT, GL_FALSE, 0, link);
		glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
		glEnableVertexAttribArray(attr_pos);
		glEnableVertexAttribArray(attr_color);
		glDrawArrays(GL_LINES, 0, linkcount);
		glDisableVertexAttribArray(attr_pos);
		glDisableVertexAttribArray(attr_color);
		/*	Link drawing	*/
		
		/*	Selected object's red box	*/
		for(i = 1; i < obj + 1; i++) {
			if(chosen==i) {
				chosenbox[0][0] = object[i].pos[0] - boxsize;
				chosenbox[0][1] = object[i].pos[1] - boxsize;
				chosenbox[1][0] = object[i].pos[0] - boxsize;
				chosenbox[1][1] = object[i].pos[1] + boxsize;
				chosenbox[2][0] = object[i].pos[0] + boxsize;
				chosenbox[2][1] = object[i].pos[1] + boxsize;
				chosenbox[3][0] = object[i].pos[0] + boxsize;
				chosenbox[3][1] = object[i].pos[1] - boxsize;
			}
		}
		
		if(chosen != 0) {
			glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, chosenbox);
			glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
			glEnableVertexAttribArray(attr_pos);
			glEnableVertexAttribArray(attr_color);
			glDrawArrays(GL_LINE_LOOP, 0, 4);
			glDisableVertexAttribArray(attr_pos);
			glDisableVertexAttribArray(attr_color);
		}
		/*	Selected object's red box	*/
		
		/*	Point/object drawing	*/
		GLfloat points[obj+1][3];
		for(i = 1; i < obj + 1; i++) {
			points[i-1][0] = object[i].pos[0];
			points[i-1][1] = object[i].pos[1];
			points[i-1][2] = object[i].pos[2];
		}
		
		glVertexAttribPointer(attr_pos, 3, GL_FLOAT, GL_FALSE, 0, points);
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
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		SDL_DestroyWindow(window);
		SDL_Quit();
		printf("Quitting!\n");
		pthread_exit(NULL);
		free(object);
		return 0;
}
