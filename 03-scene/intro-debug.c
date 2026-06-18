#include <windows.h>
#include <gl/GL.h>

#define WIDTH 1920
#define HEIGHT 1080

#define WNDCLASS_STATIC 0xC019 

// glext.h definitions
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81

typedef char GLchar;
typedef void (APIENTRY *PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROGRAMVPROC) (GLenum type, GLsizei count, const GLchar *const*strings);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);

#if COMPRESSED
#define LIST_SHADER_GL_FUNCS(X) \
	X(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv)
#else
#define LIST_SHADER_GL_FUNCS(X) \
	X(PFNGLATTACHSHADERPROC, glAttachShader) \
	X(PFNGLCOMPILESHADERPROC, glCompileShader) \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
	X(PFNGLCREATESHADERPROC, glCreateShader) \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
	X(PFNGLSHADERSOURCEPROC, glShaderSource)
#endif

#define LIST_GL_FUNCS(X) \
	LIST_SHADER_GL_FUNCS(X) \
	X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
	X(PFNGLUNIFORM1FPROC, glUniform1f) \
	X(PFNGLUSEPROGRAMPROC, glUseProgram) \

#define X(t, n) static t n;
LIST_GL_FUNCS(X)
#undef X

static void loadGLFunctions(void) {
#if COMPRESSED
#define X(t, n) n = (t)(void*)wglGetProcAddress(#n);
#else
#define X(t, n) \
	n = (t)(void*)wglGetProcAddress(#n); \
	if (!n) MessageBoxA(NULL, #n, "wglGetProcAddress", MB_ICONERROR);
#endif
	LIST_GL_FUNCS(X)
#undef X
}

static const PIXELFORMATDESCRIPTOR kPfd = {
	.nSize = sizeof(kPfd),
	.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
	.iPixelType = PFD_TYPE_RGBA,
	.cColorBits = 24,
};

static const DEVMODEA devmode = {
	.dmSize = sizeof(devmode),
	.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL,
	.dmBitsPerPel = 32,
	.dmPelsWidth = WIDTH,
	.dmPelsHeight = HEIGHT,
};

static const char *shader_source =
"uniform float t;\n"
"void main() { gl_FragColor = vec4(0., 1., .5 + .5 * sin(t), 0.); }";

#ifdef _DEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nShowCmd) {
	(void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nShowCmd;
#else
int WinMainCRTStartup(void) {
#endif

	ShowCursor(FALSE);
	ChangeDisplaySettingsA((DEVMODEA*)&devmode, CDS_FULLSCREEN);

	const HWND hwnd = CreateWindowA(
		(void*)WNDCLASS_STATIC,
		"intro", // title
		WS_POPUP | WS_VISIBLE,
		0, 0, WIDTH, HEIGHT,
		NULL, NULL, NULL, NULL
	);

	const HDC hdc = GetDC(hwnd);
	SetPixelFormat(hdc, ChoosePixelFormat(hdc, &kPfd), &kPfd);
	const HGLRC hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hglrc);

	loadGLFunctions();

	glViewport(0, 0, WIDTH, HEIGHT);

#ifndef _DEBUG
	const GLint program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &shader_source);
#else
	const GLint program = glCreateProgram();
	{
		const GLint shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(shader, 1, &shader_source, NULL);
		glCompileShader(shader);
		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char buf[1024];
			glGetShaderInfoLog(shader, sizeof(buf)-1, NULL, buf);
			MessageBoxA(NULL, buf, "Shader error", MB_ICONERROR);
			ExitProcess(0);
		}
		glAttachShader(program, shader);
		glLinkProgram(program);
	}
#endif
	glUseProgram(program);
	const GLint t_loc = glGetUniformLocation(program, "t");

	for (;;) {
		if (GetAsyncKeyState(VK_ESCAPE))
			break;

		glUniform1f(t_loc, GetTickCount() / 1000.0f);
		glRects(-1, -1, 1, 1);
		SwapBuffers(hdc);

		// Pump messages to tell Windows we're alive
		PeekMessageA(NULL, 0, 0, 0, PM_REMOVE);
	}

	ExitProcess(0);
}
