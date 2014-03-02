/*	Standard header files	*/
#include <stdio.h>
#include <tgmath.h>
#include <sys/time.h>

/*	Dependencies	*/
#include <SDL2/SDL.h>

/*	Functions	*/
#include "physics.h"
#include "parser.h"
#include "glfun.h"

/*	Default settings.	*/
int obj = 0, width = 1200, height = 600;
float boxsize = 0.1;
char fontname[200] = "./resources/fonts/DejaVuSansMono.ttf";
char filename[200] = "posdata.dat";
float dt = 0.008, radius = 12.0;
long double elcharge = 0, gconst = 0, epsno = 0;
bool novid = 0, vsync = 1, quiet = 0, stop = 0, enforced = 0, nowipe = 0, random = 1, flicked = 0;
unsigned int chosen = 0;
unsigned short int avail_cores = 0;


//Object shader global vars
GLuint programObj = 3;
GLint u_matrix = -1;
GLint objattr_pos = 0, objattr_color = 1;

//Text shader global vars
GLuint programText = 6;
GLint textattr_coord = 2, textattr_texcoord = 4, textattr_tex = 3, textattr_color = 5;

GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0, scalefactor = 1.0;
GLfloat rotatex = 0.0, rotatey = 0.0, rotatez = 0.0;
GLfloat chosenbox[4][2];


int main(int argc, char *argv[])
{
	/*	Main function vars	*/
		int linkcount;
		struct timeval t1, t2;
		float deltatime, totaltime = 0.0f, fps;
		unsigned int frames = 0;
		char osdtext[500] = "FPS = inf";
	/*	Main function vars	*/
	
	/*	Arguments	*/
		if(argc > 1) {
			for(int i=1; i < argc; i++) {
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
		obj = preparser();
	/*	Arguments	*/
	
	/*	Error handling.	*/
		if(obj == 0) {
			printf("ERROR! NO OBJECTS!\n");
			return 1;
		}
	/*	Error handling.	*/
	
	/*	OpenGL ES 2.0 + SDL2	*/
		GLuint tex, textvbo, linevbo, pointvbo;
		
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if(novid == 0) {
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			/*	SDL bugs again. Set the line below to anything and you'll render on only half of your screen.	*/
			/*	For now, sticking to regular, good, awesome OpenGL.	*/
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			window = SDL_CreateWindow("Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL);
			SDL_GL_CreateContext(window);
			SDL_GL_SetSwapInterval(vsync);
			glViewport(0, 0, width, height);
			create_shaders();
			glActiveTexture(GL_TEXTURE0);
			glGenBuffers(1, &textvbo);
			glGenBuffers(1, &linevbo);
			glGenBuffers(1, &pointvbo);
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glUniform1i(textattr_tex, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glClearColor(0.1, 0.1, 0.1, 1);
		}
		
		SDL_Event event;
		int mousex, mousey, initmousex, initmousey;
		if(quiet == 0) {
			printf("OpenGL Version %s\n", glGetString(GL_VERSION));
		}
		
	/*	OpenGL ES 2.0 + SDL2	*/
	
	/*	Freetype.	*/
		if(FT_Init_FreeType(&library)) {
			fprintf(stderr, "Could not init freetype library\n");
			return 1;
		}
		if(FT_New_Face(library, fontname, 0, &face)) fprintf(stderr, "Could not open font\n");
		FT_Set_Pixel_Sizes(face, 0, 40);
		g = face->glyph;
	/*	Freetype.	*/
	
	/*	Physics.	*/
		data* object;
		if(quiet == 0) {
			printf("Objects: %i\n", obj);
			printf("Settings: dt=%f, widith=%i, height=%i, boxsize=%f, fontname=%s\n", dt, width, height, boxsize, fontname);
			printf("Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n", elcharge, gconst, epsno);
		}
		
		/*	Mallocs and wipes	*/
		initphys(&object);
		parser(&object, filename);
	/*	Physics.	*/
	
	gettimeofday (&t1 , NULL);
	
	linkcount = obj*2;
	
	while( 1 ) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						flicked = 1;
						SDL_ShowCursor(0);
						SDL_GetMouseState(&initmousex, &initmousey);
						SDL_SetRelativeMouseMode(1);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.button == SDL_BUTTON_LEFT) {
						flicked = 0;
						SDL_SetRelativeMouseMode(0);
						SDL_WarpMouseInWindow(window, initmousex, initmousey);
						SDL_ShowCursor(1);
					}
					break;
				case SDL_MOUSEWHEEL:
					scalefactor += (float)event.wheel.y/10;
					break;
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
					if(event.key.keysym.sym==SDLK_w) {
						view_rotx += 5.0;
					}
					if(event.key.keysym.sym==SDLK_s) {
						view_rotx -= 5.0;
					}
					if(event.key.keysym.sym==SDLK_a) {
						view_roty += 5.0;
					}
					if(event.key.keysym.sym==SDLK_d) {
						view_roty -= 5.0;
					}
					if(event.key.keysym.sym==SDLK_n) {
						if(nowipe == 1) nowipe = 0;
						else nowipe = 1;
					}
					if(event.key.keysym.sym==SDLK_2) {
						if(chosen < obj) chosen++;
					}
					if(event.key.keysym.sym==SDLK_1) {
						if(chosen > 0) chosen--;
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
			if (totaltime >  1.0f) {
				fps = frames/totaltime;
				if(novid == 0) {
					sprintf(osdtext, "FPS = %3.2f", fps);
				} else {
					printf("Current FPS = %3.2f\n", fps);
				}
				totaltime -= 1.0f;
				frames = 0;
			}
		}
		if(flicked == 1) {
			SDL_GetRelativeMouseState(&mousex, &mousey);
			view_roty += (float)mousex/4;
			view_rotx += (float)mousey/4;
		}
		if(novid == 1) continue;
		if(nowipe == 0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glUseProgram(programObj);
		
		/*	Point/object drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, pointvbo);
		for(int i = 1; i < obj + 1; i++) drawobject(object[i]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Point/object drawing	*/
		
		/*	Link drawing	*/
		GLfloat link[linkcount][3];
		linkcount = 0;
		
		for(int i = 1; i < obj + 1; i++) {
			for(int j = 1; j < obj + 1; j++) {
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
		
		glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, link);
		glEnableVertexAttribArray(objattr_pos);
		glDrawArrays(GL_LINES, 0, linkcount+1);
		glDisableVertexAttribArray(objattr_pos);
		/*	Link drawing	*/
		
		//glUseProgram(0);
		
		glUseProgram(programText);
		
		/*	Selected object's red box	*/
		for(int i = 1; i < obj + 1; i++) {
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
			glBindBuffer(GL_ARRAY_BUFFER, linevbo);
			glEnableVertexAttribArray(textattr_coord);
			glVertexAttribPointer(textattr_coord, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glBufferData(GL_ARRAY_BUFFER, sizeof chosenbox, chosenbox, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_LINE_LOOP, 0, 4);
			glDisableVertexAttribArray(textattr_coord);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		/*	Selected object's red box	*/
		
		/*	Text drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, textvbo);
		render_text(osdtext, -0.95, 0.85, 1.0/width, 1.0/height);
		for(int i = 1; i < obj + 1; i++) {
			if(chosen==i) {
				char osdstr[500];
				sprintf(osdstr, "Object %i", i);
				
				render_text(osdstr, object[i].pos[0] + object[i].radius, \
				object[i].pos[1] + object[i].radius, 1.0/width, 1.0/height);
				
				unsigned int counter = 0;
				unsigned int links[obj+1];
				
				for(int j = 1; j < obj + 1; j++) {
					if(object[i].linkwith[j] != 0) {
						counter++;
						links[counter] = j;
					}
				}
				if(counter != 0) {
					memset(osdstr, 0, sizeof(osdstr));
					char linkcount[obj+1];
					sprintf(osdstr, "Links: ");
					for(int j = 1; j < counter + 1; j++) {
						sprintf(linkcount, "%u, ", links[j]);
						strcat(osdstr, linkcount);
					}
					render_text(osdstr, object[i].pos[0] + object[i].radius, \
					object[i].pos[1] + object[i].radius - 0.075, 1.0/width, 1.0/height);
				}
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Text drawing	*/
		//glUseProgram(0);
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		SDL_DestroyWindow(window);
		SDL_Quit();
		if(quiet == 0) printf("Quitting!\n");
		pthread_exit(NULL);
		free(object);
		return 0;
}
