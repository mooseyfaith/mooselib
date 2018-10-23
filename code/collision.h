#if !defined ENGINE_COLLISION_H
#define ENGINE_COLLISION_H

#include "mathdefs.h"

#define VECTOR vec3f

#if defined DEBUG_COLLISION

#include "immediate_render.h"

struct Debug_Draw_Info {
    Immediate_Render_Context *immediate_render_context;
    vec3f camera_forward_direction;
    u32 current_iteration_count;
    u32 max_iteration_count;
    u32 level;
    u32 disabled_at_level;
    mat4x3f to_world_transform;
} global_debug_draw_info;

#define DEBUG_DRAW_PUSH_LEVEL() \
++global_debug_draw_info.level; \
defer { \
    DEBUG_DRAW_ENABLE(); \
    --global_debug_draw_info.level; \
};

inline void DEBUG_DRAW_INIT(Immediate_Render_Context *immediate_render_context, u32 max_iteration_count = 0) {
    global_debug_draw_info.immediate_render_context = immediate_render_context;
    global_debug_draw_info.max_iteration_count = max_iteration_count;
    global_debug_draw_info.to_world_transform = MAT4X3_IDENTITY;
}

inline void DEBUG_DRAW_ENABLE() {
    if (global_debug_draw_info.disabled_at_level == global_debug_draw_info.level)
        global_debug_draw_info.disabled_at_level = 0;
}

inline void DEBUG_DRAW_DISABLE() {
    if (!global_debug_draw_info.disabled_at_level)
        global_debug_draw_info.disabled_at_level = global_debug_draw_info.level;
}

inline mat4x3f DEBUG_DRAW_SET_TO_WORLD_MATRIX(mat4x3f to_world_transform) {
    mat4x3f old = global_debug_draw_info.to_world_transform;
    global_debug_draw_info.to_world_transform = to_world_transform;
    return old;
}

#define DEBUG_DRAW_PUSH_TO_WORLD_MATRIX(to_world_transform) \
mat4x3f _debug_to_world_transform_ ## __LINE__ = DEBUG_DRAW_SET_TO_WORLD_MATRIX(to_world_transform); defer { DEBUG_DRAW_SET_TO_WORLD_MATRIX(_debug_to_world_transform_ ## __LINE__); };

inline void DEBUG_DRAW_LINE(VECTOR a, VECTOR b, rgba32 color) {
    if (!global_debug_draw_info.disabled_at_level || (global_debug_draw_info.disabled_at_level > global_debug_draw_info.level))
        draw_line(global_debug_draw_info.immediate_render_context, transform_point(global_debug_draw_info.to_world_transform, a), transform_point(global_debug_draw_info.to_world_transform, b), color);
}

inline void DEBUG_DRAW_CIRCLE(VECTOR center, float radius, rgba32 color) {
    if (!global_debug_draw_info.disabled_at_level || (global_debug_draw_info.disabled_at_level > global_debug_draw_info.level))
        draw_circle(global_debug_draw_info.immediate_render_context, transform_point(global_debug_draw_info.to_world_transform, center), global_debug_draw_info.camera_forward_direction, radius, color);
}

inline Render_Batch_Checkpoint DEBUG_DRAW_CHECKPOINT() {
    return get_checkpoint(&global_debug_draw_info.immediate_render_context->render_batch);
}

inline void DEBUG_DRAW_REVERT(Render_Batch_Checkpoint checkpoint) {
    revert_to_checkpoint(&global_debug_draw_info.immediate_render_context->render_batch, &checkpoint);
}

#else

#define DEBUG_DRAW_INIT(imc, ...)
#define DEBUG_DRAW_LINE(a, b, color)
#define DEBUG_DRAW_CIRCLE(center, radius, color)
#define DEBUG_DRAW_CHECKPOINT() Render_Batch_Checkpoint{}
#define DEBUG_DRAW_REVERT(checkpoint)

#endif


#define SUPPORT_DEC(name) VECTOR name(any object, VECTOR direction);

typedef SUPPORT_DEC((*Support_Funtion));

struct Weighted_Support_Info {
    any *objects;
    Support_Funtion *supports;
    float *weights;
    u32 count;
};

VECTOR weighted_support(any support_info_pointer, VECTOR direction) {
    Weighted_Support_Info *info = CAST_P(Weighted_Support_Info, support_info_pointer);
    VECTOR result = VECTOR{};
    
    for (u32 i = 0; i < info->count; ++i)
        result += info->supports[i](info->objects[i], direction * info->weights[i]) * info->weights[i];
    
    return result;
}

VECTOR sweep_support(any sweep_dir_ptr, VECTOR direction) {
    VECTOR *sweep_dir = CAST_P(VECTOR, sweep_dir_ptr);
    float d = dot(direction, *sweep_dir);
    
    if (d > 0.0f)
        return *sweep_dir;
    else if (d < 0.0f)
        return VECTOR{};
    else
        return *sweep_dir * 0.5f;
}

struct LINE {
    VECTOR from, to;
};

VECTOR line_support(any line_ptr, VECTOR direction) {
    LINE *line = CAST_P(LINE, line_ptr);
    float d = dot(direction, line->to - line->from);
    
    if (d > 0.0f)
        return line->to;
    else if (d < 0.0f)
        return line->from;
    else
        return line->from * 0.5f + line->to * 0.5f;
}

struct SPHERE {
    VECTOR center;
    float radius;
};

VECTOR sphere_support(any sphere_pointer, VECTOR direction) {
    SPHERE *sphere = CAST_P(SPHERE, sphere_pointer);
    return sphere->center + normalize(direction) * sphere->radius;
}

struct BOX {
    VECTOR bottom_left;
    VECTOR dimensions;
    mat4x3f *transform;
    mat4x3f *inverse_transform;
};

VECTOR box_support(void *box_pointer, VECTOR direction) {
    BOX *box = CAST_P(BOX, box_pointer);
    
    if (box->inverse_transform) {
        assert(box->transform);
        direction = transform_direction(*box->inverse_transform, direction);
    }
    
    VECTOR half_dimensions = box->dimensions * 0.5f;
    VECTOR center = box->bottom_left + half_dimensions;
    
    VECTOR point;
    for (u32 i = 0; i < ARRAY_COUNT(point.values); ++i)
        point[i] = center[i] + (direction[i] > 0.0f ? half_dimensions[i] : (direction[i] < 0.0f ? -half_dimensions[i] : 0.0f));
    
    if (box->inverse_transform) {
        assert(box->transform);
        return transform_point(*box->transform, point);
    }
    
    return point;
}

inline bool gjk_simplex_edge_case(u32 *corner_count, VECTOR *direction, VECTOR a, VECTOR *b, VECTOR ab, VECTOR ao) {
    if (dot(ab, ao) > 0.0f) { // edge points towards origin
        // ab x ao x ab => vector perpendicular to ab and pointing to origin
        *direction = cross(cross(ab, ao), ab);
        
        //{
        //rgba32 color = { 200, 0, 200, 255 };
        //DEBUG_DRAW_LINE((a + *b) * 0.5f, (a + *b) * 0.5f + normalize(*direction), color);
        //}
        
        // origin is on edge
        if (squared_length(*direction) == 0.0f)
            return true;
    }
    else {
        *b = a;
        *corner_count = 1;
        *direction = ao;
    }
    
    return false;
}

inline bool gjk_simplex(VECTOR corners[4], u32 *corner_count, VECTOR *direction) {
    switch (*corner_count) {
        case 2: {
            VECTOR *b = corners + 0;
            VECTOR *a = corners + 1;
            
            return gjk_simplex_edge_case(corner_count, direction, *a, b, *b - *a, -(*a));
        }
        case 3: {
            VECTOR *c = corners + 0;
            VECTOR *b = corners + 1;
            VECTOR *a = corners + 2;
            
            VECTOR ab = *b - *a;
            VECTOR ac = *c - *a;
            VECTOR ao = -(*a);
            
            VECTOR face_normal = cross(ab, ac); // order is important!
            VECTOR edge_normal = cross(ab, face_normal); // order is important!
            
            if (dot(edge_normal, ao) > 0.0f) { // edge normal of ab points to origin
                if (dot(ab, ao) > 0.0f) { // edge ab is closest to origin
                    *c = *a; // remove c from corners
                    *corner_count = 2;
                    
                    // ab x ao x ab => vector perpendicular to ab and pointing to origin
                    *direction = cross(cross(ab, ao), ab);
                }
                else {
                    *b = *a;
                    *corner_count = 2;
                    return gjk_simplex_edge_case(corner_count, direction, *a, c, ac, ao);
                }
            }
            else {
                edge_normal = cross(face_normal, ac); // order important!
                
                if (dot(edge_normal, ao) > 0.0f) { // edge normal of ac points to origin
                    *b = *a;
                    *corner_count = 2;
                    return gjk_simplex_edge_case(corner_count, direction, *a, c, ac, ao);
                }
                else {
                    // origin is below triangle, otherwise above 
                    float d = dot(face_normal, *a);
                    
                    if (d > 0.0f) {
                        *direction = -face_normal; // origin is closest to triangle plane
                        // swap corners, but should be irrelevant ...
                        VECTOR t = *b;
                        *b = *c;
                        *c = t;
                    }
                    else if (d < 0.0f) {
                        *direction = face_normal;
                    } else {
                        return true;
                    }
                }
            }
            
            { 
                rgba32 color = { 255, 255, 255, 255 };
                DEBUG_DRAW_LINE(*a, vec3f{}, color);
            }
            {
                rgba32 color = rgba32{ 0, 200, 200, 255 };
                DEBUG_DRAW_LINE((*a + *b + *c) / 3.0f, (*a + *b + *c) / 3.0f + normalize(face_normal), color);
            }
            {
                rgba32 color = rgba32{ 200, 0, 200, 255 };
                DEBUG_DRAW_LINE((*a + *b + *c) / 3.0f, (*a + *b + *c) / 3.0f + normalize(edge_normal), color);
            }
            
        } break;
        
        case 4: {
            VECTOR *d = corners + 0;
            VECTOR *c = corners + 1;
            VECTOR *b = corners + 2;
            VECTOR *a = corners + 3;
            
            VECTOR ab = *b - *a;
            VECTOR ac = *c - *a;
            VECTOR ao = -(*a);
            
            VECTOR bcn = cross(ab, ac);
            if (dot(bcn, ao) > 0.0f) {
                *d = *b;
                *b = *a;
                *corner_count = 3;
                *direction = bcn;
                return gjk_simplex(corners, corner_count, direction);
            }
            else {
                VECTOR ad = *d - *a;
                VECTOR cdn = cross(ac, ad);
                if (dot(cdn, ao) > 0.0f) {
                    *b = *a;
                    *corner_count = 3;
                    *direction = cdn;
                    return gjk_simplex(corners, corner_count, direction);
                }
                else {
                    VECTOR dbn = cross(ad, ab);
                    if (dot(dbn, ao) > 0.0f) {
                        *c = *b;
                        *b = *a;
                        *corner_count = 3;
                        *direction = dbn;
                        return gjk_simplex(corners, corner_count, direction);
                    } else {
                        return true;
                    }
                }
            }
        } break;
        
        default:
        UNREACHABLE_CODE;
        break;
    }
    
    return false;
}

bool is_intersecting(any object, Support_Funtion support) {
    VECTOR corners[4];
    
    Render_Batch_Checkpoint checkpoint = DEBUG_DRAW_CHECKPOINT();
    
    corners[0] = support(object, VECTOR{1.0f});
    VECTOR direction = -corners[0];
    u32 corner_count = 1;
    
#if defined DEBUG_COLLISION
    global_debug_draw_info.current_iteration_count = 0;
#endif
    
    LOOP {
        VECTOR a = support(object, direction);
        
        if (dot(a, direction) < 0.0f)
            return false;
        
        corners[corner_count] = a;
        ++corner_count;
        
#if defined DEBUG_COLLISION
        
        rgba32 color = { 255, 255, 0, 255 };
        for (u32 a = 0; a < corner_count - 1; ++a) {
            for (u32 b = a + 1; b < corner_count; ++b) {
                draw_line(global_debug_draw_info.immediate_render_context, corners[a], corners[b], color);
            }
        }
        
        draw_circle(global_debug_draw_info.immediate_render_context, corners[corner_count - 1], global_debug_draw_info.camera_forward_direction, 0.1f, color);
        
        bool simplex_done = gjk_simplex(corners, &corner_count, &direction);
        
        ++global_debug_draw_info.current_iteration_count;
        
        if (global_debug_draw_info.max_iteration_count && ( global_debug_draw_info.current_iteration_count == global_debug_draw_info.max_iteration_count))
            return false;
        
        if (!simplex_done)
            DEBUG_DRAW_REVERT(checkpoint);
        
        if (simplex_done)
            return true;
#endif
        
        if (gjk_simplex(corners, &corner_count, &direction))
            return true;
    }
}

#if 0
void get_contact_info(float *contact_depth, float *contact_point, float *contact_normal, const void *object, Support_Funtion support, const float *contact_direction) {
    float corners[12];
    float direction[3];
    size_t corner_count = 1;
    
    set(direction, contact_direction, 3);
    support(corners, object, direction);
    
    float tmp[3];
    cross(tmp, corners, contact_direction, 3);
    cross(direction, tmp, corners, 3);
    
    if (length_squared(direction, 3) == 0.0) {
        set(contact_point, corners, 3);
        set(contact_normal, vec3_zero, 3);
        *contact_depth = dot(contact_point, contact_direction, 3) / length_squared(contact_direction, 3);
        
        return;
    }
    
    for(;;) {// && (it < max_it)) {
        float *a = corners + 3 * corner_count;
        support(a, object, direction);
        ++corner_count;
        
        switch (corner_count) {
            case 2: {
                float *b = corners;
                
                /*if (equals(a, b, 3)) {
                set(contact_point, a, 3);
                set(contact_normal, vec3_zero, 3);
                break;
                }*/
                
                float tmp[3];
                cross(tmp, a, contact_direction, 3);
                
                float ao_normal[3];
                cross(ao_normal, tmp, a, 3);
                
                if (dot(ao_normal, b, 3) <= 0.0f) {
                    set(b, a, 3);
                    corner_count = 1;
                    set(direction, ao_normal, 3);
                }
                else {
                    float ab[3];
                    sub(ab, b, a, 3);
                    
                    float tmp[3];
                    cross(tmp, ab, contact_direction, 3);
                    cross(direction, tmp, ab, 3);
                }
                
                break;
            }
            case 3: {
                float *c = corners;
                float *b = corners + 3;
                
                float ab[3];
                sub(ab, b, a, 3);
                
                float ac[3];
                sub(ac, c, a, 3);
                
                const float epsilon_squared = 0.00001f * 0.00001f;
                
                if (length_squared(ab, 3) < epsilon_squared || length_squared(ac, 3) < epsilon_squared) {
                    float bc[3];
                    sub(bc, c, b, 3);
                    
                    float tmp[3];
                    cross(tmp, bc, contact_direction, 3);
                    cross(contact_normal, tmp, bc, 3);
                    
                    *contact_depth = dot(contact_normal, b, 3) / dot(contact_normal, contact_direction, 3);
                    mul(contact_point, contact_direction, *contact_depth, 3);
                    
                    return;
                }
                
                float tmp[3];
                cross(tmp, a, contact_direction, 3);
                
                float ao_normal[3];
                cross(ao_normal, tmp, a, 3);
                
                float db = dot(ao_normal, b, 3);
                float dc = dot(ao_normal, c, 3);
                
                if (db * dc > 0.0f) {
                    float bc[3];
                    sub(bc, c, b, 3);
                    
                    float tmp[3];
                    cross(tmp, bc, contact_direction, 3);
                    cross(contact_normal, tmp, bc, 3);
                    
                    *contact_depth = dot(contact_normal, b, 3) / dot(contact_normal, contact_direction, 3);
                    mul(contact_point, contact_direction, *contact_depth, 3);
                    
                    return;
                }
                
                if (db > dc) {
                    set(c, a, 3);
                }
                else {
                    set(b, a, 3);
                }
                
                corner_count = 2;
                
                float bc[3];
                sub(bc, c, b, 3);
                
                cross(tmp, bc, contact_direction, 3);
                assert(length_squared(tmp, 3) > 0.0f);
                cross(direction, tmp, bc, 3);
                
                break;
            }
            default:
            assert(0);
            break;
        }
    }
}
#endif



// returns 0.0f <= t <= 1.0f
// for a maximum t <= 1.0f, where translation of Sphere A by move_dir_a * t is safe to perform without colliding with Sphere B
inline float get_collision_distance(SPHERE *a, VECTOR move_dir_a, SPHERE *b) {
    VECTOR distance = a->center - b->center;
    
    float d2 = squared_length(distance);
    float m2 = squared_length(move_dir_a);
    float md = dot(distance, move_dir_a);
    float r2 = a->radius + b->radius;
    r2 = r2 * r2;
    
    if (md >= 0.0f)
        return 1.0f;
    
    assert(m2 > 0.0f);
    
    float p_2 = -md / m2;
    
    float under_root = p_2 * p_2 + (r2 - d2) / m2;
    
    // no intersection
    if (under_root < 0.0f)
        return 1.0f;
    
    float root = sqrt(under_root);
    float t0 = p_2 - root;
    float t1 = p_2 + root;
    
    float t;
    if (t0 > 0.0f)
        t = t0;
    else
        t = t1;
    
    // t < 0.0f collision happend in the "past" or intersection in initial state
    if (t > 1.0f || t < 0.0f)
        t = 1.0f;
    
    return t;
}

inline float get_collision_distance(SPHERE *a, VECTOR move_dir_a, SPHERE *b, VECTOR move_dir_b) {
    return get_collision_distance(a, move_dir_a - move_dir_b, b);
}

struct Ray {
    VECTOR origin, direction;
};

inline VECTOR get_point_at(Ray ray, float distance) {
    return ray.origin + ray.direction * distance;
}

inline Ray transform_ray(mat4x3f transform, Ray ray) {
    return { transform_point(transform, ray.origin), transform_direction(transform, ray.direction) };
}

inline float get_plane_distance(VECTOR plane_point, VECTOR plane_normal, VECTOR point) {
    return dot(plane_normal, point - plane_point);
}

bool is_intersecting_ray_polygon(OUTPUT VECTOR *point, OUTPUT float *distance, Ray ray, u8 *vertices, u32 vertex_stride_in_bytes, Indices indices, u32 corner_offset, u32 corner_count) {
    DEBUG_DRAW_PUSH_LEVEL();
    
    assert(corner_count > 2);
    assert(is_unit_length(ray.direction));
    
    VECTOR *a = CAST_P(VECTOR, vertices + vertex_stride_in_bytes * get_index(indices, corner_offset + 0));
    VECTOR *b = CAST_P(VECTOR, vertices + vertex_stride_in_bytes * get_index(indices, corner_offset +  1));
    VECTOR *c = CAST_P(VECTOR, vertices + vertex_stride_in_bytes * get_index(indices, corner_offset +  2));
    
    Render_Batch_Checkpoint checkpoint = DEBUG_DRAW_CHECKPOINT();
    
    VECTOR ab = *b - *a;
    VECTOR ac = *c - *a;
    
    VECTOR face_normal = cross(ab, ac);
    
    {
        rgba32 color = { 255, 255, 0, 255 };
        DEBUG_DRAW_LINE(*a, *b, color);
        DEBUG_DRAW_LINE(*a, *c, color);
        DEBUG_DRAW_LINE(*c, *b, color);
        
        color = { 255, 0, 255, 255 };
        DEBUG_DRAW_LINE((*a + * b + *c) / 3.0f, (*a + * b + *c) / 3.0f + normalize(face_normal), color);
    }
    
    float l = dot(face_normal, ray.direction);
    
    // ray is parallel to triangle plane, ignored even if its inside the triangle
    // it should hopefully be detected by adjacent triangles if the mesh is closed
    if (l == 0.0f) {
        return false;
    } else {
        float t = dot(face_normal, *a - ray.origin) / l;
        
        if ((t < 0.0f) || (t >= *distance)) {
            DEBUG_DRAW_REVERT(checkpoint);
            return false;
        }
        
        VECTOR hit = ray.origin + ray.direction * t;
        
        // check if point is inside all planes described by edge normals
        VECTOR *last_vertex = CAST_P(VECTOR, vertices + vertex_stride_in_bytes * get_index(indices, corner_offset + corner_count - 1));
        for (u32 i = 0; i < corner_count; ++i) {
            VECTOR *vertex = CAST_P(VECTOR, vertices + vertex_stride_in_bytes * get_index(indices, corner_offset + i));
            VECTOR edge_normal = cross(face_normal, *vertex - *last_vertex);
            
            if (get_plane_distance(*vertex, edge_normal, hit) < 0.0f) {
                DEBUG_DRAW_REVERT(checkpoint);
                
                rgba32 color = { 255, 0, 0, 255 };
                DEBUG_DRAW_LINE(*last_vertex, *vertex, color);
                
                color = { 255, 0, 255, 255 };
                DEBUG_DRAW_LINE((*last_vertex + *vertex) * 0.5f, (*last_vertex + *vertex) * 0.5f + normalize(edge_normal),  color);
                
                return false;
            }
            
            last_vertex = vertex;
        }
        
        *point = hit;
        *distance = t;
        
        {
            rgba32 color = { 0, 255, 255, 255 };
            DEBUG_DRAW_CIRCLE(*point, 0.01f, color);
        }
        
        return true;
    }
}

bool is_intersecting_ray_triangles(OUTPUT VECTOR *point, OUTPUT float *distance,
                                   Ray ray, u8 *vertices, u32 vertex_stride_in_bytes, Indices indices, u32 first_triangle_corner_index, u32 triangle_count) {
    DEBUG_DRAW_PUSH_LEVEL();
    
    Render_Batch_Checkpoint checkpoint = DEBUG_DRAW_CHECKPOINT();
    
    bool is_intersecting = false;
    
    for (u32 i = 0; i < triangle_count; ++i) {
        
        DEBUG_DRAW_DISABLE();
        
        if (is_intersecting_ray_polygon(point, distance, ray, vertices, vertex_stride_in_bytes, indices, first_triangle_corner_index + 3 * i, 3)) {
            is_intersecting = true;
            
            DEBUG_DRAW_REVERT(checkpoint);
            DEBUG_DRAW_ENABLE();
            
            rgba32 color = { 255, 255, 0, 255 };
            for (u32 a = 0; a < 2; ++a) {
                VECTOR *va =CAST_P(VECTOR, vertices + vertex_stride_in_bytes * get_index(indices, first_triangle_corner_index + 3 * i + a));
                
                for (u32 b = a + 1; b < 3; ++b) {
                    VECTOR *vb =CAST_P(VECTOR, vertices + vertex_stride_in_bytes * get_index(indices, first_triangle_corner_index + 3 * i + b));
                    
                    DEBUG_DRAW_LINE(*va, *vb, color);
                }
            }
            
            DEBUG_DRAW_CIRCLE(*point, 0.01f, color);
        }
    }
    
    return is_intersecting;
}

#endif // ENGINE_COLLISION_H