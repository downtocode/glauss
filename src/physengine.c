/*	Standard header files	*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <tgmath.h>
#include <sys/time.h>

/*	Dependencies	*/
#include <SDL2/SDL.h>

/*	Functions	*/
#include "physics.h"
#include "parser.h"
#include "glfun.h"
#include "toxyz.h"
#include "options.h"
#include "msg_phys.h"

int main(int argc, char *argv[])
{
	/*	Default settings.	*/
		option = calloc(1, sizeof(*option));
		
		 *option = (struct option_struct){
			.width = 1200, .height = 600,
			.avail_cores = 0, .oglmin = 2, .oglmax = 0,
			.dt = 0.008, .vsync = 1, .verbosity = 5,
		};
		strcpy(option->filename,"nanotubes.dat");
		strcpy(option->fontname,"./resources/fonts/DejaVuSansMono.ttf");
	/*	Default settings.	*/
	
	/*	Main function vars	*/
		SDL_Event event;
		int mousex, mousey, initmousex, initmousey;
		struct timeval t1, t2;
		struct timespec ts;
		float deltatime = 0.0, totaltime = 0.0f, timestep = 0.0;
		static volatile float fps = 0.0;
		unsigned int frames = 0, chosen = 0, dumplevel = 0;
		char osdfps[100] = "FPS = n/a", osdobj[100] = "Objects = n/a";
		char osdtime[100] = "Timestep = 0.0";
		bool flicked = 0, translate = 0;
		GLfloat view_rotx, view_roty, view_rotz, scalefactor = 0.09;
		GLfloat tr_x, tr_y, tr_z;
		GLuint* shaderprogs;
	/*	Main function vars	*/
	
	/*	Arguments	*/
		if(argc > 1) {
			for(int i=1; i < argc; i++) {
				if(!strcmp( "--novid", argv[i])) {
					option->novid = 1;
				}
				if(!strcmp( "-f", argv[i] ) ) {
					strcpy( option->filename, argv[i+1]);
				}
				if(!strcmp( "--nosync", argv[i])) {
					option->vsync = 0;
				}
				if(!strcmp( "--fullogl", argv[i])) {
					option->fullogl = 1;
				}
				if(!strcmp("--threads", argv[i])) {
					sscanf(argv[i+1], "%hu", &option->avail_cores);
					if(option->avail_cores == 0) {
						fprintf(stderr, "ERROR! Requires at least 1 thread.\n");
						return 1;
					}
				}
				if(!strcmp("--dumplevel", argv[i])) {
					sscanf(argv[i+1], "%u", &dumplevel);
				}
				if(!strcmp("--verb", argv[i])) {
					sscanf(argv[i+1], "%hu", &option->verbosity);
				}
				if( !strcmp("--help", argv[i])) {
					printf("%s\n", revision);
					printf("Usage:\n");
					printf("	-f (filename)		Specify a posdata file. Takes priority over configfile.\n");
					printf("	--novid 		Disable video output, do not initialize any graphical libraries.\n");
					printf("	--nosync 		Disables vsync.\n");
					printf("	--fullogl 		Initialize full OpenGL instead of ES.\n");
					printf("	--threads (int)		Make the program run with this many threads.\n");
					printf("	--verb (uint)		Set how much to output to stdout. Def. 5\n"); 
					printf("	--dumplevel (uint)		Set the dumplevel. 1=XYZ file every second. 2=every frame.\n"); 
					printf("	--help  		What you're reading.\n");
					return 0;
				}
			}
		}
		option->obj = preparser();
	/*	Arguments	*/
	
	/*	Error handling.	*/
		if(option->obj == 0) {
			fprintf(stderr, "Error: no objects!\n");
			return 1;
		} else sprintf(osdobj, "Objects = %i", option->obj);
		if(dumplevel) printf("Outputting XYZ file every second.\n");
		if(dumplevel == 2) fprintf(stderr, "Printing XYZ file every frame!\n");
	/*	Error handling.	*/
	
	/*	OpenGL ES 2.0 + SDL2	*/
		GLuint textvbo, linevbo, axisvbo, pointvbo, linkvbo;
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if(option->novid == 0) {
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			if(!option->fullogl) SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, option->oglmin);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, option->oglmax);
			window = SDL_CreateWindow(revision, \
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, option->width, option->height, \
				SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
			resize_wind();
			SDL_GL_CreateContext(window);
			SDL_GL_SetSwapInterval(option->vsync);
			glViewport(0, 0, option->width, option->height);
			create_shaders(&shaderprogs);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glActiveTexture(GL_TEXTURE0);
			glGenBuffers(1, &textvbo);
			glGenBuffers(1, &linevbo);
			glGenBuffers(1, &axisvbo);
			glGenBuffers(1, &pointvbo);
			glGenBuffers(1, &linkvbo);
			glClearColor(0.12, 0.12, 0.12, 1);
			pprintf(4, "OpenGL Version %s\n", glGetString(GL_VERSION));
		}
	/*	OpenGL ES 2.0 + SDL2	*/
	
	/*	Freetype.	*/
		if(option->novid == 0) {
			if(FT_Init_FreeType(&library)) {
				fprintf(stderr, "Could not init freetype library\n");
				return 1;
			}
			if(FT_New_Face(library, option->fontname, 0, &face)) {
				fprintf(stderr, "Could not open font\n");
				return 1;
			}
			FT_Set_Pixel_Sizes(face, 0, 34);
			g = face->glyph;
		}
	/*	Freetype.	*/
	
	/*	Physics.	*/
		data* object;
		pprintf(PRI_ESSENTIAL, "Objects: %i\n", option->obj);
		pprintf(PRI_ESSENTIAL, "Settings: dt=%f\n", option->dt);
		pprintf(PRI_ESSENTIAL, "Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n" \
		, option->elcharge, option->gconst, option->epsno);
		/*	Mallocs and wipes	*/
		initphys(&object);
		char threadtime[option->avail_cores][100];
		parser(&object, option->filename);
	/*	Physics.	*/
	
	gettimeofday (&t1 , NULL);
	
	threadcontrol(8, &object);
	
	while( 1 ) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_WINDOWEVENT:
					if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
						SDL_GetWindowSize(window, &option->width, &option->height);
						resize_wind();
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if(event.button.button == SDL_BUTTON_LEFT) flicked = 1;
					if(event.button.button == SDL_BUTTON_MIDDLE) {
						chosen = 0;
						translate = 1;
					}
					SDL_ShowCursor(0);
					SDL_GetMouseState(&initmousex, &initmousey);
					SDL_SetRelativeMouseMode(1);
					SDL_GetRelativeMouseState(NULL, NULL);
					break;
				case SDL_MOUSEBUTTONUP:
					if(event.button.button == SDL_BUTTON_LEFT) flicked = 0;
					if(event.button.button == SDL_BUTTON_MIDDLE) translate = 0;
					if(!translate && !flicked) {
						SDL_SetRelativeMouseMode(0);
						SDL_WarpMouseInWindow(window, initmousex, initmousey);
						SDL_ShowCursor(1);
					}
					break;
				case SDL_MOUSEWHEEL:
					if(event.wheel.y == 1) scalefactor *= 1.11;
					if(event.wheel.y == -1) scalefactor /= 1.11;
					if(scalefactor < 0.005) scalefactor = 0.005;
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_ESCAPE) {
						goto quit;
					}
					if(event.key.keysym.sym==SDLK_RIGHTBRACKET) {
						option->dt *= 2;
						printf("dt = %f\n", option->dt);
					}
					if(event.key.keysym.sym==SDLK_LEFTBRACKET) {
						option->dt /= 2;
						printf("dt = %f\n", option->dt);
					}
					if(event.key.keysym.sym==SDLK_SPACE) {
						if(threadcontrol(2, &object)) threadcontrol(0, &object);
						else threadcontrol(1, &object);
					}
					if(event.key.keysym.sym==SDLK_r) {
						view_roty = view_rotx = view_rotz = 0.0;
						tr_x = tr_y = tr_z = 0.0;
						scalefactor = 0.1;
						chosen = 0;
					}
					if(event.key.keysym.sym==SDLK_z) {
						toxyz(option->obj, object, timestep);
					}
					if(event.key.keysym.sym==SDLK_2) {
						if(chosen < option->obj) chosen++;
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
		timestep = thread_opts[1].processed*option->dt;
		gettimeofday(&t2, NULL);
		deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
		t1 = t2;
		totaltime += deltatime;
		frames++;
		if (totaltime >  1.0f) {
			if(dumplevel == 1) toxyz(option->obj, object, timestep);
			fps = frames/totaltime;
			sprintf(osdfps, "FPS = %3.2f", fps);
			totaltime = frames = 0;
			
			pprintf(PRI_VERYLOW, "Progressed %0.2f timeunits.\n", timestep);
			
			for(int i = 1; i < option->avail_cores + 1; i++) {
				clock_gettime(thread_opts[i].clockid, &ts);
				sprintf(threadtime[i], "Thread %i = %ld.%ld", i, ts.tv_sec, ts.tv_nsec / 1000000);
				pprintf(PRI_SPAM, "%s\n", threadtime[i]);
			}
		}
		
		if(option->novid) {
			SDL_Delay(100);
			continue;
		}
		
		if(flicked || translate) {
			SDL_GetRelativeMouseState(&mousex, &mousey);
			if(flicked) {
				view_roty += (float)mousex/4;
				view_rotx += (float)mousey/4;
			}
			if(translate) {
				tr_x += -(float)mousex/100;
				tr_y += (float)mousey/100;
			}
		}
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		glUseProgram(shaderprogs[0]);
		adjust_rot(view_rotx, view_roty, view_rotz, scalefactor, tr_x, tr_y, tr_z);
		
		/*	Link drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, linkvbo);
		drawlinks(object);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Link drawing	*/
		
		/*	Point/object drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, pointvbo);
		for(int i = 1; i < option->obj+1; i++) drawobject(object[i]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Point/object drawing	*/
		
		/*	Axis drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, axisvbo);
		drawaxis();
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Axis drawing	*/
		
		glUseProgram(shaderprogs[1]);
		
		/*	Selected object's red box	*/
		glBindBuffer(GL_ARRAY_BUFFER, linevbo);
		if(chosen != 0) {
			selected_box_text(object[chosen]);
			tr_x = -object[chosen].pos[0];
			tr_y = -object[chosen].pos[1];
			tr_z = -object[chosen].pos[2];
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Selected object's red box	*/
		
		/*	Text drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, textvbo);
		unsigned int fpscolor = GL_WHITE;
		if(fps < 25) fpscolor = GL_RED;
		if(fps >= 25 && fps < 48) fpscolor = GL_BLUE;
		if(fps >= 48) fpscolor = GL_GREEN;
		render_text(osdfps, -0.95, 0.85, 1.0/option->width, 1.0/option->height, fpscolor);
		
		render_text(osdobj, -0.95, 0.75, 1.0/option->width, 1.0/option->height, GL_WHITE);
		
		sprintf(osdtime, "Timestep = %0.2f", timestep);
		render_text(osdtime, -0.95, 0.65, 1.0/option->width, 1.0/option->height, GL_WHITE);
		
		if(!threadcontrol(2, &object)) render_text("Simulation stopped", -0.95, -0.95, 1.0/option->width, 1.0/option->height, GL_RED);
		
		for(int i = 1; i < option->avail_cores + 1; i++) {
			render_text(threadtime[i], 0.76, 0.95-((float)i/13), 0.75/option->width, 0.75/option->height, GL_WHITE);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Text drawing	*/
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		printf("Quitting... ");
		threadcontrol(9, &object);
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		SDL_DestroyWindow(window);
		SDL_Quit();
		free(shaderprogs);
		free(option);
		free(object);
		printf("success!\n");
		return 0;
}
