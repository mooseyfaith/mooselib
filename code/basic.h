#if !defined BASIC_H
#define BASIC_H

#include <stdarg.h>
#include <climits>

// parameter annotations
#define OPTIONAL
#define OUTPUT
#define GLOBAL
#define INTERNAL static

typedef char                    s8;
typedef short                  s16; // pixel/texel sizes, coordinates
typedef int                    s32; // default signed type
typedef long long int          s64;

typedef unsigned char           u8;
typedef unsigned short         u16; // pixel/texel sizes, coordinates
typedef unsigned int           u32; // default signed type
typedef unsigned long long int u64;

typedef float                  f32; // default floating point
typedef double                 f64;

// memory size, offsets, pointer offsets, same size as pointers
#if defined _M_X64
typedef u64 usize;
#else
typedef u32 usize;
#endif

typedef void * any;
typedef char const * c_const_string; 


#define HALF_RANGE(bit_count) (u64(1) << (bit_count - 1))

#define DEF_SIGNED_INTEGER_MIN_MAX(bit_count) \
const auto s ## bit_count ## _min = - s ## bit_count (HALF_RANGE(bit_count)); \
const auto s ## bit_count ## _max =   s ## bit_count (HALF_RANGE(bit_count) - 1);

DEF_SIGNED_INTEGER_MIN_MAX(8);
DEF_SIGNED_INTEGER_MIN_MAX(16);
DEF_SIGNED_INTEGER_MIN_MAX(32);

#define DEF_UNSIGNED_INTEGER_MIN_MAX(bit_count) \
const auto u ## bit_count ## _min =  u ## bit_count (0); \
const auto u ## bit_count ## _max = -u ## bit_count (1);

DEF_UNSIGNED_INTEGER_MIN_MAX(8);
DEF_UNSIGNED_INTEGER_MIN_MAX(16);
DEF_UNSIGNED_INTEGER_MIN_MAX(32);
DEF_UNSIGNED_INTEGER_MIN_MAX(64);

const auto usize_min = usize(0);
const auto usize_max = -usize(1);

#include <float.h>

const f32 f32_min     = FLT_MIN;
const f32 f32_max     = FLT_MAX;
const f32 f32_epsilon = FLT_EPSILON;

const f64 f64_min     = DBL_MIN;
const f64 f64_max     = DBL_MAX;
const f64 f64_epsilon = DBL_EPSILON;

// it just looks nicer?
#define null 0

#if defined(DEBUG)

#include <stdio.h>

#define assert(x, ...) { \
    if (!(x)) { \
        printf("\n\n%s:%d:\n\n\t Assertion ( %s ) failed\n\n\t" __VA_ARGS__, __FILE__, __LINE__, # x); \
        *((int*)0) = 0; \
    } \
}

#else

#define assert(x, ...)

#endif

#define UNREACHABLE_CODE assert(0, "unreachbale code");
#define CASES_COMPLETE default: UNREACHABLE_CODE

#define ARRAY_COUNT(array) cast_v(usize, sizeof(array) / sizeof(*(array)))

#define ARRAY_WITH_COUNT(array) array, ARRAY_COUNT(array)
#define COUNT_WITH_ARRAY(array) ARRAY_COUNT(array), array
#define ARRAY_INFO(array) { ARRAY_WITH_COUNT(array) }

#define ARRAY_INDEX(array, it) ((it) - (array))

#define KILO(k) ((usize)k << 10)
#define MEGA(m) ((usize)m << 20)
#define GIGA(g) ((usize)g << 30)

#define cast_p(type, pointer)   reinterpret_cast<type *> (pointer)
#define cast_v(type, value)     static_cast     <type>   (value)
#define cast_any(pointer)       reinterpret_cast<any>    (pointer)
#define memory_address(pointer) reinterpret_cast<usize>  (cast_p(u8, pointer))

#define ABS(a) ((a) < 0 ? -(a) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define CLAMP(a, min, max) ((a) < (min) ? (min) : MIN(a, max))
#define DEADZONE(a, abs_min) (((-(abs_min) < (a)) && ((a) < (abs_min))) ? 0 : (a))

#define FLAG(bit) (1 << bit)
#define CHECK_FLAG(value, flags) ((value) & (flags))
#define CHECK_MASK(value, mask) (CHECK_FLAG(value, mask) == (mask))

#define STRINGIFY__(a) # a
#define STRINGIFY_(a)  STRINGIFY__(a)
#define STRINGIFY(a)   STRINGIFY_(a)

#define CHAIN_(a, b) a ## b
#define CHAIN(a, b) CHAIN_(a, b)

#define MOD(a, b) ((a) % (b))

#define RING_NEXT(a, max) (((a) + 1) % (max))
#define RING_PREV(a, max) (((a) + (max) - 1) % (max))
#define RING_INCREMENT(a, max)  (a) = RING_NEXT(a, max)
#define RING_DECREMENT(a, max)  (a) = RING_PREV(a, max)
#define RING_ADD(a, delta, max) (a) = ((a) + (delta)              ) % (max)
#define RING_SUB(a, delta, max) (a) = ((a) + ((max) - 1) * (delta)) % (max)

#define RING_PREV_EX(a, min, max) (RING_PREV((a) - (min), (max) - (min)) + (min))
#define RING_NEXT_EX(a, min, max) (RING_NEXT((a) - (min), (max) - (min)) + (min))

// sum = 0; for (i = 1; i <= count) sum += i;
#define RANGE_SUM(count) ((count) * ((count) + 1) / 2)

#define LOOP for(;;)

// for tagged unions
// example
#if 0
struct A {
    enum Kind {
        Kind_foo,
        Kind_bar,
        Kind_Count
    } kind;
    
    union {
        struct {
            u32 a, b;
        } foo;
        
        struct {
            f32 c;
            string d;
        } bar;
    }
};

A x = make_kind(A, foo, 1, 2); // a = 1, b = 2
asssert(is_kind(x, foo));
auto f = kind(x, foo);
use f->a, f->b
#endif

#define make_kind(type, k, ...) [&]() { type result = {}; result.kind = type::Kind_ ## k; result.k = { __VA_ARGS__ }; return result; }()
#define make_null_kind(type) { type::Kind_Count }

#define is_kind(a, k) ((a).kind == (a).Kind_ ## k)
#define is_kind_null(a) is_kind(a, Count)
#define try_kind_of(a, k) (is_kind(a, k) ? &(a).k : null)
#define kind_of(a, k) [&]() { assert(is_kind(a, k)); return &(a).k; }()

#define case_kind(type, kind) case type::Kind_ ## kind:

#define new_kind(allocator, type, k, ...) (&(*ALLOCATE(allocator, type) = make_kind(type, k, __VA_ARGS__)))

INTERNAL void copy(any destination, any source, usize size_in_bytes) {
    if (destination > source) {
        u8 *dest_end = cast_p(u8, destination) - 1;
        u8 *src = cast_p(u8, source) + size_in_bytes - 1;
        u8 *dest = dest_end + size_in_bytes;
        
        while (dest != dest_end) {
            *(dest--) = *(src--);
        }
    } else {
        u8 *dest = cast_p(u8, destination);
        u8 *src = cast_p(u8, source);
        u8 *dest_end = dest + size_in_bytes;
        
        while (dest != dest_end) {
            *(dest++) = *(src++);
        }
    }
}

INTERNAL void reset(any destination, usize byte_count, u8 value = 0) {
    u8 *dest = cast_p(u8, destination);
    u8 *end = dest + byte_count;
    for (u8 *it = dest; it != end; ++it)
        *it = value;
}

INTERNAL bool are_equal(any a, any b, usize byte_count) {
    auto a_it = cast_p(u8, a);
    auto b_it = cast_p(u8, b);
    
    u8 *a_end = a_it + byte_count;
    
    while (a_it != a_end) {
        if (*(a_it++) != *(b_it++))
            return false;
    }
    
    return true;
}

struct Pixel_Position {
    s16 x, y;
};

struct Pixel_Dimensions {
    s16 width, height;
};

struct Pixel_Rectangle {
    s16 x, y;
    union {
        struct { s16 width, height; };
        Pixel_Dimensions size;
    };
};

struct Pixel_Borders {
    s16 left, right;
    s16 bottom, top;
};

INTERNAL bool operator==(const Pixel_Rectangle &a, const Pixel_Rectangle &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.width == b.width) && (a.height == b.height);
}

INTERNAL bool operator!=(const Pixel_Rectangle &a, const Pixel_Rectangle &b) {
    return !(a == b);
}

INTERNAL bool contains(Pixel_Rectangle rect, s16 x, s16 y) {
    return (rect.x <= x) && (x < (rect.x + rect.width)) && (rect.y <= y) && (y < (rect.y + rect.height));
}

INTERNAL bool are_intersecting(Pixel_Rectangle a, Pixel_Rectangle b) {
    if (a.x <= b.x) {
        if (b.x >= a.x + a.width)
            return false;
    }
    else {
        if (a.x >= b.x + b.width)
            return false;
    }
    
    if (a.y <= b.y) {
        if (b.y >= a.y + a.height)
            return false;
    }
    else {
        if (a.y >= b.y + b.height)
            return false;
    }
    
    return true;
}

INTERNAL Pixel_Rectangle merge(Pixel_Rectangle a, Pixel_Rectangle b) {
    Pixel_Rectangle result;
    result.x = MIN(a.x, b.x);
    result.y = MIN(a.y, b.y);
    result.width  = MAX(a.x + a.width,  b.x + b.width)  - result.x;
    result.height = MAX(a.y + a.height, b.y + b.height) - result.y;
    
    return result;
}

INTERNAL f32 width_over_height(Pixel_Dimensions dimensions) {
    return dimensions.width / cast_v(f32, dimensions.height);
}

INTERNAL f32 height_over_width(Pixel_Dimensions dimensions) {
    return dimensions.height / cast_v(f32, dimensions.width);
}

// converts
// to: -1.0f <= interval_minus_one_to_one <= 1.0f
// from: 0 <= interval_zero_to_one <= 1.0f
INTERNAL f32 interval_zero_to_one(f32 interval_minus_one_to_plus_one) {
    return (interval_minus_one_to_plus_one + 1.0f) * 0.5f;
}


// converts
// from: 0 <= interval_zero_to_one <= 1.0f
// to: -1.0f <= interval_minus_one_to_plus_one <= 1.0f
INTERNAL f32 interval_minus_one_to_plus_one(f32 interval_zero_to_one) {
    return interval_zero_to_one * 2.0f - 1.0f;
}

#include "memory.h"

#include "u8_buffer.h"

struct Indices {
    u8_buffer buffer;
    u32 bytes_per_index;
};

INTERNAL void push_index(Indices *indices, u32 index) {
    u8 *dest = push(&indices->buffer, indices->bytes_per_index);
    // should work because of endieness
    copy(dest, &index, indices->bytes_per_index);
    //push(&indices->buffer, cast_p(u8, &index), indices->bytes_per_index);
}

INTERNAL u32 get_index(Indices indices, u32 offset) {
    switch (indices.bytes_per_index) {
        case 1:
        return indices.buffer[offset];
        
        case 2:
        //return *PACKED_ITEM_AT(&indices.buffer, u16, offset);
        //return *packed_front(indices.buffer, u16, offset);
        return *item_at(indices.buffer, u16, offset);
        
        case 4:
        //return *PACKED_ITEM_AT(&indices.buffer, u32, offset);
        //return *packed_front(indices.buffer, u32, offset);
        return *item_at(indices.buffer, u32, offset);
        
        default:
        UNREACHABLE_CODE;
        return -1;
    }
}

INTERNAL u32 index_count(Indices indices) {
    return indices.buffer.count / indices.bytes_per_index;
}

// TODO: add intrinsics
INTERNAL u32 bit_count(u32 value) {
    u32 result = 0;
    do {
        ++result;
        value >>= 1;
    } while (value);
    
    return result;
}

template <typename F>
struct Deferer {
    F f;
    
    Deferer(F f) : f(f) {}
    ~Deferer() { f(); }
};

template <typename F>
Deferer<F> operator++(F f) { return Deferer<F>(f); }

#define defer \
auto CHAIN(_defer_, __LINE__) = ++[&]()

#define SCOPE_PUSH(var, value) \
auto CHAIN(_scope_backup_, __LINE__) = var; \
var = value; \
defer { var = CHAIN(_scope_backup_, __LINE__); };

#endif // BASIC_H