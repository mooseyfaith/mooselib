
#include "mathdefs.h"

#if !defined Template_Geometry_Dimension_Count
#  define Template_Geometry_Dimension_Count 3
#endif

#if Template_Geometry_Use_Double_Precision
#  define SUFFIX CHAIN(Template_Geometry_Dimension_Count, d)
#  define REAL   f64
#  define MAT4X3 mat4x3d
#else
#  define SUFFIX CHAIN(Template_Geometry_Dimension_Count, f)
#  define REAL   f32
#  define MAT4X3 mat4x3f
#endif

#define VECTOR CHAIN(vec, SUFFIX)

#define PLANE   CHAIN(Plane, SUFFIX)
#define SPHERE  CHAIN(Sphere, SUFFIX)

// for each point x on the plane: dot(orthogonal, x) - distance_to_origin == 0
struct PLANE {
    VECTOR orthogonal;
    REAL   distance_to_origin; // times orthogonal lengths
};

INTERNAL PLANE make_plane(VECTOR orthogonal, VECTOR point) {
    PLANE result;
    result.orthogonal         = orthogonal;
    result.distance_to_origin = dot(orthogonal, point);
    
    return result;
}

INTERNAL REAL evaluate(PLANE plane, VECTOR point) {
    REAL result = (dot(plane.orthogonal, point) - plane.distance_to_origin);
    return result;
}

INTERNAL REAL distance(PLANE plane, VECTOR point) {
    REAL result = evaluate(plane, point) / length(plane.orthogonal);
    return result;
}

// point is on half space behind plane, e.g. distance from plane to point is <= 0
INTERNAL bool contains(PLANE plane, VECTOR point) {
    return (evaluate(plane, point) <= (REAL)0);
}

INTERNAL VECTOR project(PLANE plane, VECTOR point) {
    return plane.orthogonal * (-evaluate(plane, point) * squared_length(plane.orthogonal));
}

INTERNAL REAL intersection(PLANE plane, VECTOR ray_origin, VECTOR ray_direction) {
    // dot(plane.orthogonal, ray_origin + direction * t) - plane.distance_to_origin == 0
    // dot(plane.orthogonal, ray_origin) - plane.distance_to_origin == t * -dot(plane.orthogonal, direction)
    // t == dot(plane.orthogonal, ray_origin) - plane.distance_to_origin / -dot(plane.orthogonal, direction)
    
    return plane.distance_to_origin - dot(plane.orthogonal, ray_origin) / dot(plane.orthogonal, ray_direction);
}

// Sphere

struct SPHERE {
    VECTOR center;
    REAL radius;
};

// basically distance to sphere border
//  > 0 => point is outside sphere
// == 0 => point is at sphere border
//  < 0 => point is inside sphere
// == -radius => point is at sphere center
INTERNAL REAL distance(SPHERE sphere, VECTOR point) {
    return length(point - sphere.center) - sphere.radius;
}

INTERNAL bool contains(SPHERE sphere, VECTOR point) {
    return (squared_length(point - sphere.center) - sphere.radius * sphere.radius <= (REAL)0);
}

INTERNAL bool intersect(SPHERE a, SPHERE b) {
    return (squared_length(a.center - b.center) - (a.radius + b.radius) * (a.radius + b.radius) <= (REAL)0);
}

INTERNAL bool intersect(PLANE plane, SPHERE sphere) {
    return (distance(plane, sphere.center) <= sphere.radius);
}

// return distance t so that
// sphere moved by direction * t intersects with plane
// 
INTERNAL REAL distance_until_collision(PLANE plane, SPHERE sphere, VECTOR direction, OUTPUT bool *can_collide) {
    // (dot(plane.orthogonal, sphere.center + direction * t) - plane.distance_to_origin) / squared_length(plane.orthogonal) == sphere.radius;
    // dot(plane.orthogonal, sphere.center) + dot(plane.orthogonal, direction * t) - plane.distance_to_origin == sphere.radius * squared_length(plane.orthogonal)
    // dot(plane.orthogonal, sphere.center) + dot(plane.orthogonal, direction * t) == sphere.radius * squared_length(plane.orthogonal) + plane.distance_to_origin
    // dot(plane.orthogonal, direction * t) == sphere.radius * squared_length(plane.orthogonal) + plane.distance_to_origin - dot(plane.orthogonal, sphere.center)
    // t * dot(plane.orthogonal, direction) == sphere.radius * squared_length(plane.orthogonal) + plane.distance_to_origin - dot(plane.orthogonal, sphere.center)
    REAL divident = dot(plane.orthogonal, direction);
    
    *can_collide = true;
    
    if (divident == (REAL)0) {
        // plane and sphere are allready intersecting
        if (intersect(plane, sphere))
            return 0;
        
        *can_collide = false;
        // return 1 to indicate, that a full motion of sphere is possible without collision
        return (REAL)1;
    }
    
    REAL t = ( sphere.radius * squared_length(plane.orthogonal) + plane.distance_to_origin - dot(plane.orthogonal, sphere.center) ) / divident;
    
    return t;
}

INTERNAL s32 movement_distance_until_collision(VECTOR delta_positon, VECTOR movement, REAL radius, OUTPUT REAL *out_distances)
{
    REAL m2 = squared_length(movement);
    assert(m2 != (REAL)0);
    
    REAL dp2 = squared_length(delta_positon);
    REAL r2 = radius * radius;
    REAL minus_p_over_2 = -dot(movement, delta_positon) / m2;
    
    // (p²/4 - q)²
    REAL root_squared = minus_p_over_2 * minus_p_over_2 - (dp2 - r2) / m2;
    
    if (root_squared < (REAL)0) {
        // allready intersecting without movement
        // distance is <= 0
        if (dp2 - r2 <= (REAL)0)
            return -1;
        
        // are not moving close to each other
        return 0;
    }
    
    // both spheres would pass each other only toughing at 1 point in time
    if (root_squared == (REAL)0) {
        out_distances[0] = minus_p_over_2;
        return 1;
    }
    
    REAL root = sqrt(root_squared);
    out_distances[0] = minus_p_over_2 - root;
    out_distances[1] = minus_p_over_2 + root;
    
    return 2;
}

INTERNAL REAL distance_until_collision(SPHERE a, SPHERE b, VECTOR b_direction, OUTPUT bool *can_collide) {
    // |b.center + b_direction * t - a.center|² == (a.radius + b.radius)²
    
    // center_delta = b.center - a.center
    // (b_direction.x * t + center_delta.x)² == b_direction.x² * t² + 2 * b_direction.x * center_delta.x * t + center_delta.x²
    // analog for y and z ...
    
    // |b_direction|² * t² + 2 * dot(b_direction, center_delta) * t + |center_delta|² - (a.radius + b.radius)² == 0
    
    // t²  +  2 * dot(b_direction, center_delta) / |b_direction|² * t  +  (|center_delta|² - (a.radius + b.radius)²) / |b_direction|²  ==  0
    
    // t² + p * t + q == 0
    // -p/2 +- sqrt( p²/4 - q )
    
    VECTOR center_delta = b.center - a.center;
    
    REAL b2 = squared_length(b_direction);
    
    REAL r = a.radius + b.radius;
    REAL q = (squared_length(center_delta) - r * r) / b2;
    REAL p_over_2 = dot(b_direction, center_delta) / b2;
    REAL root_squared = p_over_2 * p_over_2 - q;
    
    *can_collide = true;
    
    if (root_squared < (REAL)0) {
        // allready intersection without motion
        if (intersect(a, b))
            return (REAL)0;
        
        // are not moving close to each other
        *can_collide = false;
        return (REAL)1;
    }
    
    // both spheres would pass each other only toughing at 1 point in time
    if (root_squared ==(REAL)0)
        return (REAL)1;
    
    REAL root = sqrt(root_squared);
    REAL t = -p_over_2 - root;
    
    if (t >= (REAL)0)
        return t;
    
    REAL t2 = -p_over_2 + root;
    
    if (t2 < (REAL)0)
        return 1;
    
    if (-t < t2)
        return t;
    else
        return t2;
}

#undef Template_Geometry_Dimension_Count

#undef SUFFIX
#undef REAL
#undef VECTOR
#undef MAT4X3
#undef PLANE
#undef SPHERE