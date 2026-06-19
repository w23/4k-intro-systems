#include <windows.h>
#include <gl/GL.h>

#include <mmreg.h>
#include "music.h"

#define WIDTH 1920
#define HEIGHT 1080

#define WNDCLASS_STATIC 0xC019 

// glext.h definitions
#define GL_FRAGMENT_SHADER                0x8B30

typedef char GLchar;
#define APIENTRYP APIENTRY*
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROGRAMVPROC) (GLenum type, GLsizei count, const GLchar *const*strings);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);

static PFNGLCREATESHADERPROGRAMVPROC glCreateShaderProgramv;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUNIFORM1FPROC glUniform1f;

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

static const WAVEFORMATEX kWaveFormat = {
#ifdef SU_SAMPLE_FLOAT
	.wFormatTag = WAVE_FORMAT_IEEE_FLOAT,
#else
	.wFormatTag = WAVE_FORMAT_PCM,
#endif
	.nChannels = SU_CHANNEL_COUNT,
	.nSamplesPerSec = SU_SAMPLE_RATE,
	.nAvgBytesPerSec = SU_SAMPLE_SIZE * SU_SAMPLE_RATE * SU_CHANNEL_COUNT,
	.nBlockAlign = SU_SAMPLE_SIZE * SU_CHANNEL_COUNT,
	.wBitsPerSample = SU_SAMPLE_SIZE * 8,
	.cbSize = 0, // no extras
};

static SUsample sound_buffer[SU_LENGTH_IN_SAMPLES * SU_CHANNEL_COUNT];

static WAVEHDR kWaveHeader = {
	.lpData = (LPSTR)sound_buffer,
	.dwBufferLength = sizeof(sound_buffer),
	.dwFlags = WHDR_PREPARED,
};

static const char *shader_source =
"uniform float t;\n"
"void main() { gl_FragColor = vec4(0., 1., .5 + .5 * sin(t), 0.); }";

#ifdef _DEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nShowCmd) {
	(void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nShowCmd;
#else
int WinMainCRTStartup(void) {
	ShowCursor(FALSE);
	ChangeDisplaySettingsA((DEVMODEA*)&devmode, CDS_FULLSCREEN);
#endif
#ifdef SU_LOAD_GMDLS
	su_load_gmdls();
#endif

	// Give synth thread a head start
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)su_render_song, sound_buffer, 0, NULL);

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
	glGetUniformLocation = (void*)wglGetProcAddress("glGetUniformLocation");
	glUniform1f = (void*)wglGetProcAddress("glUniform1f");

	const GLint program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &shader_source);

	glUseProgram(program);

	const GLint t_loc = glGetUniformLocation(program, "t");

	HWAVEOUT waveout;
	waveOutOpen(&waveout, WAVE_MAPPER, &kWaveFormat, 0, 0, 0);
	waveOutWrite(waveout, &kWaveHeader, sizeof(kWaveHeader));

	for (;;) {
		if (GetAsyncKeyState(VK_ESCAPE))
			break;

		MMTIME mmtime;
		mmtime.wType = TIME_SAMPLES;
		waveOutGetPosition(waveout, &mmtime, sizeof(mmtime));
		if (mmtime.u.sample >= SU_LENGTH_IN_SAMPLES)
			break;

		glUniform1f(t_loc, mmtime.u.sample / (float)SU_SAMPLES_PER_ROW);
		glRects(-1, -1, 1, 1);
		SwapBuffers(hdc);

		// Pump messages to tell Windows we're alive
		PeekMessageA(NULL, 0, 0, 0, PM_REMOVE);
	}

	ExitProcess(0);
}
