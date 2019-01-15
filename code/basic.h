#if !defined BASIC_H
#define BASIC_H

#include <stdarg.h>

// parameter annotations
#define OPTIONAL
#define OUTPUT
#define GLOBAL
#define INTERNAL static inline

typedef char                    s8;
typedef short                  s16; // pixel/texel sizes, coordinates
typedef int                    s32; // default signed type
typedef long int               s64;

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

// it just looks nicer?
#define null 0

const usize u32_min = 0;
const usize u32_max = -u32(1);

const usize u64_min = 0;
const usize u64_max = -u64(1);

const usize usize_min = 0;
const usize usize_max = -usize(1);


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


#define UNREACHABLE_CODE assert(0);

#define ARRAY_COUNT(array) CAST_V(u32, sizeof(array) / sizeof(*(array)))

#define ARRAY_WITH_COUNT(array) array, ARRAY_COUNT(array)
#define COUNT_WITH_ARRAY(array) ARRAY_COUNT(array), array
#define ARRAY_INFO(array) { ARRAY_WITH_COUNT(array) }

#define ARRAY_INDEX(array, it) ((it) - (array))

#define KILO(k) ((usize)k << 10)
#define MEGA(m) ((usize)m << 20)
#define GIGA(g) ((usize)g << 30)

#define CAST_P(type, pointer) reinterpret_cast<type *> (pointer)
#define CAST_V(type, value) static_cast<type> (value)
#define CAST_ANY(pointer) reinterpret_cast<any> (pointer)
#define MEMORY_ADDRESS(pointer) reinterpret_cast<usize> (CAST_P(u8, pointer))

#define cast_p(type, pointer)   CAST_P(type, pointer)
#define cast_v(type, value)     CAST_V(type, value)
#define cast_any(pointer)       CAST_ANY(pointer)
#define memory_address(pointer) MEMORY_ADDRESS(pointer)

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

#define COPY(destination, source, size_in_bytes) copy(CAST_ANY(destination), CAST_ANY(source), CAST_V(usize, size_in_bytes))
INTERNAL void copy(any destination, any source, usize size_in_bytes) {
    if (destination > source) {
        u8 *dest_end = CAST_P(u8, destination) - 1;
        u8 *src = CAST_P(u8, source) + size_in_bytes - 1;
        u8 *dest = dest_end + size_in_bytes;
        
        while (dest != dest_end) {
            *(dest--) = *(src--);
        }
    } else {
        u8 *dest = CAST_P(u8, destination);
        u8 *src = CAST_P(u8, source);
        u8 *dest_end = dest + size_in_bytes;
        
        while (dest != dest_end) {
            *(dest++) = *(src++);
        }
    }
}

INTERNAL void reset(any destination, usize size_in_bytes, u8 value = 0) {
    u8 *dest = CAST_P(u8, destination);
    u8 *end = dest + size_in_bytes;
    for (u8 *it = dest; it != end; ++it)
        *it = value;
}

#define ARE_EQUAL(a, b, size_in_bytes) are_equal(cast_p(u8, a), cast_p(u8, b), CAST_V(usize, size_in_bytes))

INTERNAL bool are_equal(u8 *a, u8 *b, usize size_in_bytes) {
    u8 *a_end = a + size_in_bytes;
    
    while (a != a_end) {
        if (*(a++) != *(b++))
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
    return dimensions.width / CAST_V(f32, dimensions.height);
}

INTERNAL f32 height_over_width(Pixel_Dimensions dimensions) {
    return dimensions.height / CAST_V(f32, dimensions.width);
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
    COPY(dest, cast_p(u8, &index), indices->bytes_per_index);
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