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
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);

static PFNGLCREATESHADERPROGRAMVPROC glCreateShaderProgramv;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUNIFORM1FPROC glUniform1f;

#ifdef _DEBUG
#define GL_LINK_STATUS                    0x8B82

typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

#define LIST_GL_FUNCS(X) \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) 

#define X(t, n) static t n;
LIST_GL_FUNCS(X)
#undef X

#endif

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
"#version 130\n"
"uniform float t;\n"
"const vec2 R = vec2(1920.,1080.);\n"
"\n"
"const float S = 4.;\n"
"float hash1(float x){return fract(sin(x)*265871.1723);}\n"
"float hash2(vec2 x){return hash1(dot(x,vec2(11.,313.)));}\n"
"float hash3(vec3 x){return hash1(dot(x,vec3(3.2,57.55,117.234)));}\n"
"//vec3 hash33(vec3 x){return vec3(hash3(x),hash2(x.yz),hash1(x.x));}\n"
"vec3 hash3v(float x){return vec3(hash1(x),hash1(x+.1),hash1(x+.2));}\n"
"vec2 hash2v(float x){return vec2(hash1(x),hash1(x+.1));}\n"
"\n"
"float noise1(float v) {\n"
"	float V=floor(v);v-=V;\n"
"	return mix(hash1(V),hash1(V+1),v);\n"
"}\n"
"\n"
"float noise2(vec2 v) {\n"
"	vec2 /*E=vec2(0.,1.), */V=floor(v); v-=V;\n"
"	return mix(\n"
"		mix(hash2(V),     hash2(V+vec2(1.,0.)),v.x),\n"
"		mix(hash2(V+vec2(0.,1.)),hash2(V+vec2(1.)),v.x),v.y);\n"
"}\n"
"\n"
"float fbm(vec2 v) {\n"
"	return noise2(v)*.5\n"
"		+ noise2(v*2.1)*.25\n"
"		+ noise2(v*3.9)*.125\n"
"		//+ noise2(v*8.1)*.125\n"
"		;\n"
"}\n"
"\n"
"/*\n"
"float noise3(vec3 v) {\n"
"	vec3 V = floor(v);v-=V;\n"
"	return mix(mix(\n"
"		mix(hash3(V),hash3(V+E.xyy),v.x),\n"
"			mix(hash3(V+E.yxy),hash3(V+E.xxy),v.x),v.y),\n"
"			mix(mix(hash3(V+E.yyx),hash3(V+E.xyx),v.x),\n"
"			mix(hash3(V+E.yxx),hash3(V+E.xxx),v.x),v.y),v.z);\n"
"}\n"
"*/\n"
"\n"
"float quantize(float v, float n) { return floor(v*n)/n; }\n"
"vec2 quantize2(vec2 v, vec2 n) { return floor(v*n)/n; }\n"
"\n"
"//mat2 Rm(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}\n"
"\n"
"float rep(float p, float s) { return mod(p, s) - s*.5; }\n"
"vec2 rep2(vec2 p, vec2 s) { return mod(p, s) - s*.5; }\n"
"//vec3 rep3(vec3 p, vec3 s) { return mod(p, s) - s*.5; }\n"
"//float box2(vec2 p, vec2 s) { p = abs(p) - s; return max(p.x,p.y); }\n"
"//float box3(vec3 p, vec3 s) { p = abs(p) - s; return max(max(p.x,p.y),p.z); }\n"
"float vmax3(vec3 v){return max(max(v.x,v.y),v.z);}\n"
"float vmax2(vec2 v){return max(v.x,v.y);}\n"
"float box3(vec3 p, vec3 s) { return vmax3(abs(p)-s); }\n"
"float box2(vec2 p, vec2 s) { return vmax2(abs(p)-s); }\n"
"\n"
"vec3 vminc(vec3 v){return step(v.xyz,v.yzx)*step(v.xyz,v.zxy);}\n"
"vec3 vmaxc(vec3 v){return step(v.yzx,v.xyz)*step(v.zxy,v.xyz);}\n"
"\n"
"#define RM(a) mat2(cos(a),sin(a),-sin(a),cos(a))\n"
"\n"
"vec2 w_off = vec2(0.);\n"
"float w_kust = 0.;\n"
"float w_h = 0.;\n"
"float w_xlam = 0.;\n"
"float wmat = 0.;\n"
"// 0 == wall\n"
"// 1 == ground\n"
"// 2 == kust\n"
"float w(vec3 p) {\n"
"	float d = 1e6;\n"
"	vec3 po = p;\n"
"\n"
"	//float table = box3(p+vec3(0.,3.,0.), vec3(2., 2., 2.));\n"
"	//d = min(d, table);\n"
"\n"
"	wmat = 0.;\n"
"	float hm = fbm((p.xz - w_off)*.1);\n"
"	float dm = po.y + 8. - (hm - .3) * 18. * w_h;\n"
"	if (d > dm) { d = dm; wmat = 1.; }\n"
"\n"
"	float bounding = -box3(p, vec3(20.,8.,30.));\n"
"	if (d > bounding) { d = bounding; wmat = 0.; }\n"
"\n"
"	if (w_kust != 0.) {\n"
"		vec3 kp = vec3(rep2(p.xz-w_off,vec2(12., 25.)), p.y - hm * 8. + 6.).xzy;\n"
"		float kust = length(kp)-3.;\n"
"		if (d > kust) { d = kust; wmat = 2.; }\n"
"	}\n"
"\n"
"	if (w_xlam > 0.)\n"
"	{\n"
"		d = min(d, hash3(.01*vec3(p.xz-floor(w_off), p.y).xzy)*20.1-.01);\n"
"	}\n"
"	\n"
"	return d;\n"
"}\n"
"\n"
"vec3 CI,N;\n"
"float tracegrid(vec3 O, vec3 D, float steps){\n"
"	vec3 Di = 1. / D, Ds = sign(D);\n"
"	vec3 ci = CI = floor(O);\n"
"	vec3 sd = (ci - O + .5 + Ds * .5) * Di;\n"
"	vec3 n = vec3(0.);\n"
"	for (float i = 0.; i < steps; i++) {\n"
"		float ww = w(ci + .5);\n"
"		if (ww < 0.) {\n"
"			N = - n * Ds;\n"
"			sd = (ci - O + .5 - Ds * .5) * Di;\n"
"			return max(max(sd.x,sd.y),sd.z);\n"
"		}\n"
"		CI = ci;\n"
"		n = vminc(sd);\n"
"		sd += n * Ds * Di;\n"
"		ci += n * Ds;\n"
"		//if (dot(ci,ci) > 2048.) break;\n"
"	}\n"
"\n"
"	return 1e6;\n"
"}\n"
"\n"
"mat3 lookat(vec3 pos, vec3 at, vec3 up) {\n"
"	vec3 z = normalize(pos - at);\n"
"	vec3 x = normalize(cross(up, z));\n"
"	up = normalize(cross(z, x));\n"
"	return mat3(x, up, z);\n"
"}\n"
"\n"
"vec3 skyc(vec3 d, vec3 sund) {\n"
"	float clouds_l = 8. / d.y; vec2 cloud_pos = d.xz * clouds_l;\n"
"	vec3 sky = 1. * vec3(.1,.3,.7);\n"
"	float clouds = step(fbm(-w_off*.041+cloud_pos*.1), .3);\n"
"	float dotsun = pow(max(dot(d,sund)*1.1, 0.), 20.);\n"
"	sky = mix(sky, 2. * vec3(.9,.6,.4), dotsun);\n"
"	sky = mix(sky, vec3(1.), clouds);\n"
"	return sky;\n"
"\n"
"	// sky = mix(sky, vec3(1.), step(fbm(-w_off*.041+d*.1), .3));\n"
"	// vec3 base_color = mix(vec3(.2), sky, max(0.,pow(d.y, .7)));\n"
"	// sky = mix(sky, 10. * vec3(.9,.6,.4), step(length(ci.xz-vec2(10.,-10.)), 3.));\n"
"	//return mix(vec3(.005), base_color * 6., max(0.,sund.y));\n"
"}\n"
"\n"
"void main() {\n"
"	vec2 uv = gl_FragCoord.xy / R.xy - .5; uv.x *= R.x/R.y;\n"
"	//float seed = fract(t + dot(vec2(.3,5.),gl_FragCoord.xy));\n"
"	float seed;// = fract(t + hash1(uv.x) + hash1(uv.y));\n"
"\n"
"	float do_sky = 0.;\n"
"	float do_lamps = 2.;\n"
"	float do_lake = 0.;\n"
"	float do_color = 0.;\n"
"	float do_borders = 0.;\n"
"	vec2 do_move = vec2(.2);\n"
"	//float do_wall_rough = .3;\n"
"\n"
"	vec3 cam_pos = vec3(0.,0.,28.), cam_at = vec3(0.,0.,-30.);\n"
"	float cam_roll = 0.;\n"
"	float pat = ((t<1376.)?t:t+32.) / 128.;\n"
"	float phase = fract(pat);\n"
"	float npat = floor(pat);\n"
"\n"
"	float do_color_env = smoothstep(0., .125, phase) * smoothstep(1., .75, phase);\n"
"\n"
"	if (npat < 1.) {\n"
"		//vec2 pos = npat * (hash2v(npat) - .5) * vec2(5., 4.);\n"
"		cam_pos = vec3(0., 4., -25. + phase * 30.);\n"
"		cam_at = vec3(0., 0., -30.);\n"
"	} else if (npat < 2.) {\n"
"		cam_pos = vec3(10., 4., -25. + phase * 30.);\n"
"		cam_at = vec3(-10., 0., cam_pos.z);\n"
"	} else if (npat < 3.) {\n"
"		cam_pos = vec3(8., 4., phase * 10.);\n"
"		cam_at = vec3(0., 0., -30.);\n"
"	} else if (npat < 4.) {\n"
"		cam_pos = vec3(0., 2., 20. + phase * 8.);\n"
"		cam_at = vec3(0., 0., -30.);\n"
"	} else if (npat < 5.) {\n"
"		do_lamps = 1. - quantize(phase, 4.);\n"
"	}\n"
"\n"
"	if (npat > 4.) {\n"
"		do_lamps = 0.;\n"
"		do_sky = 1.;\n"
"		do_color = 1.;\n"
"		//do_lake = 1.;\n"
"		vec2 pos = (hash2v(npat) - .5) * vec2(20., 4.);\n"
"		cam_at = mix(vec3(-20., -4., -30.), vec3(20., 0., 30.), hash3v(npat).yzx);\n"
"		cam_pos = vec3(pos.x, pos.y + 2., 30. - phase * 40.);\n"
"	}\n"
"\n"
"	if (npat > 6.) {\n"
"		cam_pos = vec3(-8. + 16.*mod(npat-7.,2.), 2., -20. + phase * 40.);\n"
"		cam_at = vec3(0., -8., -3.);\n"
"		do_lake = 1.;\n"
"		//do_wall_rough = .01;\n"
"	}\n"
"\n"
"	if (npat > 8.) {\n"
"		cam_pos = vec3(-8. + 16.*mod(npat-7.,2.), 7.,\n"
"			(-20. + phase * 40.) * (npat>9.?-1.:1.));\n"
"		cam_at = vec3(0., -8., 1.) + cam_pos;\n"
"	}\n"
"\n"
"	if (npat > 10.) {\n"
"		do_move = vec2(0., 2.);\n"
"		vec3 from = vec3(\n"
"			sin(npat*17.)*10.,\n"
"			2. + 2. * cos(npat*4.),\n"
"			0.\n"
"		);\n"
"		vec3 to = vec3(\n"
"			sin(npat*11.)*10.,\n"
"			2. + 2. * cos(npat*14.),\n"
"			0.\n"
"		);\n"
"		cam_roll = .4 * (mix(hash1(npat),hash1(npat+3.),phase)-.5);\n"
"		cam_pos = vec3(0., 2., 30. - phase * 16.) + mix(from, to, phase);\n"
"		cam_at = vec3(0., 0., -30.);\n"
"	}\n"
"\n"
"	w_off = do_move * smoothstep(640., 704., t) * (t-640.);\n"
"	w_h = smoothstep(256., 768., t);\n"
"\n"
"	if (npat > 10.) {\n"
"		w_kust = 1.;\n"
"	}\n"
"\n"
"	if (npat > 12.) {\n"
"		w_xlam = 1.;\n"
"	}\n"
"\n"
"	do_color_env *= smoothstep(1880., 1800., t);\n"
"\n"
"	float primary_length = 0;\n"
"	vec3 C = vec3(0.);\n"
"	for (float s=0.;s<S;++s){\n"
"		seed = fract(t + hash1(s) + hash1(uv.x) + hash1(uv.y));\n"
"		vec2 aauv = uv + (vec2(hash1(seed+uv.x), hash1(seed+uv.y)) - .5) / R;\n"
"		seed = fract(hash1(aauv.x) + hash1(aauv.y) + s);\n"
"\n"
"		vec3 O = cam_pos;\n"
"		mat3 view = lookat(cam_pos, cam_at, vec3(sin(cam_roll),1.,0.));\n"
"		vec3 D = view * normalize(vec3(aauv, -1.));\n"
"\n"
"		vec3 kc = vec3(1.);\n"
"		for (float b=0.;b<3.;++b) {\n"
"			float l = tracegrid(O, D, 128.);\n"
"			if (b == 0.) primary_length = l;\n"
"			vec3 P = O + D * l;\n"
"			vec3 ci = CI;\n"
"			vec3 Pl = fract(P);\n"
"			vec2 puv = abs(Pl.xz*N.y + Pl.xy*N.z + Pl.yz*N.x);\n"
"			vec2 Puv = abs(ci.xz*N.y + ci.xy*N.z + ci.yz*N.x);\n"
"\n"
"			vec3 me = vec3(0.);\n"
"			vec3 ma = vec3(.7);\n"
"			float mr = .6;\n"
"\n"
"			if (do_color > 0. && wmat == 1.) {\n"
"				ma = vec3(.3, .8, .4) * (.6 + .4*hash2(Puv+quantize2(puv, vec2(4.))));\n"
"				mr = .9;\n"
"			} else if (wmat == 0.) {\n"
"				// Ceiling\n"
"				if (N.y < 0. || (do_sky > 0. && ci.y > 0.)) {\n"
"					if (do_sky > 0.) {\n"
"						vec3 skydir = normalize(ci);\n"
"						//float skyl = 8. / skydir.y; vec2 skypos = ci.xz * skyl;\n"
"						me += skyc(skydir, normalize(vec3(vec2(1.,-1.)+vec2(-2.,2.)*((t-1000.)/500.),1.).xzy));\n"
"					}\n"
"\n"
"					if (do_lamps > 0.) {\n"
"						vec3 lamps = vec3(1.5)\n"
"							* step(box2(rep2(ci.xz, vec2(10.)), vec2(1.)), 1.)\n"
"							//* step(ci.z, (1. - phase) * 30. - 30.)\n"
"							* step(ci.z, do_lamps * 40. - 30.)\n"
"							//* step(ci.z, 30. - do_lamps * 30.)\n"
"							;\n"
"						me += lamps;\n"
"					}\n"
"				} else if (N.y == 0.) {\n"
"					// lower walls\n"
"					mr = .4;// + .1 * hash2(Puv);\n"
"					//mr = do_wall_rough;\n"
"					if (do_borders > 0.) {\n"
"						vec2 puvm = step(abs(puv - .5), vec2(.48));\n"
"						float mask = puvm.x * puvm.y;\n"
"						//ma.xy *= puvm;\n"
"						ma *= (.1 + .9 * mask);\n"
"						//me *= mask;\n"
"					}\n"
"				}\n"
"			} else if (wmat == 2.) {\n"
"				ma = vec3(.3, .7, .4) * (.6 + .4*hash2(Puv+quantize2(puv, vec2(4.))));\n"
"				mr = .2;\n"
"			}\n"
"\n"
"			if (do_lake > 0. && N.y > 0. && ci.y <= -8.) {\n"
"				ma = vec3(1.);\n"
"				mr = .05;\n"
"			}\n"
"\n"
"			// THE REST OF THE PATH TRACER KEKW\n"
"			vec3 c = kc * me;\n"
"			if (!any(isnan(c))) C += c;\n"
"			kc *= ma;\n"
"			O = P + .01 * N;\n"
"			D = normalize(mix(\n"
"				reflect(D, N),\n"
"				vec3(hash1(seed+=P.z),hash1(seed+=D.x),hash1(seed+=P.y))-.5,\n"
"				mr));\n"
"			D *= sign(dot(D, N));\n"
"			//if (all(lessThan(kc,vec3(.001)))) break;\n"
"		}\n"
"	}\n"
"	C /= S;\n"
"	C *= do_color_env;\n"
"	\n"
"	gl_FragColor = vec4(pow(C,vec3(1./2.2)), primary_length);\n"
"}\n"
;

#ifdef _DEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nShowCmd) {
	(void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nShowCmd;
#else
int WinMainCRTStartup(void) {
	ShowCursor(FALSE);
	ChangeDisplaySettingsA((DEVMODEA*)&devmode, CDS_FULLSCREEN);
#endif

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
#ifdef _DEBUG
#define X(t, n) \
	n = (t)(void*)wglGetProcAddress(#n); \
	if (!n) MessageBoxA(NULL, #n, "wglGetProcAddress", MB_ICONERROR);
	LIST_GL_FUNCS(X)
#undef X

	{
		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			char buf[1024];
			glGetProgramInfoLog(program, sizeof(buf)-1, NULL, buf);
			MessageBoxA(NULL, buf, "Shader error", MB_ICONERROR);
			ExitProcess(0);
		}
	}
#endif

	glUseProgram(program);

	const GLint t_loc = glGetUniformLocation(program, "t");

	const DWORD st = GetTickCount();
	for (;;) {
		if (GetAsyncKeyState(VK_ESCAPE))
			break;

		glUniform1f(t_loc, 16.f*(GetTickCount() - st)/1000.f);
		glRects(-1, -1, 1, 1);
		SwapBuffers(hdc);

		// Pump messages to tell Windows we're alive
		PeekMessageA(NULL, 0, 0, 0, PM_REMOVE);
	}

	ExitProcess(0);
}
