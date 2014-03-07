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
#include "toxyz.h"

/*	Default settings.	*/
unsigned int obj = 0, width = 1200, height = 600;
float boxsize = 0.1;
char fontname[200] = "./resources/fonts/DejaVuSansMono.ttf";
char filename[200] = "posdata.dat";
float dt = 0.008, radius = 12.0;
long sleepfor = 16;
long double elcharge = 0, gconst = 0, epsno = 0;
bool novid = 0, vsync = 1, quiet = 0, stop = 0, enforced = 0, quit = 0;
bool nowipe = 0, random = 1, flicked = 0, dumped = 0, fullogl = 0, restart = 0;
unsigned int chosen = 0, dumplevel = 0;
unsigned short int avail_cores = 0, oglmin = 2, oglmax = 0;


//glfun global vars
GLuint programObj;
GLuint programText;
GLint textattr_tex;


GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0, scalefactor = 1.0;
GLfloat rotatex = 0.0, rotatey = 0.0, rotatez = 0.0;


int main(int argc, char *argv[])
{
	/*	Main function vars	*/
		unsigned int linkcount;
		struct timeval t1, t2;
		float deltatime, totaltime = 0.0f, fps;
		unsigned int frames = 0;
		char osdfps[500] = "FPS = n/a";
		char osdobj[500] = "Objects = n/a";
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
				if(!strcmp( "--fullogl", argv[i])) {
					fullogl = 1;
				}
				if(!strcmp("--threads", argv[i])) {
					sscanf(argv[i+1], "%hu", &avail_cores);
					if(avail_cores == 0) {
						fprintf(stderr, "WARNING! Running with 0 cores disables all force calculations. Press Enter to continue.");
						while(getchar() != '\n');
						enforced = 1;
					}
				}
				if(!strcmp("--dumplevel", argv[i])) {
					sscanf(argv[i+1], "%u", &dumplevel);
				}
				if( !strcmp("--help", argv[i])) {
					printf("Usage:\n");
					printf("	-f (filename)		Specify a posdata file. Takes priority over configfile.\n");
					printf("	--novid 		Disable video output, do not initialize any graphical libraries.\n");
					printf("	--fullogl 		Initialize full OpenGL instead of ES.\n");
					printf("	--threads (int)		Make the program run with this many threads.\n"); 
					printf("	--dumplevel (uint)		Set the dumplevel. 1=XYZ file every second. 2=every frame.\n"); 
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
		} else sprintf(osdobj, "Objects = %i", obj);
		if(dumplevel && !quiet) printf("Outputting XYZ file every second.\n");
		if(dumplevel == 2) fprintf(stderr, "Printing XYZ file every frame!\n");
	/*	Error handling.	*/
	
	/*	OpenGL ES 2.0 + SDL2	*/
		GLuint tex, textvbo, linevbo, pointvbo, linkvbo;
		
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if(novid == 0) {
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			if(!fullogl) SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, oglmin);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, oglmax);
			window = SDL_CreateWindow("Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL);
			SDL_GL_CreateContext(window);
			SDL_GL_SetSwapInterval(vsync);
			glViewport(0, 0, width, height);
			create_shaders();
			glActiveTexture(GL_TEXTURE0);
			glGenBuffers(1, &textvbo);
			glGenBuffers(1, &linevbo);
			glGenBuffers(1, &pointvbo);
			glGenBuffers(1, &linkvbo);
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
			printf("Settings: dt=%f, widith=%i, height=%i, boxsize=%f, fontname=%s\n" \
			, dt, width, height, boxsize, fontname);
			printf("Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n" \
			, elcharge, gconst, epsno);
		}
		/*	Mallocs and wipes	*/
		initphys(&object);
		parser(&object, filename);
	/*	Physics.	*/
	
	gettimeofday (&t1 , NULL);
	
	pthread_t physthread;
	pthread_create(&physthread, NULL, integrate, (void*)(long)sleepfor);
	
	while( 1 ) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						flicked = 1;
						SDL_ShowCursor(0);
						SDL_GetMouseState(&initmousex, &initmousey);
						SDL_SetRelativeMouseMode(1);
						SDL_GetRelativeMouseState(NULL, NULL);
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
					if(scalefactor < 0) scalefactor = 0;
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_SPACE) {
						if( stop == 0 ) stop = 1;
						else if( stop == 1 ) {
							stop = 0;
							restart = 1;
						}
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
					if(event.key.keysym.sym==SDLK_z) {
						if(dumped == 0) {
							toxyz(obj, object);
							dumped = 1;
						}
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
		if(stop && !restart) {
			quit = 1;
			pthread_join(physthread, NULL);
		} else if(restart) {
			quit = 0;
			pthread_create(&physthread, NULL, integrate, (void*)(long)sleepfor);
			stop = 0;
			restart = 0;
		}
		if(!quiet) {
			gettimeofday(&t2, NULL);
			deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
			t1 = t2;
			totaltime += deltatime;
			frames++;
			if(dumplevel == 2) toxyz(obj, object);
			if (totaltime >  1.0f) {
				fps = frames/totaltime;
				if(novid == 0) {
					sprintf(osdfps, "FPS = %3.2f", fps);
				} else {
					printf("Current FPS = %3.2f\n", fps);
				}
				totaltime -= 1.0f;
				frames = 0;
				//To prevent excessive dumps.
				dumped = 0;
				if(dumplevel == 1) toxyz(obj, object);
			}
		}
		if(flicked) {
			SDL_GetRelativeMouseState(&mousex, &mousey);
			view_roty += (float)mousex/4;
			view_rotx += (float)mousey/4;
		}
		if(novid) continue;
		if(!nowipe) glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		glUseProgram(programObj);
		
		/*	Point/object drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, pointvbo);
		for(int i = 1; i < obj + 1; i++) drawobject(object[i]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Point/object drawing	*/
		
		/*	Link drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, linkvbo);
		for(int i = 1; i < obj + 1; i++) {
			for(int j = 1; j < obj + 1; j++) {
				if( j==i || j > i ) continue;
				if( object[i].linkwith[j] != 0 ) linkcount++;
			}
		}
		drawlinks(object, 2*linkcount);
		linkcount = 0;
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Link drawing	*/
		
		glUseProgram(programText);
		
		/*	Selected object's red box	*/
		glBindBuffer(GL_ARRAY_BUFFER, linevbo);
		if(chosen != 0) selected_box_text(object[chosen]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Selected object's red box	*/
		
		/*	Text drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, textvbo);
		render_text(osdfps, -0.95, 0.85, 1.0/width, 1.0/height, 0);
		render_text(osdobj, -0.95, 0.75, 1.0/width, 1.0/height, 0);
		if(stop == 1) render_text("Simulation stopped", -0.95, -0.95, 1.0/width, 1.0/height, 1);
		for(int i = 1; i < obj + 1; i++) {
			if(chosen==i) {
				char osdstr[500];
				sprintf(osdstr, "Object %i", i);
				
				render_text(osdstr, object[i].pos[0] + object[i].radius, \
				object[i].pos[1] + object[i].radius, 1.0/width, 1.0/height, 0);
				
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
					object[i].pos[1] + object[i].radius - 0.075, 1.0/width, 1.0/height, 0);
				}
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Text drawing	*/
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		quit = 1;
		pthread_join(physthread, NULL);
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		SDL_DestroyWindow(window);
		SDL_Quit();
		if(quiet == 0) printf("Quitting!\n");
		pthread_exit(NULL);
		free(object);
		return 0;
}
