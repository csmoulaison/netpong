#include <GL/glx.h>

typedef GLXContext(*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

typedef struct
{
	XVisualInfo* visual_info;
	GLXFBConfig framebuffer_config;
} GlxInitInfo;

GlxInitInfo glx_init_pre_window(Xlib* xlib)
{
	int32_t glx_version_major;
	int32_t glx_version_minor;
	if(glXQueryVersion(xlib->display, &glx_version_major, &glx_version_minor) == 0
	|| ((glx_version_major == 1) && (glx_version_minor < 3)) || (glx_version_major < 1)) {
		panic();
	}

	int32_t desired_framebuffer_attributes[] = {
		GLX_X_RENDERABLE, True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_DOUBLEBUFFER, True,
		GLX_SAMPLE_BUFFERS, 1,
		GLX_SAMPLES, 4,
		None
	};

	int32_t framebuffer_configs_len;
	GLXFBConfig* framebuffer_configs = glXChooseFBConfig(xlib->display, DefaultScreen(xlib->display), desired_framebuffer_attributes, &framebuffer_configs_len);
	if(framebuffer_configs == NULL) {
		panic();
	}

	// Here we choose the framebuffer config with the most samples per pixel.
	int32_t best_framebuffer_config = -1;
	int32_t most_samples = -1;
	for(int32_t i = 0; i < framebuffer_configs_len; i++) {
		XVisualInfo* tmp_visual_info = glXGetVisualFromFBConfig(xlib->display, framebuffer_configs[i]);
		if(tmp_visual_info != NULL) {
			int32_t sample_buffers;
			int32_t samples;
			glXGetFBConfigAttrib(xlib->display, framebuffer_configs[i], GLX_SAMPLE_BUFFERS, &sample_buffers);
			glXGetFBConfigAttrib(xlib->display, framebuffer_configs[i], GLX_SAMPLES, &samples);
			if(best_framebuffer_config == -1 || (sample_buffers && samples > most_samples)) {
				best_framebuffer_config = i;
				most_samples = samples;
			}
		}
		XFree(tmp_visual_info);
	}

	GLXFBConfig framebuffer_config = framebuffer_configs[best_framebuffer_config];
	XFree(framebuffer_configs);

	// The visual info returned from the chosen framebuffer config will be used for Xlib window creation.
	XVisualInfo* visual_info = glXGetVisualFromFBConfig(xlib->display, framebuffer_config);

	GlxInitInfo init_info;
	init_info.visual_info = visual_info;
	init_info.framebuffer_config = framebuffer_config;
	return init_info;
}

void glx_init_post_window(Xlib* xlib, GLXFBConfig framebuffer_config)
{
	// Check for required GL extensions.
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB;
	char* gl_extensions = (char*)glXQueryExtensionsString(xlib->display, DefaultScreen(xlib->display));
	glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

	const char* extension = "GLX_ARB_create_context";
	char* start;
	char* where;
	char* terminator;

	// Extension names should not have spaces.
	where = strchr((char*)extension, ' ');
	if (where || *extension == '\0') {
		panic();
	}

	// It takes a bit of care to be fool-proof about parsing the OpenGL
	// extensions string. Don't be fooled by sub-strings, etc.
	bool found_extension = true;
	for (start = gl_extensions;;) {
		where = strstr(start, extension);
		if (!where) {
			break;
		}

		terminator = where + strlen(extension);

		if (where == start || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				found_extension = true;
			}

			start = terminator;
		}
	}
	if(found_extension == false) {
		panic();
	}

	// Create GLX context and window
	int32_t glx_attributes[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 6,
		None
	};

	GLXContext glx = glXCreateContextAttribsARB(xlib->display, framebuffer_config, 0, 1, glx_attributes);
	if(glXIsDirect(xlib->display, glx) == false) {
		panic();
	}

	// Bind GLX to window
	glXMakeCurrent(xlib->display, xlib->window, glx);
}
