
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  template_vector_math.h                                                    //
//                                                                            //
//  Tafil Kajtazi                                                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define REAL   Template_Vector_Math_Data_Type
#define VEC2   Template_Vector_Math_Vector2_Name
#define VEC3   Template_Vector_Math_Vector3_Name
#define VEC4   Template_Vector_Math_Vector4_Name
#define MAT4X3 Template_Vector_Math_Matrix4x3_Name
#define MAT4   Template_Vector_Math_Matrix4_Name
#define QUAT   Template_Vector_Math_Quaternion_Name

#define Template_Vector_Name      VEC2
#define Template_Vector_Data_Type REAL
#define Template_Vector_Dimension_Count 2
#define Template_Vector_Union struct { REAL x, y; }; struct { REAL width, height; };
#include "template_vector.h"

#define Template_Vector_Name      VEC3
#define Template_Vector_Data_Type REAL
#define Template_Vector_Dimension_Count 3
#define Template_Vector_Union struct { REAL x, y, z; }; struct { REAL r, g, b; }; struct { REAL red, green, blue; };
#include "template_vector.h"

#define Template_Vector_Name      VEC4
#define Template_Vector_Data_Type REAL
#define Template_Vector_Dimension_Count 4
#define Template_Vector_Union struct { REAL x, y, z, w; }; struct { REAL r, g, b, a; }; struct { REAL red, green, blue, alpha; };
#include "template_vector.h"

#define Template_Vector_Name      QUAT
#define Template_Vector_Data_Type REAL
#define Template_Vector_Dimension_Count 4
#define Template_Vector_Union struct { REAL w, x, y, z; };
#include "template_vector.h"

#if 0

#define VEC_LENGTH 2
#define VEC_NAME VEC2
#define VEC_VALUE_UNIONS struct { REAL x, y; }; struct { REAL u, v; }; struct { REAL width, height; };
#include "vector.h"

#define VEC_LENGTH 3
#define VEC_NAME VEC3
#define VEC_VALUE_UNIONS struct { REAL x, y, z; }; struct { REAL r, g, b; }; struct { REAL red, green, blue; };
#include "vector.h"

#define VEC_LENGTH 4
#define VEC_NAME VEC4
#define VEC_VALUE_UNIONS struct { REAL x, y, z, w; }; struct { REAL r, g, b, a; }; struct { REAL red, green, blue, alpha; };
#include "vector.h"

#define VEC_LENGTH 4
#define VEC_NAME QUAT
#define VEC_VALUE_UNIONS struct { REAL w, x, y, z; };
#include "vector.h"

#endif

INTERNAL VEC2 make_vec2(REAL x, REAL y) {
    return VEC2{ x, y };
}

INTERNAL VEC2 make_vec2(VEC3 vec, u32 x_index = 0, u32 y_index = 1) {
    return VEC2{ vec.values[x_index], vec.values[y_index] };
}

// devides by w
INTERNAL VEC2 make_vec2(VEC4 vec, u32 x_index = 0, u32 y_index = 1, u32 w_index = 3) {
    return VEC2{ vec.values[x_index], vec.values[y_index] } / vec.values[w_index];
}

// does not devide by w
INTERNAL VEC2 make_vec2_cut(VEC4 vec, u32 x_index = 0, u32 y_index = 1) {
    return VEC2{ vec.values[x_index], vec.values[y_index] };
}

INTERNAL VEC3 make_vec3(REAL x, REAL y, REAL z) {
    return VEC3{ x, y, z };
}

INTERNAL VEC3 make_vec3(const VEC2& vec, REAL z = (REAL)0) {
    return VEC3{ vec.values[0], vec.values[1], z };
}

INTERNAL VEC3 make_vec3_swizzle(VEC2 vec, u32 x_index, u32 y_index, REAL z = (REAL)0, REAL x_sign = (REAL)1, REAL y_sign = (REAL)1)
{
    assert(x_index < 3);
    assert(y_index < 3);
    assert(x_index != y_index);
    
    VEC3 result;
    result[x_index] = vec.x * x_sign;
    result[y_index] = vec.y * y_sign;
    result[3 - x_index - y_index] = z; // since 0 + 1 + 2 == 3, and x_index != y_index => z_index = 3 - x_index - y_index
    
    return result;
}

// devides by w
INTERNAL VEC3 make_vec3(const VEC4& vec) {
    return VEC3{ vec.values[0], vec.values[1], vec.values[2] } / vec.values[3];
}

// does not devide by w
INTERNAL VEC3 make_vec3_cut(const VEC4& vec) {
    return VEC3{ vec.values[0], vec.values[1], vec.values[2] };
}

INTERNAL VEC3 make_vec3_scale(REAL scale) {
    return VEC3{ scale, scale, scale };
}

INTERNAL VEC4 make_vec4(REAL x, REAL y, REAL z, REAL w = (REAL)1) {
    return VEC4{ x, y, z, w };
}

INTERNAL VEC4 make_vec4(const VEC2& vec, REAL z = (REAL)0, REAL w = (REAL)1) {
    return VEC4{ vec.values[0],  vec.values[1], z, w };
}

INTERNAL VEC4 make_vec4(const VEC3& vec, REAL w = (REAL)1) {
    return VEC4{ vec.values[0], vec.values[1], vec.values[2], w };
}

INTERNAL VEC4 make_vec4_scale(REAL scale, REAL w = (REAL)1) {
    return VEC4{ scale, scale, scale, w };
}


/*
struct QUAT {
 REAL w, x, y, z;
 
 INTERNAL REAL & operator[](u32 i) {
  assert(i < 4);
  return *(&w + i);
 }
 
 INTERNAL REAL operator[](u32 i) const {
  assert(i < 4);
  return *(&w + i);
 }
 
 INTERNAL QUAT & operator=(const QUAT &other) {
  w = other.w;
  x = other.x;
  y = other.y;
  z = other.z;
  return *this;
 }
 
 INTERNAL QUAT & operator=(const REAL other[]) {
  w = other[0];
  x = other[1];
  y = other[2];
  z = other[3];
  return *this;
 }
 
 INTERNAL operator REAL *() { return &w; }
 INTERNAL operator const REAL *() const { return &w; }
};
*/

QUAT make_quat(VEC3 normalized_rotation_axis, REAL radians)
{
    //assert(are_close(length(normalized_rotation_axis), (REAL)1));
    
    radians *= (REAL)0.5;
    REAL s = (REAL)sin(radians);
    
    return QUAT {
        (REAL)cos(radians),
        normalized_rotation_axis[0] * s,
        normalized_rotation_axis[1] * s,
        normalized_rotation_axis[2] * s,
    };
}

INTERNAL QUAT negative(QUAT a) {
    return QUAT{ a.w, -a.x, -a.y, -a.z };
}

INTERNAL QUAT multiply(QUAT a, QUAT b) {
    return QUAT {
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
    };
}

INTERNAL QUAT inverse(const QUAT &a) {
    REAL inv_length2 = (REAL)-1 / squared_length(a);
    return QUAT{ a.w * inv_length2, a.x * inv_length2, a.y * inv_length2, a.z * inv_length2 };
}

struct MAT4X3 {
    union {
        struct {
            VEC3 right;
            VEC3 up;
            VEC3 forward;
            VEC3 translation;
        };
        VEC3 columns[4];
    };
    
    inline REAL & operator[](u32 i) {
        assert(i < 12);
        return columns[i / 3].values[i % 3];
    }
    
    inline REAL operator[](u32 i) const {
        assert(i < 12);
        return columns[i / 3].values[i % 3];
    }
    
    inline operator REAL *() { return columns[0].values; }
    inline operator const REAL *() const { return columns[0].values; }
};

INTERNAL void set_identity(MAT4X3 *matrix) {
    *matrix = {};
    
    for (u32 col = 0; col != 3; ++col)
        matrix->columns[col].values[col] = (REAL)1;
}

INTERNAL void update_rotation_scale(MAT4X3 *matrix, QUAT rotation, VEC3 scale = VEC3{ (REAL)1, (REAL)1, (REAL)1 }) {
    REAL xx = rotation.x * rotation.x;
    REAL xy = rotation.x * rotation.y;
    REAL xz = rotation.x * rotation.z;
    
    REAL yy = rotation.y * rotation.y;
    REAL yz = rotation.y * rotation.z;
    
    REAL zz = rotation.z * rotation.z;
    
    REAL wx = rotation.w * rotation.x;
    REAL wy = rotation.w * rotation.y;
    REAL wz = rotation.w * rotation.z;
    
    matrix->columns[0].values[0] = ((REAL)1 - (REAL)2 * (yy + zz))	* scale.values[0];
    matrix->columns[1].values[0] = (REAL)2 * (xy - wz)				* scale.values[1];
    matrix->columns[2].values[0] = (REAL)2 * (xz + wy)				* scale.values[2];
    
    matrix->columns[0].values[1] = (REAL)2 * (xy + wz)				* scale.values[0];
    matrix->columns[1].values[1] = ((REAL)1 - (REAL)2 * (xx + zz))	* scale.values[1];
    matrix->columns[2].values[1] = (REAL)2 * (yz - wx)				* scale.values[2];
    
    matrix->columns[0].values[2] = (REAL)2 * (xz - wy)				* scale.values[0];
    matrix->columns[1].values[2] = (REAL)2 * (yz + wx)				* scale.values[1];
    matrix->columns[2].values[2] = ((REAL)1 - (REAL)2 * (xx + yy))	* scale.values[2];
}

INTERNAL VEC4 transform(MAT4X3 matrix, VEC4 v) {
    VEC3 result = matrix.columns[0] * v.values[0];
    
    for (u32 col = 1; col != 4; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return VEC4{ result.values[0], result.values[1], result.values[2], v.values[3] };
}

INTERNAL VEC4 operator*(MAT4X3 matrix, VEC4 v) {
    return transform(matrix, v);
}

INTERNAL VEC4 transform(MAT4X3 matrix, VEC3 v, REAL w) {
    VEC3 result = matrix.translation * w;
    
    for (u32 col = 0; col != 3; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return VEC4{ result.values[0], result.values[1], result.values[2], w };
}

// like transform with w == 1
INTERNAL VEC3 transform_point(MAT4X3 matrix, VEC3 v) {
    VEC3 result = matrix.translation;
    
    for (u32 col = 0; col != 3; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return result;
}

// like transform_point (w == 1)
INTERNAL VEC3 operator*(MAT4X3 matrix, VEC3 v) {
    return transform_point(matrix, v);
}

// like transform with w == 0
INTERNAL VEC3 transform_direction(MAT4X3 matrix, VEC3 v) {
    VEC3 result = matrix.columns[0] * v.values[0];
    
    for (u32 col = 1; col != 3; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return result;
}

INTERNAL MAT4X3 make_transform(QUAT rotation, VEC3 translation = VEC3{ (REAL)0, (REAL)0, (REAL)0 }, VEC3 scale = VEC3{ (REAL)1, (REAL)1, (REAL)1 }) {
    MAT4X3 result;
    update_rotation_scale(&result, rotation, scale);
    result.translation = translation;
    return result;
}

/* Todo: figure it out
INTERNAL MAT4X3 make_inverse_transform(const QUAT &rotation, const VEC3 &translation, const VEC3 &scale = VEC3{ (REAL)1, (REAL)1, (REAL)1 }) {
MAT4X3 result;
// not correct for scale, needs 2 scale as input?
update_rotation_scale(result, -rotation, VEC3{ (REAL)1 / scale.values[0], (REAL)1 / scale.values[1], (REAL)1 / scale.values[2] }); 
result.columns[3] = transform_direction(result, -translation);
return result;
}
*/

INTERNAL MAT4X3 make_inverse_unscaled_transform(const MAT4X3 &transform) {
    MAT4X3 result;
    
    // inverse of rotation
    for (u32 x = 0; x < 3; ++x) {
        for (u32 y = 0; y < 3; ++y)
            result.columns[x].values[y] = transform.columns[y].values[x];
        
        result.columns[x] = normalize(result.columns[x]);
    }
    
    result.translation = transform_direction(result, -transform.translation);
    
    return result;
}

// transfrom from local space to world space
// can be used to place object at camera pos
// inverse of camera view
INTERNAL MAT4X3 make_look_at(VEC3 eye, VEC3 target, VEC3 up) {
    MAT4X3 result;
    VEC3 forward = normalize(eye - target);
    VEC3 right = normalize(cross(up, forward));
    up = cross(forward, right);
    
    result.right       = right;
    result.up          = up;
    result.forward     = forward;
    result.translation = eye;
    
    return result;
}

// transform from world space to local space (e.g. camera space)
// camera view
INTERNAL MAT4X3 make_inverse_look_at(VEC3 eye, VEC3 target, VEC3 up) {
    MAT4X3 result;
    VEC3 forward = normalize(eye - target);
    VEC3 right = normalize(cross(up, forward));
    up = cross(forward, right);
    
    result.columns[0].values[0] = right.values[0];
    result.columns[1].values[0] = right.values[1];
    result.columns[2].values[0] = right.values[2];
    
    result.columns[0].values[1] = up.values[0];
    result.columns[1].values[1] = up.values[1];
    result.columns[2].values[1] = up.values[2];
    
    result.columns[0].values[2] = forward.values[0];
    result.columns[1].values[2] = forward.values[1];
    result.columns[2].values[2] = forward.values[2];
    
    result.translation = transform_direction(result, -eye);
    return result;
}

// Matrix4x4

struct MAT4 {
    VEC4 columns[4];
    
    inline REAL & operator[](u32 i) {
        assert(i < 16);
        return columns[i / 4].values[i % 4];
    }
    
    inline REAL operator[](u32 i) const {
        assert(i < 16);
        return columns[i / 4].values[i % 4];
    }
    
    inline operator REAL *() { return columns[0].values; }
    inline operator const REAL *() const { return columns[0].values; }
};

INTERNAL void set_identity(MAT4 *matrix) {
    for (u32 col = 0; col != 4; ++col)
        set_axis(matrix->columns + col, col);
}

INTERNAL VEC4 transform(MAT4 matrix, VEC4 v) {
    VEC4 result = matrix.columns[0] * v.values[0];
    
    for (u32 col = 1; col != 4; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return result;
}

INTERNAL VEC4 operator*(MAT4 matrix, VEC4 v) {
    return transform(matrix, v);
}

INTERNAL VEC4 transform(MAT4 matrix, VEC3 v, REAL w) {
    VEC4 result = matrix.columns[3] * w;
    
    for (u32 col = 0; col != 3; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return result;
}

// like transform with w == 1
INTERNAL VEC4 transform_point(MAT4 matrix, VEC3 v) {
    VEC4 result = matrix.columns[3];
    
    for (u32 col = 0; col != 3; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return result;
}

// like transform_point (w == 1)
INTERNAL VEC4 operator*(MAT4 matrix, VEC3 v) {
    return transform_point(matrix, v);
}

// like transform with w == 0
INTERNAL VEC4 transform_direction(MAT4 matrix, VEC3 v) {
    VEC4 result = matrix.columns[0] * v.values[0];
    
    for (u32 col = 1; col != 3; ++col)
        result += matrix.columns[col] * v.values[col];
    
    return result;
}

/*
projection matrix
a 0 0 0
0 b 0 0
0 0 c d
0 0 e 0
*/
INTERNAL MAT4 make_perspective_projection(REAL a, REAL b, REAL c, REAL d, REAL e) {
    MAT4 result = {};
    
    result.columns[0].values[0] = a;
    result.columns[1].values[1] = b;
    result.columns[2].values[2] = c;
    result.columns[3].values[2] = d;
    result.columns[2].values[3] = e;
    
    return result;
}

/*
inverse projection matrix
1 / a |     0 |     0 |            0
0     | 1 / b |     0 |            0
0     |     0 |     0 |        1 / e
0     |     0 | 1 / d | -c / (d * e)
*/
INTERNAL MAT4 make_inverse_perspective_projection(REAL a, REAL b, REAL c, REAL d, REAL e) {
    MAT4 result = {};
    
    result.columns[0].values[0] = (REAL)1 / a;
    result.columns[1].values[1] = (REAL)1 / b;
    result.columns[2].values[3] = (REAL)1 / d;
    result.columns[3].values[2] = (REAL)1 / e;
    result.columns[3].values[3] = -c / (d * e);
    
    return result;
}

INTERNAL MAT4 make_inverse_perspective_projection(MAT4 projection) {
    return make_inverse_perspective_projection(projection.columns[0].values[0], projection.columns[1].values[1], projection.columns[2].values[2], projection.columns[3].values[2], projection.columns[2].values[3]);
}

INTERNAL MAT4 make_perspective_projection(REAL width, REAL height, REAL near_ = (REAL)0.01, REAL far_ = (REAL)1000.0)
{
    return make_perspective_projection((REAL)2 * near_ / width, (REAL)2 * near_ / height, -(far_ + near_) / (far_ - near_), (REAL)-2 * far_ * near_ / (far_ - near_), (REAL)-1);
}

INTERNAL MAT4 make_perspective_fov_projection(REAL fov_y, REAL width_over_height, REAL near_ =(REAL)0.01, REAL far_ = (REAL)1000.0) {
    REAL height = 2 * near_ * (REAL)tan(fov_y * (REAL)0.5);
    REAL width = width_over_height * height;
    return make_perspective_projection(width, height, near_, far_);
}

INTERNAL MAT4 make_orthographic_projection(REAL width, REAL height, REAL near_ = (REAL)0.01, REAL far_ = (REAL)1000.0) {
    MAT4 result = {};
    
    result.columns[0].values[0] = (REAL)2 / width;
    result.columns[1].values[1] = (REAL)2 / height;
    result.columns[2].values[2] = (REAL)2 / (near_ - far_);
    result.columns[3].values[2] = (near_ + far_) / (near_ - far_);
    result.columns[3].values[3] = (REAL)1;
    
    return result;
}

INTERNAL MAT4 make_inverse_orthographic_projection(MAT4 orthographic_projection) {
    MAT4 result = {};
    REAL n_minus_f = (REAL)2 / orthographic_projection.columns[2][2];
    REAL n_plus_f = n_minus_f * orthographic_projection.columns[3][2];
    
    result.columns[0][0] = n_plus_f / orthographic_projection.columns[0][0];
    result.columns[1][1] = n_plus_f / orthographic_projection.columns[1][1];
    result.columns[1][2] = (REAL)-2 / orthographic_projection.columns[1][1];
    result.columns[2][2] = n_minus_f;
    result.columns[3][3] = n_plus_f;
    
    return result;
}

// Matrix multiplications

INTERNAL MAT4X3 mul(MAT4X3 second, MAT4X3 first) {
    MAT4X3 result;
    
#if 0
    const REAL first_row3[] = { (REAL)0, (REAL)0, (REAL)0, (REAL)1 };
    
    for (u32 row = 0; row != 3; ++row)
        for (u32 col = 0; col != 4; ++col) {
        result.columns[col].values[row] = second.columns[3].values[row] * first_row3[col];
        
        for (u32 i = 0; i != 3; ++i)
            result.columns[col].values[row] += second.columns[i].values[row] * first.columns[col].values[i];
    }
#else
    for (u32 col = 0; col != 3; ++col)
        result.columns[col] = transform_direction(second, first.columns[col]);
    
    result.columns[3] = transform_point(second, first.columns[3]);
#endif
    return result;
}

INTERNAL MAT4 mul(MAT4 second, MAT4X3 first) {
    MAT4 result;
    
    for (u32 col = 0; col != 3; ++col)
        result.columns[col] = transform_direction(second, first.columns[col]);
    
    result.columns[3] = transform_point(second, first.columns[3]);
    return result;
}

INTERNAL MAT4 mul(MAT4X3 second, MAT4 first) {
    MAT4 result;
    
    for (u32 col = 0; col != 4; ++col)
        result.columns[col] = transform(second, first.columns[col]);
    
    return result;
}

INTERNAL MAT4 mul(MAT4 second, MAT4 first) {
    MAT4 result;
    
    for (u32 col = 0; col != 4; ++col)
        result.columns[col] = transform(second, first.columns[col]);
    
    return result;
}

INTERNAL MAT4X3 operator*(MAT4X3 second, MAT4X3 first) {
    return mul(second, first);
}

INTERNAL MAT4 operator*(MAT4 second, MAT4X3 first) {
    return mul(second, first);
}

INTERNAL MAT4 operator*(MAT4X3 second, MAT4 first) {
    return mul(second, first);
}

INTERNAL MAT4 operator*(MAT4 second, MAT4 first) {
    return mul(second, first);
}

INTERNAL VEC3 get_world_to_clip_point(MAT4 camera_to_clip_projection, MAT4X3 world_to_camera_transform, VEC3 world_point) {
    return make_vec3(camera_to_clip_projection * (world_to_camera_transform * world_point));
}

INTERNAL VEC3 get_clip_to_world_point(MAT4X3 camera_to_world_transform, MAT4 clip_to_camera_projection, VEC3 clip_point) {
    return transform_point(camera_to_world_transform, make_vec3(transform_point(clip_to_camera_projection, clip_point)));
}

INTERNAL REAL get_clip_plane_z(MAT4 camera_to_clip_projection, MAT4X3 world_to_camera_transform, VEC3 world_plane_point) {
    VEC4 clip_plane_point = camera_to_clip_projection * (world_to_camera_transform * world_plane_point);
    return clip_plane_point.values[2] / clip_plane_point.values[3];
}

INTERNAL VEC3 get_clip_to_world_direction(MAT4X3 camera_to_world_transform, MAT4 clip_to_camera_projection, VEC3 direction) {
    VEC3 clip_center_to_world = get_clip_to_world_point(camera_to_world_transform, clip_to_camera_projection, VEC3{});
    VEC3 clip_end_point_to_world = get_clip_to_world_point(camera_to_world_transform, clip_to_camera_projection, direction);
    return clip_end_point_to_world - clip_center_to_world;
}

INTERNAL REAL get_clip_to_world_up_scale(MAT4X3 camera_to_world_transform, MAT4 clip_to_camera_projection, REAL clip_z) {
    VEC3 clip_center_to_world = get_clip_to_world_point(camera_to_world_transform, clip_to_camera_projection, { cast_v(REAL, 0.0), cast_v(REAL, 0.0), clip_z });
    VEC3 clip_up_to_world = get_clip_to_world_point(camera_to_world_transform, clip_to_camera_projection, { cast_v(REAL, 0.0), cast_v(REAL, 1.0), clip_z });
    return length(clip_up_to_world - clip_center_to_world);
}

#undef REAL
#undef MAT4X3
#undef MAT4
#undef VEC2
#undef VEC3
#undef VEC4
#undef QUAT

#undef Template_Vector_Math_Data_Type
#undef Template_Vector_Math_Vector2_Name
#undef Template_Vector_Math_Vector3_Name
#undef Template_Vector_Math_Vector4_Name
#undef Template_Vector_Math_Matrix4x3_Name
#undef Template_Vector_Math_Matrix4_Name
#undef Template_Vector_Math_Quaternion_Name