#include <stdio.h>
#include <assert.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

void x11disp(Display *x_dpy, EGLDisplay egl_dpy, const char *name, int x, int y, int width, int height, Window *winRet, EGLContext *ctxRet, EGLSurface *surfRet) {
	static const EGLint attribs[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_DEPTH_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	
	static const EGLint ctx_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	
	int scrnum = DefaultScreen( x_dpy );;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root = RootWindow( x_dpy, scrnum );
	Window win;
	XVisualInfo *visInfo, visTemplate;
	int num_visuals;
	EGLContext ctx;
	EGLConfig config;
	EGLint num_configs;
	EGLint vid;
	
	
	eglChooseConfig( egl_dpy, attribs, &config, 1, &num_configs);
	
	assert(config);
	assert(num_configs > 0);
	
	eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid);
	
	visTemplate.visualid = vid;
	visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
	
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( x_dpy, root, visInfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	
	win = XCreateWindow( x_dpy, root, 0, 0, width, height,
	0, visInfo->depth, InputOutput,
	visInfo->visual, mask, &attr );
	
	XSizeHints sizehints;
	sizehints.x = x;
	sizehints.y = y;
	sizehints.width  = width;
	sizehints.height = height;
	sizehints.flags = USSize | USPosition;
	XSetNormalHints(x_dpy, win, &sizehints);
	XSetStandardProperties(x_dpy, win, name, name,
	None, (char **)NULL, 0, &sizehints);
	
	eglBindAPI(EGL_OPENGL_ES_API);
	
	ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs );
	
	EGLint val;
	eglQueryContext(egl_dpy, ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
	assert(val == 2);
	
	*surfRet = eglCreateWindowSurface(egl_dpy, config, win, NULL);
	
	//EGLint val;
	eglQuerySurface(egl_dpy, *surfRet, EGL_WIDTH, &val);
	assert(val == width);
	eglQuerySurface(egl_dpy, *surfRet, EGL_HEIGHT, &val);
	assert(val == height);
	assert(eglGetConfigAttrib(egl_dpy, config, EGL_SURFACE_TYPE, &val));
	assert(val & EGL_WINDOW_BIT);
	
	XFree(visInfo);
	
	*winRet = win;
	*ctxRet = ctx;
}
