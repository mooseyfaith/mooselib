
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  vector_math.h                                                             //
//                                                                            //
//  Tafil Kajtazi                                                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include "basic.h"

#include <cmath>

#define Pi64 3.14159265358979323846
#define Degree_To_Radian64 (Pi64 / 180.0)

#define Pi32 cast_v(f32, Pi64)
#define Degree_To_Radian32 cast_v(f32, Degree_To_Radian64)

#define MAX_ULP_DIFF_DEFAULT 7

// Unit in the last place (ulp) comparision
// numbers should not be close to zero, thats what they said ...
// see: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
INTERNAL bool are_close(f32 a, f32 b, s32 max_ulp_diff = MAX_ULP_DIFF_DEFAULT) {
    if ((a > 0.0f) != (b > 0.0f))
        return a == b;
    
    return abs(*cast_p(s32, &a) - *cast_p(s32, &b)) <= max_ulp_diff;
}

INTERNAL bool are_close(f64 a, f64 b, s64 max_ulp_diff = MAX_ULP_DIFF_DEFAULT) {
    if ((a > 0.0) != (b > 0.0))
        return a == b;
    
    return abs(*cast_p(s64, &a) - *cast_p(s64, &b)) <= max_ulp_diff;
}

INTERNAL u32 binary_distance(f32 a, f32 b) {
    if ((a > 0.0f) != (b > 0.0f))
        return -1;
    
    return abs(*cast_p(s32, &a) - *cast_p(s32, &b));
}

INTERNAL u64 binary_distance(f64 a, f64 b) {
    if ((a > 0.0) != (b > 0.0))
        return -1;
    
    return abs(*cast_p(s64, &a) - *cast_p(s64, &b));
}

INTERNAL f32 sign(f32 a) {
    if (a < 0.0f)
        return -1.0f;
    else if (a > 0.0f)
        return 1.0f;
    else
        return 0.0f;
}

INTERNAL f64 sign(f64 a) {
    if (a < 0.0)
        return -1.0;
    else if (a > 0.0)
        return 1.0;
    else
        return 0.0;
}

INTERNAL f32 dirty_mod(f32 value, f32 divisor) {
    s64 x = value / divisor;
    return value - x * divisor;
}

#define Template_Vector_Math_Data_Type        f32
#define Template_Vector_Math_Vector2_Name     vec2f
#define Template_Vector_Math_Vector3_Name     vec3f
#define Template_Vector_Math_Vector4_Name     vec4f
#define Template_Vector_Math_Matrix4x3_Name   mat4x3f
#define Template_Vector_Math_Matrix4_Name     mat4f
#define Template_Vector_Math_Quaternion_Name  quatf
#include "template_vector_math.h"

#define Template_Vector_Math_Data_Type        f64
#define Template_Vector_Math_Vector2_Name     vec2d
#define Template_Vector_Math_Vector3_Name     vec3d
#define Template_Vector_Math_Vector4_Name     vec4d
#define Template_Vector_Math_Matrix4x3_Name   mat4x3d
#define Template_Vector_Math_Matrix4_Name     mat4d
#define Template_Vector_Math_Quaternion_Name  quatd
#include "template_vector_math.h"

typedef vec2f vec2;
typedef vec3f vec3;
typedef vec4f vec4;

typedef mat4f mat4;
typedef mat4x3f mat4x3;

typedef quatf quat;

const quat QUAT_IDENTITY = { 1, 0, 0, 0 };

const vec2 vec2_zero     = { 0, 0 };
const vec2 vec2_one      = { 1, 1 };

const vec3 Vec3_Zero     = { 0, 0, 0 };
const vec3 Vec3_One      = { 1, 1, 1 };

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

struct area2f {
    union {
        struct {
            vec2f min, size;
        };
        
        struct {
            f32 x, y;
            f32 width, height;
        };
    };
    
    bool is_valid;
};

#define Template_Area_Name        area2f
#define Template_Area_Vector_Type vec2f
#define Template_Area_Data_Type   f32
#define Template_Area_Struct_Is_Declared
#include "template_area.h"

#endif // VECTOR_MATH_H
