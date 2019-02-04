#include <stdint.h>
#include <cmath>

struct VEC_NAME {
    
#if defined VEC_VALUE_UNIONS
    union {
        REAL values[VEC_LENGTH];
        VEC_VALUE_UNIONS
    };
#else
    REAL values[VEC_LENGTH];
#endif
    
    inline REAL & operator[](u32 i) {
        assert(i < VEC_LENGTH);
        return values[i];
    }
    
    inline REAL operator[](u32 i) const {
        assert(i < VEC_LENGTH);
        return values[i];
    }
    
    inline operator REAL *() { return values; }
    inline operator const REAL *() const { return values; }
};

inline void set_all(VEC_NAME *a, REAL scalar) {
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        a->values[i] = scalar;
}

inline void set_axis(VEC_NAME *a, u32 axis) {
    assert(axis < VEC_LENGTH);
    for (u32 i = 0; i != axis; ++i)
        a->values[i] = (REAL)0;
    
    a->values[axis] = (REAL)1;
    
    for (u32 i = axis + 1; i != VEC_LENGTH; ++i)
        a->values[i] = (REAL)0;
}

inline VEC_NAME operator+(const VEC_NAME &a, const VEC_NAME &b) {
    VEC_NAME result;
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        result.values[i] = a.values[i] + b.values[i];
    return result;
}

inline VEC_NAME & operator+=(VEC_NAME &a, const VEC_NAME &b) {
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        a.values[i] += b.values[i];
    return a;
}

inline VEC_NAME operator-(const VEC_NAME &a, const VEC_NAME &b) {
    VEC_NAME result;
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        result.values[i] = a.values[i] - b.values[i];
    return result;
}

inline VEC_NAME & operator-=(VEC_NAME &a, const VEC_NAME &b) {
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        a.values[i] -= b.values[i];
    return a;
}

inline VEC_NAME operator*(const VEC_NAME &a, const VEC_NAME &b) {
    VEC_NAME result;
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        result.values[i] = a.values[i] * b.values[i];
    return result;
}

inline VEC_NAME & operator*=(VEC_NAME &a, const VEC_NAME &b) {
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        a.values[i] *= b.values[i];
    return a;
}

inline VEC_NAME operator*(const VEC_NAME &a, const REAL &b) {
    VEC_NAME result;
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        result.values[i] = a.values[i] * b;
    return result;
}

inline VEC_NAME & operator*=(VEC_NAME &a, const REAL &b) {
    for (u32 i = 0; i != VEC_LENGTH; ++i)
        a.values[i] *= b;
    return a;
}

inline VEC_NAME operator-(const VEC_NAME &a) {
    return a * (REAL)-1;
}

inline VEC_NAME operator/(const VEC_NAME &a, const REAL &b) {
    assert(b != (REAL)0);
    return a * ((REAL)1 / b);
}

inline VEC_NAME & operator/=(VEC_NAME &a, const REAL &b) {
    assert(b != (REAL)0);
    return (a *= ((REAL)1 / b));
}

inline VEC_NAME operator/(const VEC_NAME &a, const VEC_NAME &b) {
    VEC_NAME result;
    for (u32 i = 0; i != VEC_LENGTH; ++i) {
        assert(b.values[i] != (REAL)0);
        result.values[i] = a.values[i] / b.values[i];
    }
    return result;
}

inline VEC_NAME & operator/=(VEC_NAME &a, const VEC_NAME &b) {
    for (u32 i = 0; i != VEC_LENGTH; ++i) {
        assert(b.values[i] != (REAL)0);
        a.values[i] /= b.values[i];
    }
    return a;
}

inline REAL dot(const VEC_NAME &a, const VEC_NAME &b) {
    REAL d = a.values[0] * b.values[0];
    for (u32 i = 1; i != VEC_LENGTH; ++i)
        d += a.values[i] * b.values[i];
    return d;
}

inline REAL squared_length(const VEC_NAME &a) {
    return dot(a, a);
}

inline REAL length(const VEC_NAME &a) {
    return (REAL)sqrt(dot(a, a));
}

inline VEC_NAME normalize(VEC_NAME a) {
    return a / length(a);
}

inline VEC_NAME normalize_or_zero(VEC_NAME a) {
    REAL l2 = squared_length(a);
    
    if (l2 != cast_v(REAL, 0))
        return a / sqrt(l2);
    
    return a;
}

inline VEC_NAME cross(VEC_NAME right_hand_thumb, VEC_NAME right_hand_index) {
    VEC_NAME right_hand_middle;
    
    for (u32 i = 0; i != VEC_LENGTH; ++i) {
        right_hand_middle.values[i] =	right_hand_thumb.values[(i + 1) % VEC_LENGTH] * right_hand_index.values[(i + 2) % VEC_LENGTH]
            -
            right_hand_thumb.values[(i + 2) % VEC_LENGTH] * right_hand_index.values[(i + 1) % VEC_LENGTH];
    }
    
    return right_hand_middle;
}

inline REAL projection_length(const VEC_NAME &a, const VEC_NAME &onto_b) {
    return dot(onto_b, a) / squared_length(onto_b);
}

inline VEC_NAME project(const VEC_NAME &a, const VEC_NAME &onto_b) {
    return onto_b * projection_length(a, onto_b);
}

inline VEC_NAME reflect(VEC_NAME mirror_normal, VEC_NAME ray) {
    return ray - mirror_normal * (2 * dot(mirror_normal, ray));
}

INTERNAL VEC_NAME linear_interpolation(VEC_NAME a, VEC_NAME b, REAL blend_factor) {
    return a * ((REAL)1 - blend_factor) + b * blend_factor;
}

INTERNAL VEC_NAME linear_interpolation(VEC_NAME a, VEC_NAME b, VEC_NAME blend_factor) {
    VEC_NAME one;
    set_all(&one, (REAL)1.0);
    return a * (one - blend_factor) + b * blend_factor;
}

inline bool is_unit_length(const VEC_NAME &vector) {
    return are_close(length(vector), (REAL)1);
}

INTERNAL VEC_NAME abs(VEC_NAME vector) {
    VEC_NAME result;
    for (u32 i = 0; i < ARRAY_COUNT(vector.values); ++i)
        result[i] = ABS(vector[i]);
    
    return result;
}

// converts
// to: -1.0 <= interval_minus_one_to_one <= 1.0
// from: 0 <= interval_zero_to_one <= 1.0
INTERNAL VEC_NAME interval_zero_to_one(VEC_NAME interval_minus_one_to_plus_one) {
    VEC_NAME one;
    set_all(&one, (REAL)1.0);
    return (interval_minus_one_to_plus_one + one) * (REAL)0.5;
}

// converts
// from: 0 <= interval_zero_to_one <= 1.0
// to: -1.0 <= interval_minus_one_to_plus_one <= 1.0
INTERNAL VEC_NAME interval_minus_one_to_plus_one(VEC_NAME interval_zero_to_one) {
    VEC_NAME one;
    set_all(&one, (REAL)1.0);
    return interval_zero_to_one * (REAL)2.0 - one;
}

#undef VEC_NAME
#undef VEC_LENGTH

#if defined VEC_VALUE_UNIONS
#undef VEC_VALUE_UNIONS
#endif
