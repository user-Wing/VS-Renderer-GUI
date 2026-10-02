//!HOOK LUMA
//!BIND HOOKED
//!WIDTH OUTPUT.w
//!HEIGHT OUTPUT.h
//!WHEN OUTPUT.w OUTPUT.h * LUMA.w LUMA.h * = !
//!DESC fast Catmull-Rom

vec4 hook()
{
	const vec2 f = fract(HOOKED_pos * HOOKED_size - 0.5);
	const vec2 f2 = f * f;
	const vec2 f3 = f2 * f;

	// Catmull-Rom weights.
	const vec2 w0 = f2 - 0.5 * (f3 + f);
	const vec2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
	const vec2 w3 = 0.5 * (f3 - f2);
	const vec2 w2 = 1.0 - w0 - w1 - w3;
	const vec2 w12 = w1 + w2;

	// Texel coords.
	const vec2 tc = HOOKED_pos - f * HOOKED_pt;
	const vec2 tc0 = tc - 1.0 * HOOKED_pt;
	const vec2 tc3 = tc + 2.0 * HOOKED_pt;
	const vec2 tc12 = tc + w2 / w12 * HOOKED_pt;

	// Combined weights.
	const float w12w0 = w12.x * w0.y;
	const float w0w12 = w0.x * w12.y;
	const float w12w12 = w12.x * w12.y;
	const float w3w12 = w3.x * w12.y;
	const float w12w3 = w12.x * w3.y;

	float c = HOOKED_tex(vec2(tc12.x, tc0.y)).x * w12w0;
	c += HOOKED_tex(vec2(tc0.x, tc12.y)).x * w0w12;
	c += HOOKED_tex(tc12).x * w12w12;
	c += HOOKED_tex(vec2(tc3.x, tc12.y)).x * w3w12;
	c += HOOKED_tex(vec2(tc12.x, tc3.y)).x * w12w3;

	// Normalize.
	c /= w12w0 + w0w12 + w12w12 + w3w12 + w12w3;

	return vec4(c, 0.0, 0.0, 0.0);
}