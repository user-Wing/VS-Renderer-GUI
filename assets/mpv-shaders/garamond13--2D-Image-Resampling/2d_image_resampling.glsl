//!HOOK MAIN
//!BIND HOOKED
//!WHEN OUTPUT.w OUTPUT.h * MAIN.w MAIN.h * = !
//!WIDTH OUTPUT.w
//!HEIGHT OUTPUT.h
//!DESC 2D Image Scaling

// Forward declarations of kernel filter functions.
//

// Usage example as window: sinc(x / SUPPORT)
// Usage example as base: sinc(x / BLUR)
float sinc(float x);

// Usage example as window: jinc(x / SUPPORT * 1.21967 /* first zero */ )
// Usage example as base: jinc(x / BLUR)
float jinc(float x);

// Usage example as window: sphinx(x / SUPPORT * 1.430297 /* first zero */ )
float sphinx(float x);

// Usage example as window: cosine(x / SUPPORT)
float cosine(float x);

// cosine: n = 1.
// hann: n = 2.
// box: n = 0.
// Has to be satisfied: n >= 0.
// Usage example as window: power_of_cosine(x / SUPPORT, 1.0)
float power_of_cosine(float x, float n);

// Usage example as window: hann(x / SUPPORT)
float hann(float x);

// common tukey: a = 0.5.
// hann: a = 1.
// box: a = 0.
// Usage example as window: tukey(x / SUPPORT, 0.5)
float tukey(float x, float a);

// Usage example as window: hamming(x / SUPPORT)
float hamming(float x);

// common blackman: a = 0.16.
// hann: a = 0.
// Usage example as window: blackman(x / SUPPORT, 0.16)
float blackman(float x, float a);

// blackman: n = 1
// common blackman: a = 0.16, n = 1.
// hann: a = 0, n = 1.
// cosine: a = 0, n = 0.5
// box: n = 0.
// Has to be satisfied: n >= 0.
// Has to be satisfied: if n != 1, a <= 0.16.
// Usage example as window: power_of_blackman(x / SUPPORT, 0.16, 1.0)
float power_of_blackman(float x, float a, float n);

// Usage example as window: exact_blackman(x / SUPPORT)
float exact_blackman(float x);

// Usage example as window: nuttall(x / SUPPORT)
float nuttall(float x);

// Usage example as window: blackman_nuttall(x / SUPPORT)
float blackman_nuttall(float x);

// Usage example as window: blackman_harris(x / SUPPORT)
float blackman_harris(float x);

// Usage example as window: flat_top(x / SUPPORT)
float flat_top(float x);

// Usage example as window: bohman(x / SUPPORT)
float bohman(float x);

// Usage example as window: welch(x / SUPPORT)
float welch(float x);

// Usage example as window: kaiser(x / SUPPORT, 8.5)
float kaiser(float x, float beta);

// kaiser: n = 2.
// Has to be satisfied: n >= 0.
// Usage example as window: kaiser_garamond(x / SUPPORT, 8.5, 2.0)
float kaiser_garamond(float x, float beta, float n);

// Usage example as window: parzen(x / SUPPORT)
float parzen(float x);

// Usage example as window: gaussian(x / SUPPORT, 0.4)
float gaussian(float x, float sigma);

// hann–poisson: hann(x / SUPPORT) * poisson(x, alpha).
// Usage example as window: poisson(x / SUPPORT, 2.0)
float poisson(float x, float alpha);

// Usage example as window: cauchy(x / SUPPORT, 3.0)
float cauchy(float x, float alpha);

// linear: n = 1 m = 1.
// welch: n = 2, m = 1.
// box: n -> inf, m <= 1.
// box: m = 0.
// garamond window: m = 1.
// Has to be satisfied: n >= 0, m >= 0.
// Usage example as window: power_of_garamond(x / SUPPORT, 2.0, 1.0)
float power_of_garamond(float x, float n, float m);

// gaussian: n = 2.
// box: n -> inf.
// Has to be satisfied: s != 0, n >= 0.
// Usage example as window: generalized_normal_window(x, 2.0, 3.0)
float generalized_normal_window(float x, float s, float n);

// Has to be satisfied: eta != 2.
// Usage example as window: said(x, 0.416, 0.61)
float said(float x, float chi, float eta);

// orthogonal nearest neighbor: SUPPORT = 0.5
// cylindrical nearest neighbor: SUPPORT = sqrt(2.0) / 2.0
// Usage example: box(x)
float box(float x);

// For orthogonal support has to be: 1.0
// For cylindrical support has to be: sqrt(2.0)
// Usage example for orthogonal: linear(x)
// Usage example for cylindrical: linear(x / sqrt(2.0))
// Usage example as window: linear(x / SUPPORT)
float linear(float x);

// Support has to be: 1.5
// Usage example: interpolating_quadratic(x)
float interpolating_quadratic(float x);

// Support has to be: 2.0
// Usage example: sinc_fsr_kernel(x)
float sinc_fsr_kernel(float x);

// Support has to be the second Jinc zero: 2.233131
// Usage example: jinc_fsr_kernel(x)
float jinc_fsr_kernel(float x);

// original fsr kernel: c = 1.
// Has to be satisfied: b != 0, b != 2, c != 0.
// Support has to be: 2.0
// Usage example: modified_fsr_kernel(x, 0.4, 1.0)
float modified_fsr_kernel(float x, float b, float c);

// Support has to be: 2.0
// Usage example: bicubic(x, -0.5)
float bicubic(float x, float a);

// hermite: b = 0, c = 0.
// spline: b = 1, c = 0.
// catmull_rom: b = 0, c = 0.5.
// mitchell: b = 1 / 3, c = 1 / 3.
// robidoux: b = 12 / (19 + 9 * sqrt(2)) ≈ 0.37821575509399866, c = 113 / (58 + 216 * sqrt(2)) ≈ 0.31089212245300067.
// robidouxsharp: b = 6 / (13 + 7 * sqrt(2)) ≈ 0.2620145123990142, c = 7 / (2 + 12 * sqrt(2)) ≈ 0.3689927438004929.
// robidouxsoft: b = (9 - 3 * sqrt(2)) / 7 ≈ 0.67962275898295921, c ≈ 0.1601886205085204.
// Support has to be: 2.0
// Usage example: bc_spline(x, 0.0, 0.5)
float bc_spline(float x, float b, float c);

// Support has to be: 2.0
// Usage example: spline16(x)
float spline16(float x);

// Support has to be: 3.0
// Usage example: spline36(x)
float spline36(float x);

// Support has to be: 4.0
// Usage example: spline64(x)
float spline64(float x);

// Support has to be: 4.0
// Usage example: german(x)
float german(float x);

// Support has to be: 4.5
// Usage example: magic_kernel_sharp(x)
float magic_kernel_sharp(float x);

//

// LIGHTS LIST
#define LIGHT_LINEAR 1
#define LIGHT_SIGMOIDAL 2
#define LIGHT_GAMMA 3

// User configurable
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The light in which the image will be processed.
// See LIGHTS LIST above for available options.
#define LIGHT LIGHT_GAMMA

// Only relevant if LIGHT is set to LIGHT_SIGMOIDAL.
#define CONTRAST 6.0 // Slope of the sigmoid curve. Has to be > 0.
#define MIDPOINT 0.6 // Midpoint of the sigmoid curve. Has to be in range [0, 1].

// Use cylindrical resampling (ideal for jinc base) or orthogonal resampling (ideal for sinc base).
#define USE_CYLINDRICAL false // true - do use, false - do not use (it will use the orthogonal instead).

#define SUPPORT 3.0 // Kernel function support (often named kernel radius). Has to be > 0.
#define BLUR 1.0 // Kernel blur. Blures or sharpens the kernel, 1.0 is no effect. Has to be > 0.
#define ANTIRINGING 0.0 // Should only be used when upsampling. Has to be in range [0, 1].

// Set/make your kernel function here. `sinc(x / BLUR) * hann(x / SUPPORT)` is just an example.
#define KERNEL sinc(x / BLUR) * hann(x / SUPPORT)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Based on https://github.com/ImageMagick/ImageMagick/blob/main/MagickCore/enhance.c
#define C CONTRAST
#define M MIDPOINT
#define sigmoidize(rgba) (M - log(1.0 / ((1.0 / (1.0 + exp(C * (M - 1.0))) - 1.0 / (1.0 + exp(C * M))) * (rgba) + 1.0 / (1.0 + exp(C * M))) - 1.0) / C)
#define desigmoidize(rgba) (1.0 / (1.0 + exp(C * (M - (rgba)))) - 1.0 / (1.0 + exp(C * M))) / ( 1.0 / (1.0 + exp(C * (M - 1.0))) - 1.0 / (1.0 + exp(C * M)))

float get_weight(float x)
{
    if (x <= SUPPORT) {
        return KERNEL;
    }
    else {
        return 0.0;
    }
}

// The main algorithm (main function).
vec4 hook()
{
    // HOOKED_pos: Normalized texture coordinates, vec2(u, v).
    // input_size: Texture dimensions, vec2(width, height).
    // target_size: Output dimesions, vec2(width, height).
    // HOOKED_pt: Texel size, vec2(1.0 / width, 1.0 / height).
    // HOOKED_raw: Texture itself.
    // linearize: A macro for linearization, provided by libplacebo.
    // delinearize: A macro for delinearization, provided by libplacebo.
    // HOOKED_mul: Coefficient to rescale sampled value to [0.0, 1.0], float.
    
    // Convert normalized texture coordinates to pixel coordinates.
    const vec2 xy = HOOKED_pos * input_size;

    // The center of the nearest pixel to the current position.
    const vec2 base = floor(xy - 0.5) + 0.5; 
    
    // The fractional offset from the pixel center.
    const vec2 f = xy - base;
    
    vec4 color;
    vec4 csum = vec4(0.0); // Color sum.
    
    // Set weights to 1 becase we will only multiply weights later on
    // and in case of cylindrical resampling we won't use (overwrite) the y component.
    vec2 weight = vec2(1.0);
    
    float wsum = 0.0; // Weight sum.
    
    // Antiringing.
    vec4 lo = vec4(1e9);
    vec4 hi = vec4(-1e9);

    // When downsampling we need to scale the kernel.
    // When upsampling scale has to be set to 1 (we don't scale the kernel).
    // We only expect proportional scaling!
    const float image_scale = min(target_size.x / input_size.x, target_size.y / input_size.y);
    const float kernel_scale = 1.0 / min(image_scale, 1.0);

    const int kernel_radius = int(ceil(SUPPORT * kernel_scale));
    for (int y = 1 - kernel_radius; y <= kernel_radius; y++) {
        
        // Orthogonal resampling.
        if (!USE_CYLINDRICAL) {
            weight.y = get_weight(abs((float(y) - f.y) / kernel_scale));
        }

        for (int x = 1 - kernel_radius; x <= kernel_radius; x++) {
            if (USE_CYLINDRICAL) {
                weight.x = get_weight(length(vec2(x, y) - f) / kernel_scale);
            }
            else /* Orthogonal resampling */ {
                weight.x = get_weight(abs((float(x) - f.x) / kernel_scale));
            }
            
            // Sample color.
            if (LIGHT == LIGHT_LINEAR) {
                color = linearize(textureLod(HOOKED_raw, (base + vec2(x, y)) * HOOKED_pt, 0.0) * HOOKED_mul);
            }
            else if (LIGHT == LIGHT_SIGMOIDAL) {
                color = sigmoidize(clamp(linearize(textureLod(HOOKED_raw, (base + vec2(x, y)) * HOOKED_pt, 0.0) * HOOKED_mul), 0.0, 1.0));
            }
            else /* LIGHT_GAMMA */ {
                color = textureLod(HOOKED_raw, (base + vec2(x, y)) * HOOKED_pt, 0.0) * HOOKED_mul;
            }
            
            csum += color * weight.x * weight.y;
            wsum += weight.x * weight.y;
            
            // Antiringing.
            if (ANTIRINGING > 0.0 && y >= 0 && y <= 1 && x >= 0 && x <= 1) {
                lo = min(lo, color);
                hi = max(hi, color);
            }
        }
    }

    // Normalization.
    csum /= wsum;
    
    // Antiringing.
    if (ANTIRINGING > 0.0) {
        csum = mix(csum, clamp(csum, lo, hi), ANTIRINGING);
    }
    
    // MPV and libplacebo should do the clamping as necessary.
    if(LIGHT == LIGHT_LINEAR) {
        return delinearize(csum);
    }
    else if (LIGHT == LIGHT_SIGMOIDAL) {
        return delinearize(desigmoidize(csum));
    }
    else /* LIGHT_GAMMA */ {
        return csum;
    }
}

// Source corecrt_math_defines.h.
#define M_PI 3.14159265358979323846 // pi
#define M_PI_2 1.57079632679489661923 // pi/2
#define M_PI_4 0.785398163397448309616 // pi/4
#define M_2_PI 0.636619772367581343076 // 2/pi
#define M_SQRT2 1.41421356237309504880 // sqrt(2)

#define FIRST_JINC_ZERO 1.21966989126650445493
#define SECOND_JINC_ZERO 2.23313059438152863173
#define EPSILON 1e-6

// Forward declarations of math functions.
float bessel_J1(float x);
float bessel_I0(float x);
float bessel_j1(float x);

// Definitions of kernel filter functions.
//

float sinc(float x)
{
    // We wanna check is x == 0.
    if (x < EPSILON) {
        return 1.0;
    }
    else {
        return sin(M_PI * x) / (M_PI * x);
    }
}

float jinc(float x)
{
    // We wanna check is x == 0.
    if (x < EPSILON) {
        return 1.0;
    }
    else {
        return 2.0 * bessel_J1(M_PI * x) / (M_PI * x);
    }
}

// Source https://github.com/haasn/mp/commit/7d10c9b76f39bfd2fe606b8702b39888d117c685
float sphinx(float x)
{
    // We wanna check is x == 0.
    if (x < EPSILON) {
        return 1.0;
    }
    else {
        return 3.0 * bessel_j1(M_PI * x) / (M_PI * x);
    }
}

float cosine(float x)
{
    return cos(M_PI_2 * x);
}

float power_of_cosine(float x, float n)
{
    return pow(cos(M_PI_2 * x), n);
}

float hann(float x)
{
    return 0.5 + 0.5 * cos(M_PI * x);
}

float tukey(float x, float a)
{
    return 0.5 + 0.5 * cos(M_PI * x * a);
}

float hamming(float x)
{
    return 0.54 + 0.46 * cos(M_PI * x);
}

float blackman(float x, float a)
{
    return (1.0 - a) / 2.0 + 0.5 * cos(M_PI * x) + a / 2.0 * cos(2.0 * M_PI * x);
}

float power_of_blackman(float x, float a, float n)
{
    return pow((1.0 - a) / 2.0 + 0.5 * cos(M_PI * x) + a / 2.0 * cos(2.0 * M_PI * x), n);
}

float exact_blackman(float x)
{
    return 7938.0 / 18608.0 + 9240.0 / 18608.0 * cos(M_PI * x) + 1430.0 / 18608.0 * cos(2.0 * M_PI * x);
}

float nuttall(float x)
{
    return 0.355768 + 0.487396 * cos(M_PI * x) + 0.144232 * cos(2.0 * M_PI * x) + 0.012604 * cos(3.0 * M_PI * x);
}

float blackman_nuttall(float x)
{
    return 0.3635819 + 0.4891775 * cos(M_PI * x) + 0.1365995 * cos(2.0 * M_PI * x) + 0.0106411 * cos(3.0 * M_PI * x);
}

float blackman_harris(float x)
{
    return 0.35875 + 0.48829 * cos(M_PI * x) + 0.14128 * cos(2.0 * M_PI * x) + 0.01168 * cos(3.0 * M_PI * x);
}

float flat_top(float x)
{
    return 0.21557895 + 0.41663158 * cos(M_PI * x) + 0.277263158 * cos(2.0 * M_PI * x) + 0.083578947 * cos(3.0 * M_PI * x) + 0.006947368 * cos(4.0 * M_PI * x);
}

float bohman(float x)
{
    return (1.0 - x) * cos(M_PI *  x) + sin(M_PI * x) / M_PI;
}

float welch(float x)
{
    return 1.0 - x * x;
}

float kaiser(float x, float beta)
{
    return bessel_I0(beta * sqrt(1.0 - x * x)) / bessel_I0(beta);
}

float kaiser_garamond(float x, float beta, float n)
{
    return bessel_I0(beta * sqrt(1.0 - pow(x, n))) / bessel_I0(beta);
}

float parzen(float x)
{
    if (x < 0.5) {
        return 1.0 - 6.0 * x * x * (1.0 - x);
    }
    else /* x <= 1.0 */ {
        return 2.0 * (1.0 - x) * (1.0 - x) * (1.0 - x);
    }
}

float gaussian(float x, float sigma)
{
    return exp(-(x * x / (2.0 * sigma * sigma)));
}

float poisson(float x, float alpha)
{
    return exp(-alpha * x);
}

float cauchy(float x, float alpha)
{
    return 1.0 / (1.0 + alpha * alpha * x * x);
}

float power_of_garamond(float x, float n, float m)
{
    // We wanna check is n == 0.
    if (n < EPSILON) {
        return 1.0;
    }
    else {
        return pow(1.0 - pow(x, n), m);
    }
}

// GNW
float generalized_normal_window(float x, float s, float n)
{
    return exp(-pow(x / s, n));
}

float said(float x, float chi, float eta)
{
    return cosh(sqrt(2.0 * eta) * M_PI * chi * x / (2.0 - eta)) * exp(-((M_PI * chi * x / (2.0 - eta)) * (M_PI * chi * x / (2.0 - eta))));
}

float box(float x)
{
    return 1.0;
}

float linear(float x)
{
    return 1.0 - x;
}

float interpolating_quadratic(float x)
{
    if (x < 0.5) {
        return -2.0 * x * x + 1.0;
    }
    else /* x <= 1.5 */ {
        return x * x - 2.5 * x + 1.5;
    }
}

// Source https://github.com/GPUOpen-Effects/FidelityFX-FSR
float sinc_fsr_kernel(float x)
{
    const float base = 25.0 / 16.0 * (2.0 / 5.0 * x * x - 1.0) * (2.0 / 5.0 * x * x - 1.0) - (25.0 / 16.0 - 1.0);
    const float window = (1.0 / 4.0 * x * x - 1.0) * (1.0 / 4.0 * x * x - 1.0);
    return  base * window;
}

float jinc_fsr_kernel(float x)
{
    const float base = 25.0 / 16.0 * (2.0 / 5.0 / (FIRST_JINC_ZERO * FIRST_JINC_ZERO) * x * x - 1.0) * (2.0 / 5.0 / (FIRST_JINC_ZERO * FIRST_JINC_ZERO) * x * x - 1.0) - (25.0 / 16.0 - 1.0);
    const float window = (1.0 / (SECOND_JINC_ZERO * SECOND_JINC_ZERO) * x * x - 1.0) * (1.0 / (SECOND_JINC_ZERO * SECOND_JINC_ZERO) * x * x - 1.0);
    return  base * window;
}

// Based on https://github.com/GPUOpen-Effects/FidelityFX-FSR
float modified_fsr_kernel(float x, float b, float c)
{
    return (1.0 / (2.0 * b - b * b) * (b / (c * c) * x * x - 1.0) * (b / (c * c) * x * x - 1.0) - (1.0 / (2.0 * b - b * b) - 1.0)) * (0.25 * x * x - 1.0) * (0.25 * x * x - 1.0);
}

float bicubic(float x, float a)
{
    if (x < 1.0) {
        return (a + 2.0) * x * x * x - (a + 3.0) * x * x + 1.0;
    }
    else /* x <= 2.0 */ {
        return a * x * x * x - 5.0 * a * x * x + 8.0 * a * x - 4.0 * a;
    }
}

float bc_spline(float x, float b, float c)
{
    if (x < 1.0) {
        return ((12.0 - 9.0 * b - 6.0 * c) * x * x * x + (-18.0 + 12.0 * b + 6.0 * c) * x * x + (6.0 - 2.0 * b)) / 6.0;
    }
    else /* x <= 2.0 */ {
        return ((-b - 6.0 * c) * x * x * x + (6.0 * b + 30.0 * c) * x * x + (-12.0 * b - 48.0 * c) * x + (8.0 * b + 24.0 * c)) / 6.0;
    }
}

float spline16(float x)
{
    if (x < 1.0) {
        return ((x - 9.0 / 5.0) * x - 1.0 / 5.0 ) * x + 1.0;
    }
    else /* x <= 2.0 */ {
        return ((-1.0 / 3.0 * (x - 1.0) + 4.0 / 5.0) * (x - 1.0) - 7.0 / 15.0) * (x - 1.0);
    }
}

float spline36(float x)
{
    if (x < 1.0) {
        return ((13.0 / 11.0 * x - 453.0 / 209.0) * x - 3.0 / 209.0) * x + 1.0;
    }
    else if (x < 2.0) {
        return ((-6.0 / 11.0 * (x - 1.0) + 270.0 / 209.0) * (x - 1.0) - 156.0 / 209.0) * (x - 1.0);
    }
    else /* x <= 3.0 */ {
        return ((1.0 / 11.0 * (x - 2.0) - 45.0 / 209.0) * (x - 2.0) +  26.0 / 209.0) * (x - 2.0);
    }
}

float spline64(float x)
{
    if (x < 1.0) {
        return ((49.0 / 41.0 * x - 6387.0 / 2911.0) * x - 3.0 / 2911.0) * x + 1.0;
    }
    else if (x < 2.0) {
        return ((-24.0 / 41.0 * (x - 1.0) + 4032.0 / 2911.0) * (x - 1.0) - 2328.0 / 2911.0) * (x - 1.0);
    }
    else if (x < 3.0) {
        return ((6.0 / 41.0 * (x - 2.0) - 1008.0 / 2911.0) * (x - 2.0) + 582.0 / 2911.0) * (x - 2.0);
    }
    else /* x <= 4.0 */ {
        return ((-1.0 / 41.0 * (x - 3.0) + 168.0 / 2911.0) * (x - 3.0) - 97.0 / 2911.0) * (x - 3.0);
    }
}

float german(float x)
{
    if (x < 1.0) {
        return 1.0 / 24.0 * x * x * x * x + 185.0 / 144.0 * x * x * x - 335.0 / 144.0 * x * x + 1.0;
    }
    else if ( x < 2.0) {
        return -1.0 / 16.0 * x * x * x * x - 29.0 / 144.0 * x * x * x + 17.0 / 8.0 * x * x - 145.0 / 36.0 * x + 13.0 / 6.0;
    }
    else if ( x < 3.0) {
        return 1.0 / 48.0 * x * x * x * x - 7.0 / 48.0 * x * x * x + 5.0 / 18.0 * x * x + 1.0 / 36.0 * x - 1.0 / 3.0;
    }
    else /* x <= 4.0 */ {
        return 1.0 / 144.0 * x * x * x - 11.0 / 144.0 * x * x + 5.0 / 18.0 * x - 1.0 / 3.0;
    }
}

float magic_kernel_sharp(float x)
{
    if (x < 0.5) {
        return 577.0 / 576.0 - 239.0 / 144.0 * x * x;
    }
    else if (x < 1.5) {
        return 1.0 / 144.0 * (140.0 * x * x - 379.0 * x + 239.0);
    }
    else if (x < 2.5) {
        return -1.0 / 144.0 * (24.0 * x * x - 113.0 * x + 130.0);
    }
    else if (x < 3.5) {
        return 1.0 / 144.0 * (4.0 * x * x - 27.0 * x + 45.0);
    }
    else /* x <= 4.5 */ {
        return -1.0 / 1152.0 * (4.0 * x * x - 36.0 * x + 81.0);
    }
}

//

// Definitions of math functions
//

// Bessel function of the first kind, order one (J1).
// Aproximation optimized for use in shaders.
float bessel_J1(float x)
{
    if (x < 2.293116) {
        return x / 2.0 - x * x * x / 16.0 + x * x * x * x * x / 384.0 - x * x * x * x * x * x * x / 18432.0;
    }
    else {
        return sqrt(M_2_PI / x) * (1.0 + 3.0 / 16.0 / (x * x) - 99.0 / 512.0 / (x * x * x * x)) * cos(x - 3.0 * M_PI_4 + 3.0 / 8.0 / x - 21.0 / 128.0 / (x * x * x));
    }
}

// Modified Bessel function of the first kind, order zero (I0).
// Aproximation optimized for use in shaders.
float bessel_I0(float x)
{
    if (x < 4.970666) {
        return 1.0 + x * x / 4.0 + x * x * x * x / 64.0 + x * x * x * x * x * x / 2304.0 + x * x * x * x * x * x * x * x / 147456.0;
    }
    else {
        return inversesqrt(2.0 * M_PI * (x)) * exp(x);
    }
}

// Spherical bessel function of the first kind, order one (j1).
float bessel_j1(float x)
{
    return sin(x) / (x * x) - cos(x) / x;
}