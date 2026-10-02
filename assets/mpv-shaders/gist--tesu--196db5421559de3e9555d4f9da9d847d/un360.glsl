//!HOOK MAINPRESUB
//!BIND HOOKED
//!DESC un360

#define M_PI 3.1415926535897932384626433832795

const float fov = M_PI/2;   // [0 to M_PI] horizontal field of view, range is exclusive
const float yaw = M_PI*1;   // [any float] polar angle, one full revolution is 2*M_PI
const float pitch = M_PI*0; // [any float] vertical tilt, positive is up
const float roll = M_PI*0;  // [any float] view rotation, positive is clockwise
const bool cubemap = false; // either cubemap or equirectangular

const float t = tan(fov/2);
const float c = cos(pitch);
const float s = sin(pitch);
float r = target_size.y/target_size.x;
const float sR = sin(roll);
const float cR = cos(roll);

mat3 m = mat3(   2*t*cR,   2*sR*t*r,    -t*(cR+sR*r),
              -2*sR*t*c, 2*cR*t*c*r, t*c*(sR-cR*r)-s,
              -2*sR*t*s, 2*cR*t*s*r, t*s*(sR-cR*r)+c);

vec4 hook() {
    vec3 p = vec3(HOOKED_pos, 1.0) * m;

    float theta = atan(p.x, p.z) + yaw;
    float phi = atan(p.y, length(p.xz)) + M_PI/2;

    float x, y;
    if (!cubemap) {
        // equirectangular
        x = fract(theta / (2*M_PI));
        y = phi / M_PI;
    } else {
        // cubemap
        float offset = mod(theta, M_PI/2)-M_PI/4;
        float quadrant = mod(floor(theta*2/M_PI), 4);
        float h = tan(phi)*cos(offset);

        if (abs(h) > 1) { // side
            if (quadrant == 3) { // back
                x = .5/(tan(phi)*cos(offset))+.5;
                y = .5*tan(offset)+.5;
                x *= 1.0/3;
                y *= 1.0/2;

                x += 1.0/3;
                y += 1.0/2;
            } else {
                x = .5*tan(offset)+.5;
                y = -.5/(tan(phi)*cos(offset))+.5;
                x *= 1.0/3;
                y *= 1.0/2;

                x += quadrant/3;
            }
        } else if (h > 0) { // top
            x = .5*tan(M_PI-phi)*cos(theta+M_PI/4)+.5;
            y = -.5*tan(M_PI-phi)*sin(theta+M_PI/4)+.5;
            x *= 1.0/3;
            y *= 1.0/2;

            x += 2.0/3;
            y += 1.0/2;
        } else { // bottom
            x = .5*tan(M_PI-phi)*cos(theta+M_PI/4)+.5;
            y = .5*tan(M_PI-phi)*sin(theta+M_PI/4)+.5;
            x *= 1.0/3;
            y *= 1.0/2;

            y += 1.0/2;
        }
    }
    return HOOKED_tex(vec2(x,y));
}
