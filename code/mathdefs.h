#ifndef _ENGINE_MATH_DEFS_H_
#define _ENGINE_MATH_DEFS_H_

#include <cmath>

#include "basic.h"

#define Pi64 3.14159265358979323846
#define Degree_To_Radian64 (Pi64 / 180.0)

#define Pi32 cast_v(f32, Pi64)
#define Degree_To_Radian32 cast_v(f32, Degree_To_Radian64)

#define MAX_ULP_DIFF_DEFAULT 7

// Unit in the last place (ulp) comparision
// numbers should not be close to zero, thats what they said ...
// see: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
inline bool are_close(float a, float b, s32 max_ulp_diff = MAX_ULP_DIFF_DEFAULT) {
    if ((a > 0.0f) != (b > 0.0f))
        return a == b;
    
    return abs(*cast_p(s32, &a) - *cast_p(s32, &b)) <= max_ulp_diff;
}

inline bool are_close(double a, double b, s64 max_ulp_diff = MAX_ULP_DIFF_DEFAULT) {
    if ((a > 0.0) != (b > 0.0))
        return a == b;
    
    return abs(*cast_p(s64, &a) - *cast_p(s64, &b)) <= max_ulp_diff;
}

u32 binary_distance(float a, float b) {
    if ((a > 0.0f) != (b > 0.0f))
        return -1;
    
    return abs(*cast_p(s32, &a) - *cast_p(s32, &b));
}

u64 binary_distance(double a, double b) {
    if ((a > 0.0) != (b > 0.0))
        return -1;
    
    return abs(*cast_p(s64, &a) - *cast_p(s64, &b));
}

inline float sign(float a) {
    if (a < 0.0f)
        return -1.0f;
    else if (a > 0.0f)
        return 1.0f;
    else
        return 0.0f;
}

inline double sign(double a) {
    if (a < 0.0)
        return -1.0;
    else if (a > 0.0)
        return 1.0;
    else
        return 0.0;
}

f32 dirty_mod(f32 value, f32 divisor) {
    s64 x = value / divisor;
    return value - x * divisor;
}

#ifndef _VMATHF_H_
#define _VMATHF_H_
#include "vmath.h"
#endif // _VMATHF_H_

#ifndef _VMATHD_H_
#define _VMATHD_H_
#define VMATH_USE_DOUBLE
#include "vmath.h"
#undef VMATH_USE_DOUBLE
#endif // _VMATHD_H_

typedef vec2f vec2;
typedef vec3f vec3;
typedef vec4f vec4;

typedef mat4f mat4;
typedef mat4x3f mat4x3;

typedef quatf quat;

const quat QUAT_IDENTITY = { 1, 0, 0, 0 };

const vec2 vec2_zero     = { 0, 0 };
const vec2 vec2_one      = { 1, 1 };

const vec3 VEC3_ZERO     = { 0, 0, 0 };
const vec3 VEC3_ONE      = { 1, 1, 1 };

const vec3 VEC3_X_AXIS   = { 1, 0, 0 };
const vec3 VEC3_Y_AXIS   = { 0, 1, 0 };
const vec3 VEC3_Z_AXIS   = { 0, 0, 1 };

const mat4x3 MAT4X3_IDENTITY = make_transform(QUAT_IDENTITY);

#define MAT4_IDENTITY { \
    1, 0, 0, 0, \
    0, 1, 0, 0, \
    0, 0, 1, 0, \
    0, 0, 0, 1 \
}

#endif // !_ENGINE_MATH_DEFS_H_
