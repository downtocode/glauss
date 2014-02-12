//Things you should all have anyway.
#include <stdio.h>
#include <tgmath.h>
#include <time.h>

//Things you gotta get.
#include <ft2build.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include FT_FREETYPE_H

//Functions.
#include "physics.h"
#include "parser.h"
//#include "x11disp.h"


FT_Library  library;
FT_Face face;


static int i, j, linkcount;

//Default settings
int obj = 0, width = 1200, height = 600;
int boxsize = 15;
char fontname[200] = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
char filename[200] = "posdata.dat";
float dt = 0.008, radius = 12.0;
long double elcharge = 0, gconst = 0, epsno = 0;
bool novid = 0, vsync = 1, quiet = 0, stop = 0;



time_t start, end;
double fps;
unsigned int counter = 0;
unsigned int sec;


//CLEANUPFROMONWARDS
GLfloat colors[3][3] = {
      { 1, 0, 0 },
      { 0, 1, 0 },
      { 0, 0, 1 }
   };

static GLint u_matrix = -1;
static GLint attr_pos = 0, attr_color = 1;
static GLfloat view_rotx = 0.0, view_roty = 0.0;


static void make_z_rot_matrix(GLfloat angle, GLfloat *m)
{
   float c = cos(angle * acos(-1) / 180.0);
   float s = sin(angle * acos(-1) / 180.0);
   int i;
   for (i = 0; i < 16; i++)
      m[i] = 0.0;
   m[0] = m[5] = m[10] = m[15] = 1.0;

   m[0] = c;
   m[1] = s;
   m[4] = -s;
   m[5] = c;
}

static void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m)
{
   int i;
   for (i = 0; i < 16; i++)
      m[i] = 0.0;
   m[0] = xs;
   m[5] = ys;
   m[10] = zs;
   m[15] = 1.0;
}


static void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b)
{
#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  p[(col<<2)+row]
	GLfloat p[16];
	GLint i;
	for (i = 0; i < 4; i++) {
		const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
		P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
		P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
		P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
		P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
	}
	memcpy(prod, p, sizeof(p));
#undef A
#undef B
#undef PROD
}


void draw(void)
{
	GLfloat mat[16], rot[16], scale[16];

	/* Set modelview/projection matrix */
	make_z_rot_matrix(view_rotx, rot);
	make_scale_matrix(0.5, 0.5, 0.5, scale);
	mul_matrix(mat, rot, scale);
	glUniformMatrix4fv(u_matrix, 1, GL_FALSE, mat);
}


void create_shaders(void)
{
   static const char *fragShaderText =
      "void main() {\n"
      "   gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_Position = gl_Vertex;\n"
      "}\n";

   GLuint fragShader, vertShader, program;
   GLint stat;

   fragShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragShader, 1, (const char **) &fragShaderText, NULL);
   glCompileShader(fragShader);
   glGetShaderiv(fragShader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      printf("Error: fragment shader did not compile!\n");
      exit(1);
   }

   vertShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertShader, 1, (const char **) &vertShaderText, NULL);
   glCompileShader(vertShader);
   glGetShaderiv(vertShader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      printf("Error: vertex shader did not compile!\n");
      exit(1);
   }

   program = glCreateProgram();
   glAttachShader(program, fragShader);
   glAttachShader(program, vertShader);
   glLinkProgram(program);

   glGetProgramiv(program, GL_LINK_STATUS, &stat);
   if (!stat) {
      char log[1000];
      GLsizei len;
      glGetProgramInfoLog(program, 1000, &len, log);
      printf("Error: linking:\n%s\n", log);
      exit(1);
   }

   glUseProgram(program);

   if (1) {
      /* test setting attrib locations */
      glBindAttribLocation(program, attr_pos, "pos");
      glBindAttribLocation(program, attr_color, "color");
      glLinkProgram(program);  /* needed to put attribs into effect */
   }
   else {
      /* test automatic attrib locations */
      attr_pos = glGetAttribLocation(program, "pos");
      attr_color = glGetAttribLocation(program, "color");
   }

   u_matrix = glGetUniformLocation(program, "modelviewProjection");
   printf("Uniform modelviewProjection at %d\n", u_matrix);
   printf("Attrib pos at %d\n", attr_pos);
   printf("Attrib color at %d\n", attr_color);
}


int main( int argc, char *argv[] )
{
	/*	ARGUMENT SETTING	*/
	if( argc > 1 ) {
		for( i=1; i < argc; i++ ) {
			if( !strcmp( "--novid", argv[i] ) ) {
				novid = 1;
			}
			if( !strcmp( "--quiet", argv[i] ) ) {
				quiet = 1;
			}
			if( !strcmp( "-f", argv[i] ) ) {
				strcpy( filename, argv[i+1] );
			}
			if( !strcmp( "--nosync", argv[i] ) ) {
				vsync = 0;
			}
			if( !strcmp( "--help", argv[i] ) ) {
				printf("Usage:\n");
				printf("	-f (filename)		Specify a posdata file. Takes priority over configfile.\n");
				printf("	--novid 		Disable video output, do not initialize any graphical libraries.\n");
				printf("	--nosync		Disable vsync, render everything as fast as possible.\n"); 
				printf("	--quiet 		Disable any terminal output except errors.\n"); 
				printf("	--help  		What you're reading.\n");
				return 0;
			}
		}
	}
	obj = preparser(&dt, &elcharge, &gconst, &epsno, &width, &height, &boxsize, fontname, filename);
	/*	ARGUMENT SETTING	*/
	
	/*	Error handling.	*/
		if( obj == 0 ) {
			printf("ERROR! NO OBJECTS!\n");
			return 1;
		}
	/*	Error handling.	*/
	
	/*	OGL && EGL	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if( novid == 0 ) {
			window = SDL_CreateWindow( "Physengine", 0, 0, width, height, SDL_WINDOW_OPENGL );
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetSwapInterval(-1);
			SDL_GL_CreateContext(window);
		}
		SDL_Event event;
		if( quiet == 0 ) {
			printf("OpenGL Version %s\n", glGetString(GL_VERSION));
		}
		glViewport(0, 0, (GLint) width, (GLint) height);
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
		if( quiet == 0 ) {
			printf("Settings: dt=%f, widith=%i, height=%i, boxsize=%i, fontname=%s\n", dt, width, height, boxsize, fontname);
			printf("Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n", elcharge, gconst, epsno);
		}
		
		//Malloc the objects
		initphys(&object);
		
		parser(&object, filename);
	/*	PHYSICS.	*/
	
	time(&start);
	
	
	while( 1 ) {
		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
				case SDL_QUIT:
					goto quit;
					break;
			}
		}
			
		if( stop == 0 ) integrate(object);
		if( quiet == 0 ) {
			//FPS calculation.
			time(&end);
			++counter;
			sec = difftime (end, start);
			fps = (((float)counter)/((float)sec));
			printf("FPS = %.2f\r", fps);
		}
		if( novid == 1 ) continue;
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		draw();
		
		/*	Link drawing	*/
		GLfloat link[linkcount][2];
		
		link[1][0] = object[1].pos[0];
		link[1][1] = object[1].pos[1];
		link[2][0] = object[2].pos[0];
		link[2][1] = object[2].pos[1];
		
		glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, link);
		glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
		glEnableVertexAttribArray(attr_pos);
		glEnableVertexAttribArray(attr_color);
		glDrawArrays(GL_LINES, 1, 2);
		glDisableVertexAttribArray(attr_pos);
		glDisableVertexAttribArray(attr_color);
		linkcount = 0;
		/*	Link drawing	*/
		
		/*
		//Drawing choseon object's red box.
		glColor3f(255,0,0);
		glBegin(GL_LINE_LOOP);
			for(i = 1; i < obj + 1; i++) {
				if(chosen==i) {
					glVertex3f( object[i].pos[0] - boxsize, object[i].pos[1] - boxsize, 1 );
					glVertex3f( object[i].pos[0] - boxsize, object[i].pos[1] + boxsize, 1 );
					glVertex3f( object[i].pos[0] + boxsize, object[i].pos[1] + boxsize, 1 );
					glVertex3f( object[i].pos[0] + boxsize, object[i].pos[1] - boxsize, 1 );
				}
			}
			glColor3f(255,255,255);
		glEnd();*/
		
		/*	Point/object drawing	*/
		GLfloat points[obj+1][3];
		for(i = 1; i < obj + 1; i++) {
			points[i][0] = object[i].pos[0];
			points[i][1] = object[i].pos[1];
			points[i][2] = object[i].pos[2];
		}
		
		glVertexAttribPointer(attr_pos, 3, GL_FLOAT, GL_FALSE, 0, points);
		glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
		glEnableVertexAttribArray(attr_pos);
		glEnableVertexAttribArray(attr_color);
		glDrawArrays(GL_POINTS, 1, obj+1);
		glDisableVertexAttribArray(attr_pos);
		glDisableVertexAttribArray(attr_color);
		/*	Point/object drawing	*/

		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		free(object);
		SDL_Quit();
		FT_Done_Face( face );
		FT_Done_FreeType( library );
		printf("\nQuitting!\n");
		return 0;
}
