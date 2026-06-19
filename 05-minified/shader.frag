#version 130
uniform float t;
const vec2 R = vec2(1920.,1080.);

const float S = 4.;
float hash1(float x){return fract(sin(x)*265871.1723);}
float hash2(vec2 x){return hash1(dot(x,vec2(11.,313.)));}
float hash3(vec3 x){return hash1(dot(x,vec3(3.2,57.55,117.234)));}
//vec3 hash33(vec3 x){return vec3(hash3(x),hash2(x.yz),hash1(x.x));}
vec3 hash3v(float x){return vec3(hash1(x),hash1(x+.1),hash1(x+.2));}
vec2 hash2v(float x){return vec2(hash1(x),hash1(x+.1));}

float noise1(float v) {
	float V=floor(v);v-=V;
	return mix(hash1(V),hash1(V+1),v);
}

float noise2(vec2 v) {
	vec2 /*E=vec2(0.,1.), */V=floor(v); v-=V;
	return mix(
		mix(hash2(V),     hash2(V+vec2(1.,0.)),v.x),
		mix(hash2(V+vec2(0.,1.)),hash2(V+vec2(1.)),v.x),v.y);
}

float fbm(vec2 v) {
	return noise2(v)*.5
		+ noise2(v*2.1)*.25
		+ noise2(v*3.9)*.125
		//+ noise2(v*8.1)*.125
		;
}

/*
float noise3(vec3 v) {
	vec3 V = floor(v);v-=V;
	return mix(mix(
		mix(hash3(V),hash3(V+E.xyy),v.x),
			mix(hash3(V+E.yxy),hash3(V+E.xxy),v.x),v.y),
			mix(mix(hash3(V+E.yyx),hash3(V+E.xyx),v.x),
			mix(hash3(V+E.yxx),hash3(V+E.xxx),v.x),v.y),v.z);
}
*/

float quantize(float v, float n) { return floor(v*n)/n; }
vec2 quantize2(vec2 v, vec2 n) { return floor(v*n)/n; }

//mat2 Rm(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}

float rep(float p, float s) { return mod(p, s) - s*.5; }
vec2 rep2(vec2 p, vec2 s) { return mod(p, s) - s*.5; }
//vec3 rep3(vec3 p, vec3 s) { return mod(p, s) - s*.5; }
//float box2(vec2 p, vec2 s) { p = abs(p) - s; return max(p.x,p.y); }
//float box3(vec3 p, vec3 s) { p = abs(p) - s; return max(max(p.x,p.y),p.z); }
float vmax3(vec3 v){return max(max(v.x,v.y),v.z);}
float vmax2(vec2 v){return max(v.x,v.y);}
float box3(vec3 p, vec3 s) { return vmax3(abs(p)-s); }
float box2(vec2 p, vec2 s) { return vmax2(abs(p)-s); }

vec3 vminc(vec3 v){return step(v.xyz,v.yzx)*step(v.xyz,v.zxy);}
vec3 vmaxc(vec3 v){return step(v.yzx,v.xyz)*step(v.zxy,v.xyz);}

#define RM(a) mat2(cos(a),sin(a),-sin(a),cos(a))

vec2 w_off = vec2(0.);
float w_kust = 0.;
float w_h = 0.;
float w_xlam = 0.;
float wmat = 0.;
// 0 == wall
// 1 == ground
// 2 == kust
float w(vec3 p) {
	float d = 1e6;
	vec3 po = p;

	//float table = box3(p+vec3(0.,3.,0.), vec3(2., 2., 2.));
	//d = min(d, table);

	wmat = 0.;
	float hm = fbm((p.xz - w_off)*.1);
	float dm = po.y + 8. - (hm - .3) * 18. * w_h;
	if (d > dm) { d = dm; wmat = 1.; }

	float bounding = -box3(p, vec3(20.,8.,30.));
	if (d > bounding) { d = bounding; wmat = 0.; }

	if (w_kust != 0.) {
		vec3 kp = vec3(rep2(p.xz-w_off,vec2(12., 25.)), p.y - hm * 8. + 6.).xzy;
		float kust = length(kp)-3.;
		if (d > kust) { d = kust; wmat = 2.; }
	}

	if (w_xlam > 0.)
	{
		d = min(d, hash3(.01*vec3(p.xz-floor(w_off), p.y).xzy)*20.1-.01);
	}
	
	return d;
}

vec3 CI,N;
float tracegrid(vec3 O, vec3 D, float steps){
	vec3 Di = 1. / D, Ds = sign(D);
	vec3 ci = CI = floor(O);
	vec3 sd = (ci - O + .5 + Ds * .5) * Di;
	vec3 n = vec3(0.);
	for (float i = 0.; i < steps; i++) {
		float ww = w(ci + .5);
		if (ww < 0.) {
			N = - n * Ds;
			sd = (ci - O + .5 - Ds * .5) * Di;
			return max(max(sd.x,sd.y),sd.z);
		}
		CI = ci;
		n = vminc(sd);
		sd += n * Ds * Di;
		ci += n * Ds;
		//if (dot(ci,ci) > 2048.) break;
	}

	return 1e6;
}

mat3 lookat(vec3 pos, vec3 at, vec3 up) {
	vec3 z = normalize(pos - at);
	vec3 x = normalize(cross(up, z));
	up = normalize(cross(z, x));
	return mat3(x, up, z);
}

vec3 skyc(vec3 d, vec3 sund) {
	float clouds_l = 8. / d.y; vec2 cloud_pos = d.xz * clouds_l;
	vec3 sky = 1. * vec3(.1,.3,.7);
	float clouds = step(fbm(-w_off*.041+cloud_pos*.1), .3);
	float dotsun = pow(max(dot(d,sund)*1.1, 0.), 20.);
	sky = mix(sky, 2. * vec3(.9,.6,.4), dotsun);
	sky = mix(sky, vec3(1.), clouds);
	return sky;

	// sky = mix(sky, vec3(1.), step(fbm(-w_off*.041+d*.1), .3));
	// vec3 base_color = mix(vec3(.2), sky, max(0.,pow(d.y, .7)));
	// sky = mix(sky, 10. * vec3(.9,.6,.4), step(length(ci.xz-vec2(10.,-10.)), 3.));
	//return mix(vec3(.005), base_color * 6., max(0.,sund.y));
}

void main() {
	vec2 uv = gl_FragCoord.xy / R.xy - .5; uv.x *= R.x/R.y;
	//float seed = fract(t + dot(vec2(.3,5.),gl_FragCoord.xy));
	float seed;// = fract(t + hash1(uv.x) + hash1(uv.y));

	float do_sky = 0.;
	float do_lamps = 2.;
	float do_lake = 0.;
	float do_color = 0.;
	float do_borders = 0.;
	vec2 do_move = vec2(.2);
	//float do_wall_rough = .3;

	vec3 cam_pos = vec3(0.,0.,28.), cam_at = vec3(0.,0.,-30.);
	float cam_roll = 0.;
	float pat = ((t<1376.)?t:t+32.) / 128.;
	float phase = fract(pat);
	float npat = floor(pat);

	float do_color_env = smoothstep(0., .125, phase) * smoothstep(1., .75, phase);

	if (npat < 1.) {
		//vec2 pos = npat * (hash2v(npat) - .5) * vec2(5., 4.);
		cam_pos = vec3(0., 4., -25. + phase * 30.);
		cam_at = vec3(0., 0., -30.);
	} else if (npat < 2.) {
		cam_pos = vec3(10., 4., -25. + phase * 30.);
		cam_at = vec3(-10., 0., cam_pos.z);
	} else if (npat < 3.) {
		cam_pos = vec3(8., 4., phase * 10.);
		cam_at = vec3(0., 0., -30.);
	} else if (npat < 4.) {
		cam_pos = vec3(0., 2., 20. + phase * 8.);
		cam_at = vec3(0., 0., -30.);
	} else if (npat < 5.) {
		do_lamps = 1. - quantize(phase, 4.);
	}

	if (npat > 4.) {
		do_lamps = 0.;
		do_sky = 1.;
		do_color = 1.;
		//do_lake = 1.;
		vec2 pos = (hash2v(npat) - .5) * vec2(20., 4.);
		cam_at = mix(vec3(-20., -4., -30.), vec3(20., 0., 30.), hash3v(npat).yzx);
		cam_pos = vec3(pos.x, pos.y + 2., 30. - phase * 40.);
	}

	if (npat > 6.) {
		cam_pos = vec3(-8. + 16.*mod(npat-7.,2.), 2., -20. + phase * 40.);
		cam_at = vec3(0., -8., -3.);
		do_lake = 1.;
		//do_wall_rough = .01;
	}

	if (npat > 8.) {
		cam_pos = vec3(-8. + 16.*mod(npat-7.,2.), 7.,
			(-20. + phase * 40.) * (npat>9.?-1.:1.));
		cam_at = vec3(0., -8., 1.) + cam_pos;
	}

	if (npat > 10.) {
		do_move = vec2(0., 2.);
		vec3 from = vec3(
			sin(npat*17.)*10.,
			2. + 2. * cos(npat*4.),
			0.
		);
		vec3 to = vec3(
			sin(npat*11.)*10.,
			2. + 2. * cos(npat*14.),
			0.
		);
		cam_roll = .4 * (mix(hash1(npat),hash1(npat+3.),phase)-.5);
		cam_pos = vec3(0., 2., 30. - phase * 16.) + mix(from, to, phase);
		cam_at = vec3(0., 0., -30.);
	}

	w_off = do_move * smoothstep(640., 704., t) * (t-640.);
	w_h = smoothstep(256., 768., t);

	if (npat > 10.) {
		w_kust = 1.;
	}

	if (npat > 12.) {
		w_xlam = 1.;
	}

	do_color_env *= smoothstep(1880., 1800., t);

	float primary_length = 0;
	vec3 C = vec3(0.);
	for (float s=0.;s<S;++s){
		seed = fract(t + hash1(s) + hash1(uv.x) + hash1(uv.y));
		vec2 aauv = uv + (vec2(hash1(seed+uv.x), hash1(seed+uv.y)) - .5) / R;
		seed = fract(hash1(aauv.x) + hash1(aauv.y) + s);

		vec3 O = cam_pos;
		mat3 view = lookat(cam_pos, cam_at, vec3(sin(cam_roll),1.,0.));
		vec3 D = view * normalize(vec3(aauv, -1.));

		vec3 kc = vec3(1.);
		for (float b=0.;b<3.;++b) {
			float l = tracegrid(O, D, 128.);
			if (b == 0.) primary_length = l;
			vec3 P = O + D * l;
			vec3 ci = CI;
			vec3 Pl = fract(P);
			vec2 puv = abs(Pl.xz*N.y + Pl.xy*N.z + Pl.yz*N.x);
			vec2 Puv = abs(ci.xz*N.y + ci.xy*N.z + ci.yz*N.x);

			vec3 me = vec3(0.);
			vec3 ma = vec3(.7);
			float mr = .6;

			if (do_color > 0. && wmat == 1.) {
				ma = vec3(.3, .8, .4) * (.6 + .4*hash2(Puv+quantize2(puv, vec2(4.))));
				mr = .9;
			} else if (wmat == 0.) {
				// Ceiling
				if (N.y < 0. || (do_sky > 0. && ci.y > 0.)) {
					if (do_sky > 0.) {
						vec3 skydir = normalize(ci);
						//float skyl = 8. / skydir.y; vec2 skypos = ci.xz * skyl;
						me += skyc(skydir, normalize(vec3(vec2(1.,-1.)+vec2(-2.,2.)*((t-1000.)/500.),1.).xzy));
					}

					if (do_lamps > 0.) {
						vec3 lamps = vec3(1.5)
							* step(box2(rep2(ci.xz, vec2(10.)), vec2(1.)), 1.)
							//* step(ci.z, (1. - phase) * 30. - 30.)
							* step(ci.z, do_lamps * 40. - 30.)
							//* step(ci.z, 30. - do_lamps * 30.)
							;
						me += lamps;
					}
				} else if (N.y == 0.) {
					// lower walls
					mr = .4;// + .1 * hash2(Puv);
					//mr = do_wall_rough;
					if (do_borders > 0.) {
						vec2 puvm = step(abs(puv - .5), vec2(.48));
						float mask = puvm.x * puvm.y;
						//ma.xy *= puvm;
						ma *= (.1 + .9 * mask);
						//me *= mask;
					}
				}
			} else if (wmat == 2.) {
				ma = vec3(.3, .7, .4) * (.6 + .4*hash2(Puv+quantize2(puv, vec2(4.))));
				mr = .2;
			}

			if (do_lake > 0. && N.y > 0. && ci.y <= -8.) {
				ma = vec3(1.);
				mr = .05;
			}

			// THE REST OF THE PATH TRACER KEKW
			vec3 c = kc * me;
			if (!any(isnan(c))) C += c;
			kc *= ma;
			O = P + .01 * N;
			D = normalize(mix(
				reflect(D, N),
				vec3(hash1(seed+=P.z),hash1(seed+=D.x),hash1(seed+=P.y))-.5,
				mr));
			D *= sign(dot(D, N));
			//if (all(lessThan(kc,vec3(.001)))) break;
		}
	}
	C /= S;
	C *= do_color_env;
	
	gl_FragColor = vec4(pow(C,vec3(1./2.2)), primary_length);
}
