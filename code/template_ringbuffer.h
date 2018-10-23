
#include "basic.h"

#define Array_Max_Index -1

#if !defined Template_Array_Type
#  error Template_Array_Type needs to be defined befor including buffer_template.h
#endif

#if !defined Template_Array_Data_Type
#  error Template_Array_Data_Type needs to be defined befor including buffer_template.h
#endif

#if !defined Template_Array_Size_Type
#  define Template_Array_Size_Type usize
#endif

#if defined Template_Array_Is_Buffer

#  if !defined Template_Array_Grow_Formula
#    define Template_Array_Grow_Formula(array) MAX((array)->capacity << 1, (array)->capacity + count);
#  endif

#  if !defined Template_Array_Shrink_Check
#    define Template_Array_Shrink_Check(array) (!(array)->count || ((array)->capacity / (array)->count >= 4))
#  endif

#  if !defined Template_Array_Shrink_Formula
#    define Template_Array_Shrink_Formula(array) MAX((array)->count, (array)->capacity >> 1)
#  endif

#else

#  if (defined Template_Array_Grow_Formula) || (defined Template_Array_Shrink_Check) || (defined Template_Array_Shrink_Formula)
#    error you can not define Template_Array_Grow_Formula, Template_Array_Shrink_Check, Template_Array_Shrink_Formula for an array which is not a buffer (Template_Array_Is_Buffer is not defined)
#  endif

#endif

struct Template_Array_Type;
INTERNAL Template_Array_Data_Type * front(Template_Array_Type array, Template_Array_Size_Type offset = 0);

struct Template_Array_Type {
    Template_Array_Data_Type *data;
    
    Template_Array_Size_Type capacity, count, head;
    
    inline Template_Array_Data_Type & operator[](Template_Array_Size_Type offset) {
        return *front(*this, offset);
    }
    
    inline Template_Array_Data_Type operator[](Template_Array_Size_Type offset) const {
        return *front(*this, offset);
    }
};

INTERNAL bool is_empty(Template_Array_Type array) {
    return !array.count;
}

INTERNAL Template_Array_Size_Type remaining_count(Template_Array_Type array) {
    return array.capacity - array.count;
}

INTERNAL bool is_full(Template_Array_Type array) {
    return !remaining_count(array);
}

INTERNAL usize count_in_bytes(Template_Array_Type array) {
    return sizeof(Template_Array_Data_Type) * array.count;
}

INTERNAL usize capacity_in_bytes(Template_Array_Type array) {
    return sizeof(Template_Array_Data_Type) * array.capacity;
}

INTERNAL Template_Array_Data_Type * first(Template_Array_Type array) {
    return array.data + array.head;
}

INTERNAL Template_Array_Data_Type * __x(Template_Array_Type array, Template_Array_Size_Type index) {
    if (array.head + index < array.capacity)
        return array.data + array.head + index;
    
    return array.data + array.head + index - array.capacity;
}

INTERNAL Template_Array_Data_Type * one_past_last(Template_Array_Type array) {
    return __x(array, array.count);
}

INTERNAL Template_Array_Data_Type * front(Template_Array_Type array, Template_Array_Size_Type plus_offset)
{
    assert(plus_offset < array.count);
    return __x(array, plus_offset);
}

INTERNAL Template_Array_Data_Type * front_or_null(Template_Array_Type array, Template_Array_Size_Type plus_offset = 0)
{
    if (plus_offset < array.count) 
        return __x(array, plus_offset);
    
    return null;
}

INTERNAL Template_Array_Data_Type * back(Template_Array_Type array, Template_Array_Size_Type minus_offset = 0)
{
    assert(minus_offset < array.count);
    return __x(array, array.count - 1 - minus_offset)
}

INTERNAL Template_Array_Data_Type * back_or_null(Template_Array_Type array, Template_Array_Size_Type minus_offset = 0)
{
    if (minus_offset < array.count)
        return __x(array, array.count - 1 - minus_offset);
    
    return null;
}

INTERNAL Template_Array_Size_Type index(Template_Array_Type array, Template_Array_Data_Type *item)
{
    if (item >= front(array)) {
        Template_Array_Size_Type index = cast_v(Template_Array_Size_Type, item - front(array));
        assert(index < array.capacity - array.head);
        return index;
    }
    
    Template_Array_Size_Type index = cast_v(Template_Array_Size_Type, item - array.data);
    assert(index < array.head);
    return array.capacity - array.head + index;
}

INTERNAL bool are_equal(Template_Array_Type a, Template_Array_Data_Type *b_data, Template_Array_Size_Type b_count) {
    if (a.count != b_count)
        return false;
    
    Template_Array_Size_Type count = MIN(a.capacity - a.head, a.count);
    
    if (!ARE_EQUAL(front(a), b_data, count * sizeof(Template_Array_Size_Type)))
        return false;
    
    return ARE_EQUAL(a.data, b_data + count, (a.count - count) * sizeof(Template_Array_Size_Type));
}

INTERNAL bool operator==(Template_Array_Type a, Template_Array_Type b) {
    if (a.count != b.count)
        return false;
    
    for (Template_Array_Size_Type i = 0; i < a.count; ++i) {
        if (*front(a, i) != *front(b, i))
            return false;
    }
    
    return true;
}

INTERNAL bool operator!=(Template_Array_Type a, Template_Array_Type b) {
    return !(a == b);
}

INTERNAL Template_Array_Type sub(Template_Array_Type array, Template_Array_Size_Type offset, Template_Array_Size_Type count_or_max = Array_Max_Index, bool offset_from_back = false)
{
    if (count_or_max == Array_Max_Index) {
        assert(offset < array.count);
        count_or_max = array.count - offset;
    }
    
    assert(offset + count_or_max <= array.count);
    
    if (!offset_from_back) 
        return { array.data, count_or_max, array.capacity, (array.head + offset) % array.capacity };
    else 
        return { array.data, count_or_max, array.capacity, (array.head + array.count - offset) % array.capacity };
}

INTERNAL void copy(Template_Array_Type *destination, Template_Array_Data_Type *data, Template_Array_Size_Type count)
{
    assert(count <= destination->count);
    COPY(destination->data, data, count_in_bytes(*destination));
}

INTERNAL void copy(Template_Array_Type *destination, Template_Array_Type source) {
    copy(destination, source.data, source.count);
}

INTERNAL void reset(Template_Array_Type array, Template_Array_Data_Type value) {
    for (Template_Array_Size_Type i = 0;
         i < array.count;
         ++i)
    {
        *front(array, i) = value;
    }
}

#if defined Template_Array_Is_Buffer

// buffer specific functions

INTERNAL void clear(Template_Array_Type *array) {
    array->count = 0;
}

#else

// array specific functions

INTERNAL void advance(Template_Array_Type *iterator, Template_Array_Size_Type count = 1) {
    assert(count <= iterator->count);
    iterator->data  += count;
    iterator->count -= count;
}

#endif

INTERNAL Template_Array_Data_Type * push(Template_Array_Type *array, Template_Array_Data_Type *data_or_null, Template_Array_Size_Type count, Memory_Allocator *allocator = null)
{
    Template_Array_Size_Type offset = array->count;
    
    if (remaining_count(*array) < count) {
        assert(allocator);
        
        if (array->capacity) {
#if defined Template_Array_Is_Buffer
            array->capacity = Template_Array_Grow_Formula(array);
#else
            array->capacity += count;
#endif
            REALLOCATE_ARRAY(allocator, &array->data, array->capacity);
        }
        else
            *array = ALLOCATE_ARRAY_INFO(allocator, Template_Array_Data_Type, count);
    }
    
    // buffer have seperate count and capacity
#if defined Template_Array_Is_Buffer
    array->count += count;
#endif
    
    auto result = front(*array, offset);
    
    if (data_or_null)
        COPY(result, data_or_null, count * sizeof(Template_Array_Data_Type));
    
    return result;
}

INTERNAL Template_Array_Data_Type * push(Template_Array_Type *array, Template_Array_Data_Type value, Memory_Allocator *allocator = null)
{
    return push(array, &value, 1, allocator);
}

INTERNAL Template_Array_Data_Type * ordered_insert(Template_Array_Type *array, Template_Array_Data_Type *data_or_null, Template_Array_Size_Type count = 1, Template_Array_Size_Type offset_or_max = Array_Max_Index, Memory_Allocator *allocator = null)
{
    if (offset_or_max == Array_Max_Index)
        offset_or_max = array->count;
    
    push(array, null, count, allocator);
    
    auto result = front(*array, offset_or_max);
    
    COPY(front(*array, offset_or_max + count), result, (array->count - offset_or_max - count) * sizeof(Template_Array_Data_Type));
    
    if (data_or_null)
        COPY(result, data_or_null, count * sizeof(Template_Array_Data_Type));
    
    return result;
}

INTERNAL Template_Array_Data_Type * ordered_insert(Template_Array_Type *array, Template_Array_Data_Type value, Template_Array_Size_Type offset_or_max = Array_Max_Index, Memory_Allocator *allocator = null)
{
    return ordered_insert(array, &value, 1, offset_or_max, allocator);
}

INTERNAL void pop(Template_Array_Type *array, Template_Array_Size_Type count = 1, Memory_Allocator *allocator = null)
{
    assert(count <= array->count);
    array->count -= count;
    
    bool do_shrink = false;
    
#if defined Template_Array_Is_Buffer
    
    if (allocator && Template_Array_Shrink_Check(array)) {
        Template_Array_Size_Type new_capaciy = Template_Array_Shrink_Formula(array);
        
        if (new_capaciy != array->capacity) {
            do_shrink = true;
            array->capacity = new_capaciy;
        }
    }
    
#else
    
    assert(allocator);
    do_shrink = true;
    
#endif
    
    if (do_shrink) {
        if (array->capacity) {
            REALLOCATE_ARRAY(allocator, &array->data, array->count);
        }
        else {
            free(allocator, array->data);
            *array = {};
        }
    }
}

INTERNAL void unordered_remove(Template_Array_Type *array, Template_Array_Size_Type offset_or_max = Array_Max_Index, Template_Array_Size_Type count = 1, Memory_Allocator *allocator = null)
{
    if (offset_or_max == Array_Max_Index) {
        assert(count <= array->count);
        offset_or_max = array->count - count;
    }
    
    assert(offset_or_max + count <= array->count);
    COPY(front(*array, offset_or_max), back(*array, count - 1), count * sizeof(Template_Array_Data_Type));
    pop(array, count, allocator);
}

INTERNAL void ordered_remove(Template_Array_Type *array, Template_Array_Size_Type offset_or_max = -1, Template_Array_Size_Type count = 1, Memory_Allocator *allocator = null)
{
    if (offset_or_max == Array_Max_Index) {
        assert(count <= array->count);
        offset_or_max = array->count - count;
    }
    
    assert(offset_or_max + count <= array->count);
    COPY(front(*array, offset_or_max), front(*array, offset_or_max + count), (array->count - offset_or_max - count) * sizeof(Template_Array_Data_Type));
    pop(array, count, allocator);
}






#if defined Template_Array_Is_Ringbuffer


INTERNAL Template_Array_Data_Type * front(Template_Array_Type array, Template_Array_Size_Type offset = 0) {
    assert(offset < array->count);
    return array->data + ((array->head + offset) % array->capacity);
}



#endif










// undefine template defines to enable multiple includes

#undef Template_Array_Type
#undef Template_Array_Data_Type
#undef Template_Array_Size_Type

#if defined Template_Array_Is_Buffer

#  undef Template_Array_Is_Buffer
#  undef Template_Array_Grow_Formula
#  undef Template_Array_Shrink_Check
#  undef Template_Array_Shrink_Formula

#endif

