#include <windows.h>
#include <gl/GL.h>

#define WIDTH 1920
#define HEIGHT 1080

#define WNDCLASS_STATIC 0xC019 

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
		WS_POPUP | WS_VISIBLE, // style
		0, 0, WIDTH, HEIGHT,
		NULL, NULL, NULL, NULL
	);

	const HDC hdc = GetDC(hwnd);
	SetPixelFormat(hdc, ChoosePixelFormat(hdc, &kPfd), &kPfd);
	wglMakeCurrent(hdc, wglCreateContext(hdc));

	// Clear to solid red color to check that it works
	glClearColor(255, 0, 0, 0);

	for (;;) {
		if (GetAsyncKeyState(VK_ESCAPE))
			break;

		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(hdc);

		// Pump messages to tell Windows we're alive
		PeekMessageA(NULL, 0, 0, 0, PM_REMOVE);
	}

	ExitProcess(0);
}
