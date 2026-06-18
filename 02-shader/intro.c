#include <windows.h>
#include <gl/GL.h>

#define WIDTH 1920
#define HEIGHT 1080

#define WNDCLASS_STATIC 0xC019 

// glext.h definitions
#define GL_FRAGMENT_SHADER                0x8B30

typedef char GLchar;
#define APIENTRYP APIENTRY*
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROGRAMVPROC) (GLenum type, GLsizei count, const GLchar *const*strings);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);

static PFNGLCREATESHADERPROGRAMVPROC glCreateShaderProgramv;
static PFNGLUSEPROGRAMPROC glUseProgram;

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
"void main() { gl_FragColor = vec4(0., 1., 0., 0.); }";

int WinMainCRTStartup(void) {
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

	glCreateShaderProgramv = (void*)wglGetProcAddress("glCreateShaderProgramv");
	glUseProgram = (void*)wglGetProcAddress("glUseProgram");

	const GLint program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &shader_source);
	glUseProgram(program);

	for (;;) {
		if (GetAsyncKeyState(VK_ESCAPE))
			break;

		glRects(-1, -1, 1, 1);
		SwapBuffers(hdc);

		// Pump messages to tell Windows we're alive
		PeekMessageA(NULL, 0, 0, 0, PM_REMOVE);
	}

	ExitProcess(0);
}
