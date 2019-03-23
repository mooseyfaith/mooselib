
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  template_vector.h                                                         //
//                                                                            //
//  Tafil Kajtazi                                                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <cmath>

#include "basic.h"

#if !defined Template_Vector_Name
#  error Template_Vector_Name needs to be defined befor including template_vector.h
#endif

#if !defined Template_Vector_Data_Type
#  error Template_Vector_Data_Type needs to be defined befor including template_vector.h
#endif

#if !defined Template_Vector_Dimension_Count
#  error Template_Vector_Dimension_Count needs to be defined befor including template_vector.h
#endif

struct Template_Vector_Name {
    
#if defined Template_Vector_Union
    
    union {
        Template_Vector_Data_Type values[Template_Vector_Dimension_Count];
        Template_Vector_Union
    };
    
#  undef Template_Vector_Union
    
#else
    
    Template_Vector_Data_Type values[Template_Vector_Dimension_Count];
    
#endif
    
    inline Template_Vector_Data_Type & operator[](u32 i) {
        assert(i < Template_Vector_Dimension_Count);
        return values[i];
    }
    
    inline Template_Vector_Data_Type operator[](u32 i) const {
        assert(i < Template_Vector_Dimension_Count);
        return values[i];
    }
    
    inline operator Template_Vector_Data_Type *() { return values; }
    inline operator const Template_Vector_Data_Type *() const { return values; }
};

INTERNAL void set_all(Template_Vector_Name *a, Template_Vector_Data_Type scalar) {
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        a->values[i] = scalar;
}

INTERNAL void set_axis(Template_Vector_Name *a, u32 axis) {
    assert(axis < Template_Vector_Dimension_Count);
    for (u32 i = 0; i != axis; ++i)
        a->values[i] = (Template_Vector_Data_Type)0;
    
    a->values[axis] = (Template_Vector_Data_Type)1;
    
    for (u32 i = axis + 1; i != Template_Vector_Dimension_Count; ++i)
        a->values[i] = (Template_Vector_Data_Type)0;
}

INTERNAL Template_Vector_Name
operator+(Template_Vector_Name a, Template_Vector_Name b) {
    Template_Vector_Name result;
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        result.values[i] = a.values[i] + b.values[i];
    
    return result;
}

INTERNAL Template_Vector_Name &
operator+=(Template_Vector_Name &a, Template_Vector_Name b) {
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        a.values[i] += b.values[i];
    
    return a;
}

INTERNAL Template_Vector_Name
operator-(Template_Vector_Name a, Template_Vector_Name b) {
    Template_Vector_Name result;
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        result.values[i] = a.values[i] - b.values[i];
    
    return result;
}

INTERNAL Template_Vector_Name &
operator-=(Template_Vector_Name &a, Template_Vector_Name b) {
    for (u32 i = 0; i != Template_Vector_Dimension_Count; ++i)
        a.values[i] -= b.values[i];
    
    return a;
}

INTERNAL Template_Vector_Name
operator*(Template_Vector_Name a, Template_Vector_Name b) {
    Template_Vector_Name result;
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        result.values[i] = a.values[i] * b.values[i];
    
    return result;
}

INTERNAL Template_Vector_Name &
operator*=(Template_Vector_Name &a, Template_Vector_Name b) {
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        a.values[i] *= b.values[i];
    
    return a;
}

INTERNAL Template_Vector_Name
operator*(Template_Vector_Name a, Template_Vector_Data_Type b) {
    Template_Vector_Name result;
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        result.values[i] = a.values[i] * b;
    
    return result;
}

INTERNAL Template_Vector_Name &
operator*=(Template_Vector_Name &a, Template_Vector_Data_Type b) {
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        a.values[i] *= b;
    
    return a;
}

INTERNAL Template_Vector_Name
operator-(Template_Vector_Name a) {
    return a * -cast_v(Template_Vector_Data_Type, 1);
}

INTERNAL Template_Vector_Name
operator/(Template_Vector_Name a, Template_Vector_Data_Type b) {
    assert(b != cast_v(Template_Vector_Data_Type, 0));
    return a * (cast_v(Template_Vector_Data_Type, 1) / b);
}

INTERNAL Template_Vector_Name &
operator/=(Template_Vector_Name &a, Template_Vector_Data_Type b) {
    assert(b != cast_v(Template_Vector_Data_Type, 0));
    return (a *= (cast_v(Template_Vector_Data_Type, 1) / b));
}

INTERNAL Template_Vector_Name
operator/(Template_Vector_Name a, Template_Vector_Name b) {
    Template_Vector_Name result;
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i) {
        assert(b.values[i] != cast_v(Template_Vector_Data_Type, 0));
        result.values[i] = a.values[i] / b.values[i];
    }
    
    return result;
}

INTERNAL Template_Vector_Name &
operator/=(Template_Vector_Name &a, Template_Vector_Name b) {
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i) {
        assert(b.values[i] != cast_v(Template_Vector_Data_Type, 0));
        a.values[i] /= b.values[i];
    }
    
    return a;
}

INTERNAL Template_Vector_Data_Type
dot(Template_Vector_Name a, Template_Vector_Name b) {
    Template_Vector_Data_Type d = a.values[0] * b.values[0];
    for (u32 i = 1; i < Template_Vector_Dimension_Count; ++i)
        d += a.values[i] * b.values[i];
    
    return d;
}

INTERNAL Template_Vector_Data_Type
squared_length(Template_Vector_Name a) {
    return dot(a, a);
}

INTERNAL Template_Vector_Data_Type
length(Template_Vector_Name a) {
    return cast_v(Template_Vector_Data_Type, sqrt(squared_length(a)));
}

INTERNAL Template_Vector_Name
normalize(Template_Vector_Name a) {
    return a / length(a);
}

INTERNAL Template_Vector_Name
normalize_or_zero(Template_Vector_Name a) {
    Template_Vector_Data_Type l2 = squared_length(a);
    
    if (l2 != cast_v(Template_Vector_Data_Type, 0))
        return a / sqrt(l2);
    
    return a;
}

INTERNAL Template_Vector_Name
cross(Template_Vector_Name right_hand_thumb, Template_Vector_Name right_hand_index) {
    Template_Vector_Name right_hand_middle;
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i) {
        right_hand_middle.values[i] =	right_hand_thumb.values[(i + 1) % Template_Vector_Dimension_Count] * right_hand_index.values[(i + 2) % Template_Vector_Dimension_Count]
            -
            right_hand_thumb.values[(i + 2) % Template_Vector_Dimension_Count] * right_hand_index.values[(i + 1) % Template_Vector_Dimension_Count];
    }
    
    return right_hand_middle;
}

INTERNAL Template_Vector_Data_Type
projection_length(Template_Vector_Name a, Template_Vector_Name onto_b) {
    return dot(onto_b, a) / squared_length(onto_b);
}

INTERNAL Template_Vector_Name
project(Template_Vector_Name a, Template_Vector_Name onto_b) {
    return onto_b * projection_length(a, onto_b);
}

INTERNAL Template_Vector_Name
reflect(Template_Vector_Name mirror_normal, Template_Vector_Name ray) {
    return ray - mirror_normal * (2 * dot(mirror_normal, ray));
}

INTERNAL Template_Vector_Name
linear_interpolation(Template_Vector_Name a, Template_Vector_Name b, Template_Vector_Data_Type blend_factor) {
    return a * ((Template_Vector_Data_Type)1 - blend_factor) + b * blend_factor;
}

INTERNAL Template_Vector_Name
linear_interpolation(Template_Vector_Name a, Template_Vector_Name b, Template_Vector_Name blend_factor) {
    Template_Vector_Name one;
    set_all(&one, cast_v(Template_Vector_Data_Type, 1));
    return a * (one - blend_factor) + b * blend_factor;
}

INTERNAL bool
is_unit_length(Template_Vector_Name vector) {
    return are_close(length(vector), cast_v(Template_Vector_Data_Type, 1));
}

INTERNAL Template_Vector_Name
abs(Template_Vector_Name vector) {
    Template_Vector_Name result;
    
    for (u32 i = 0; i < Template_Vector_Dimension_Count; ++i)
        result[i] = ABS(vector[i]);
    
    return result;
}

// converts
// to: -1.0 <= interval_minus_one_to_one <= 1.0
// from: 0 <= interval_zero_to_one <= 1.0
INTERNAL Template_Vector_Name
interval_zero_to_one(Template_Vector_Name interval_minus_one_to_plus_one) {
    Template_Vector_Name one;
    set_all(&one, cast_v(Template_Vector_Data_Type, 1));
    
    return (interval_minus_one_to_plus_one + one) * cast_v(Template_Vector_Data_Type, 0.5);
}

// converts
// from: 0 <= interval_zero_to_one <= 1.0
// to: -1.0 <= interval_minus_one_to_plus_one <= 1.0
INTERNAL Template_Vector_Name
interval_minus_one_to_plus_one(Template_Vector_Name interval_zero_to_one) {
    Template_Vector_Name one;
    set_all(&one, cast_v(Template_Vector_Data_Type, 1));
    
    return interval_zero_to_one * cast_v(Template_Vector_Data_Type, 2) - one;
}

#undef Template_Vector_Name
#undef Template_Vector_Data_Type
#undef Template_Vector_Dimension_Count