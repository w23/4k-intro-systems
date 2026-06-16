#include <windows.h>

#ifdef COMPRESSED
int WinMainCRTStartup(void) {
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nShowCmd) {
	(void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nShowCmd;
#endif

	return 0;
}
