//!HOOK CHROMA
//!BIND HOOKED
//!SAVE PASS1
//!WHEN LUMA.w LUMA.h * CHROMA.w CHROMA.h * >
//!DESC jinc upscale chroma pass1

////////////////////////////////////////////////////////////////////////
// USER CONFIGURABLE, PASS 1 (sigmoidize)
//
// CAUTION! should use the same settings for "USER CONFIGURABLE, PASS 2" below
//
// sigmoidal curve
#define C 6.5 // contrast, equivalent to mpv's --sigmoid-slope
#define M 0.75 // midpoint, equivalent to mpv's --sigmoid-center
//
////////////////////////////////////////////////////////////////////////

// based on https://github.com/ImageMagick/ImageMagick/blob/main/MagickCore/enhance.c
#define sigmoidize(rgba) (M - log(1.0 / ((1.0 / (1.0 + exp(C * (M - 1.0))) - 1.0 / (1.0 + exp(C * M))) * (rgba) + 1.0 / (1.0 + exp(C * M))) - 1.0) / C)

vec4 hook()
{
	return vec4(sigmoidize(clamp(linearize(HOOKED_tex(HOOKED_pos)).xy, 0.0, 1.0)), 0.0, 0.0);
}

//!HOOK CHROMA
//!BIND PASS1
//!SAVE PASS2
//!WIDTH LUMA.w
//!HEIGHT LUMA.h
//!WHEN LUMA.w LUMA.h * CHROMA.w CHROMA.h * >
//!DESC jinc upscale chroma pass2

////////////////////////////////////////////////////////////////////////
// KERNEL FUNCTIONS LIST
//
#define GINSENG 1
#define COSINE 2
#define GARAMOND 3
#define BLACKMAN 4
#define GNW 5
#define SAID 6
#define FSR 7
#define BCSPLINE 8
//
////////////////////////////////////////////////////////////////////////
// USER CONFIGURABLE, PASS 2 (upsample and desigmoidize)
//
#define K GINSENG // kernel function, see "KERNEL FUNCTIONS LIST"
#define R 2.0 // kernel radius, (0.0, inf)
#define B 1.0 // kernel blur, (0.0, inf)
#define AR 1.0 //antiringing strenght, [0.0, 1.0]
//
// kernel function parameters
#define P1 0.0 // COSINE: n, GARAMOND: n, BLACKMAN: a, GNW: s, SAID: chi, FSR: b, BCSPLINE: B
#define P2 0.0 // GARAMOND: m, BLACKMAN: n, GNW: n, SAID: eta, FSR: c, BCSPLINE: C
//
// CAUTION! should use the same settings for "USER CONFIGURABLE, PASS 1" above
//
#define C 6.5 // contrast, equivalent to mpv's --sigmoid-slope
#define M 0.75 // midpoint, equivalent to mpv's --sigmoid-center
//
////////////////////////////////////////////////////////////////////////

#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.785398163397448309616
#define M_2_PI 0.636619772367581343076
#define EPS 1e-6

#define J1(x) ((x) < 2.293116 ? (x) / 2.0 - (x) * (x) * (x) / 16.0 + (x) * (x) * (x) * (x) * (x) / 384.0 - (x) * (x) * (x) * (x) * (x) * (x) * (x) / 18432.0 : sqrt(M_2_PI / (x)) * (1.0 + 3.0 / 16.0 / ((x) * (x)) - 99.0 / 512.0 / ((x) * (x) * (x) * (x))) * cos((x) - 3.0 * M_PI_4 + 3.0 / 8.0 / (x) - 21.0 / 128.0 / ((x) * (x) * (x))))

#define jinc(x) ((x) < EPS ? M_PI_2 / B : J1(M_PI / B * (x)) / (x))

#if K == GINSENG
	#define k(x) (jinc(x) * ((x) < EPS ? M_PI / R : sin(M_PI / R * (x)) / (x)))
#elif K == COSINE
	#define k(x) (jinc(x) * pow(cos(M_PI_2 / R * (x)), P1))
#elif K == GARAMOND
	#define k(x) (jinc(x) * pow(1.0 - pow((x) / R, P1), P2))
#elif K == BLACKMAN
	#define k(x) (jinc(x) * pow((1.0 - P1) / 2.0 + 0.5 * cos(M_PI / R * (x)) + P1 / 2.0 * cos(2.0 * M_PI / R * (x)), P2))
#elif K == GNW
	#define k(x) (jinc(x) * exp(-pow((x) / P1, P2)))
#elif K == SAID
	#define k(x) (jinc(x) * cosh(sqrt(2.0 * P2) * M_PI * P1 / (2.0 - P2) * (x)) * exp(-M_PI * M_PI * P1 * P1 / ((2.0 - P2) * (2.0 - P2)) * (x) * (x)))
#elif K == FSR
	#undef R
	#define R 2.0
	#define k(x) ((1.0 / (2.0 * P1 - P1 * P1) * (P1 / (P2 * P2) * (x) * (x) - 1.0) * (P1 / (P2 * P2) * (x) * (x) - 1.0) - (1.0 / (2.0 * P1 - P1 * P1) - 1.0)) * (0.25 * (x) * (x) - 1.0) * (0.25 * (x) * (x) - 1.0))
#elif K == BCSPLINE
	#undef R
	#define R 2.0
	#define k(x) ((x) < 1.0 ? (12.0 - 9.0 * P1 - 6.0 * P2) * (x) * (x) * (x) + (-18.0 + 12.0 * P1 + 6.0 * P2) * (x) * (x) + (6.0 - 2.0 * P1) : (-P1 - 6.0 * P2) * (x) * (x) * (x) + (6.0 * P1 + 30.0 * P2) * (x) * (x) + (-12.0 * P1 - 48.0 * P2) * (x) + (8.0 * P1 + 24.0 * P2))
#endif

#define get_weight(x) ((x) < R ? k(x) : 0.0)

// based on https://github.com/ImageMagick/ImageMagick/blob/main/MagickCore/enhance.c
#define desigmoidize(rgba) (1.0 / (1.0 + exp(C * (M - (rgba)))) - 1.0 / (1.0 + exp(C * M))) / ( 1.0 / (1.0 + exp(C * (M - 1.0))) - 1.0 / (1.0 + exp(C * M)))

vec4 hook()
{
	vec2 f = fract(PASS1_pos * PASS1_size - 0.5);
	vec2 base = PASS1_pos - f * PASS1_pt;
	vec2 color;
	vec2 csum = vec2(0.0);
	float weight;
	float wsum = 0.0;
	vec2 lo = vec2(1e9);
	vec2 hi = vec2(-1e9);
	for (float y = 1.0 - ceil(R); y <= ceil(R); ++y) {
		for (float x = 1.0 - ceil(R); x <= ceil(R); ++x) {
			weight = get_weight(length(vec2(x, y) - f));
			color = PASS1_tex(base + vec2(x, y) * PASS1_pt).xy;
			csum += color * weight;
			wsum += weight;
			if (AR > 0.0 && y >= 0.0 && y <= 1.0 && x >= 0.0 && x <= 1.0) {
				lo = min(lo, color);
				hi = max(hi, color);
			}
		}
	}
	csum /= wsum;
	if (AR > 0.0)
		csum = mix(csum, clamp(csum, lo, hi), AR);
	return vec4(desigmoidize(csum), 0.0, 0.0);
}

//!HOOK CHROMA
//!BIND PASS2
//!SAVE PASS3
//!WIDTH LUMA.w
//!HEIGHT LUMA.h
//!WHEN LUMA.w LUMA.h * CHROMA.w CHROMA.h * >
//!DESC jinc upscale chroma pass4

////////////////////////////////////////////////////////////////////////
// USER CONFIGURABLE, PASS 3 (blur in y axis)
//
// CAUTION! should use the same settings for "USER CONFIGURABLE, PASS 4" below
//
#define S 1.0 // blur spread or amount, (0.0, inf)
#define R 2.0 // kernel radius, (0.0, inf)
//
////////////////////////////////////////////////////////////////////////

#define get_weight(x) (exp(-(x) * (x) / (2.0 * S * S)))

vec4 hook()
{
	float weight;
	vec2 csum = PASS2_tex(PASS2_pos).xy;
	float wsum = 1.0;
	for (float i = 1.0; i <= R; ++i) {
		weight = get_weight(i);
		csum += (PASS2_tex(PASS2_pos + vec2(0.0, -i) * PASS2_pt).xy + PASS2_tex(PASS2_pos + vec2(0.0, i) * PASS2_pt).xy) * weight;
		wsum += 2.0 * weight;
	}
	return vec4(csum / wsum, 0.0, 0.0);
}

//!HOOK CHROMA
//!BIND PASS2
//!BIND PASS3
//!WIDTH LUMA.w
//!HEIGHT LUMA.h
//!WHEN LUMA.w LUMA.h * CHROMA.w CHROMA.h * >
//!DESC jinc upscale chroma pass4

////////////////////////////////////////////////////////////////////////
// USER CONFIGURABLE, PASS 4 (blur in x axis and apply unsharp mask)
//
// CAUTION! should use the same settings for "USER CONFIGURABLE, PASS 3" above
//
#define S 1.0 // blur spread or amount, (0.0, inf)
#define R 2.0 // kernel radius, (0.0, inf)
//
// sharpnes
#define A 0.5 // amount of sharpening [0.0, inf)
//
////////////////////////////////////////////////////////////////////////

#define get_weight(x) (exp(-(x) * (x) / (2.0 * S * S)))

vec4 hook()
{
	float weight;
	vec2 csum = PASS3_tex(PASS3_pos).xy;
	float wsum = 1.0;
	for (float i = 1.0; i <= R; ++i) {
		weight = get_weight(i);
		csum += (PASS3_tex(PASS3_pos + vec2(-i, 0.0) * PASS3_pt).xy + PASS3_tex(PASS3_pos + vec2(i, 0.0) * PASS3_pt).xy) * weight;
		wsum += 2.0 * weight;
	}
	vec2 original = PASS2_tex(PASS2_pos).xy;
	return delinearize(vec4(original + (original - csum / wsum) * A, 0.0, 0.0));
}
