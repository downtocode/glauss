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
bool novid = 0, vsync = 1, quiet = 0, stop = 0, enforced = 0, nowipe = 0, random = 1;
unsigned int chosen = 0;
unsigned short int avail_cores = 0;


GLint u_matrix = -1;
GLint attr_pos = 0, attr_color = 1, attr_texcoord = 2, attr_tex = 3;
GLfloat view_rotx = 0.0, view_roty = 0.0;
GLfloat rotatex = 0.0, rotatey = 0.0;
GLfloat chosenbox[4][2];

GLfloat colors[3] = {1.0f, 1.0f, 1.0f};


int main(int argc, char *argv[])
{
	/*	Main function vars	*/
		int linkcount;
		struct timeval t1, t2;
		float deltatime, totaltime = 0.0f, fps;
		unsigned int frames = 0;
		char osdtext[500] = "";
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
		GLuint tex, textvbo, linevbo;
		
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if(novid == 0) {
			window = SDL_CreateWindow("Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL);
			SDL_GL_CreateContext(window);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetSwapInterval(vsync);
			glViewport(0, 0, width, height);
			create_shaders();
			glActiveTexture(GL_TEXTURE0);
			glGenBuffers(1, &textvbo);
			glGenBuffers(1, &linevbo);
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glUniform1i(attr_tex, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glClearColor(0.1, 0.1, 0.1, 1);
		}
		SDL_Event event;
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
		FT_Set_Pixel_Sizes(face, 0, 50);
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
						rotatex -= 5.0;
					}
					if(event.key.keysym.sym==SDLK_e) {
						rotatex += 5.0;
					}
					if(event.key.keysym.sym==SDLK_w) {
						rotatey -= 5.0;
					}
					if(event.key.keysym.sym==SDLK_s) {
						rotatey += 5.0;
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
			if (totaltime >  2.0f) {
				fps = frames/totaltime;
				if(novid == 0) {
					sprintf(osdtext, "FPS = %3.2f", fps);
				} else {
					printf("Current FPS = %3.2f\n", fps);
				}
				totaltime -= 2.0f;
				frames = 0;
			}
		}
		if(novid == 1) continue;
		if(nowipe == 0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		/*	Rotation control	*/
		view_rotx = rotatex;
		view_roty = rotatey;
		
		glUniform4fv(attr_color, 1, colors);
		
		adjust_rot();
		
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
		
		glVertexAttribPointer(attr_pos, 3, GL_FLOAT, GL_FALSE, 0, link);
		glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
		glEnableVertexAttribArray(attr_pos);
		glEnableVertexAttribArray(attr_color);
		glDrawArrays(GL_LINES, 0, linkcount);
		glDisableVertexAttribArray(attr_pos);
		glDisableVertexAttribArray(attr_color);
		/*	Link drawing	*/
		
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
			glEnableVertexAttribArray(attr_pos);
			glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glBufferData(GL_ARRAY_BUFFER, sizeof chosenbox, chosenbox, GL_STATIC_DRAW);
			glDrawArrays(GL_LINE_LOOP, 0, 4);
			glDisableVertexAttribArray(attr_pos);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		/*	Selected object's red box	*/
		
		/*	Point/object drawing	*/
		for(int i = 1; i < obj + 1; i++) drawcircle(object[i].pos[0], object[i].pos[1], object[i].radius);
		/*	Point/object drawing	*/
		
		/*	Text drawing	*/
		view_rotx = view_roty = 0;
		adjust_rot();
		glBindBuffer(GL_ARRAY_BUFFER, textvbo);
		render_text(osdtext, -1.9, 1.8, 2.0/width, 2.0/height);
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
