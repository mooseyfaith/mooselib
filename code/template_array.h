
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  template_array.h                                                          //
//                                                                            //
//  Tafil Kajtazi - 19.09.2018                                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "basic.h"

#if !defined Template_Array_Name
#  error Template_Array_Name needs to be defined befor including buffer_template.h
#endif

#if !defined Template_Array_Data_Type
#  error Template_Array_Data_Type needs to be defined befor including buffer_template.h
#endif

#if !defined Template_Array_Size_Type
#  define Template_Array_Size_Type usize
#endif

struct Template_Array_Name {
    Template_Array_Data_Type *data;
    
#if defined Template_Array_Is_Buffer
    Template_Array_Size_Type capacity, count;
#else
    union { Template_Array_Size_Type capacity, count; };
#endif
    
    inline Template_Array_Data_Type &
        operator[](Template_Array_Size_Type offset)
    {
        return *(data + offset);
    }
    
    inline Template_Array_Data_Type
        operator[](Template_Array_Size_Type offset) const
    {
        return *(data + offset);
    }
    
    inline operator Template_Array_Data_Type * ()
    {
        return data;
    }
    
    inline operator Template_Array_Data_Type const * () const
    {
        return data;
    }
};

#define for_array_index(index, array) \
for(decltype((array).count) index = 0; index < (array).count; index++)

#define for_array_item(iterator, array) \
for(auto iterator = (array) + 0; iterator != one_past_last(array); iterator++)

INTERNAL Template_Array_Data_Type *
grow(Memory_Allocator *allocator, Template_Array_Name *array, Template_Array_Size_Type capacity = 1)
{
    if (array->capacity)
        REALLOCATE_ARRAY(allocator, &array->data, array->capacity + capacity);
    else
        array->data = ALLOCATE_ARRAY(allocator, Template_Array_Data_Type, capacity);
    
    Template_Array_Data_Type *result = array->data + array->capacity;
    array->capacity += capacity;
    return result;
}

// how to prevent or warn bad usage of grow/shrink/push/pop ??
#if 0
struct CHAIN(Template_Array_Name, _Reference)  {
    Template_Array_Data_Type **array_data;
    Template_Array_Size_Type offset;
    
    inline Template_Array_Data_Type & operator[](Template_Array_Size_Type index) {
        return cast_p(Template_Array_Size_Type *, (*array_data) + index);
    }
    
    inline Template_Array_Data_Type operator[](Template_Array_Size_Type index) const {
        return cast_p(Template_Array_Size_Type *, (*array_data) + index);
    }
};

#define ARRAY_BEGIN_GROW {
    
#define ARRAY_GROW(items, allocator, array, capacity) } { \
    decltype(array->data) items = grow(allocator, array, capacity);
    
#define ARRAY_GROW_END }
#endif

INTERNAL void
shrink(Memory_Allocator *allocator, Template_Array_Name *array, Template_Array_Size_Type capacity = 1)
{
    assert(capacity <= array->capacity);
    
    if (array->capacity == capacity)
        free(allocator, array->data);
    else 
        REALLOCATE_ARRAY(allocator, &array->data, array->capacity - capacity);
    
    array->capacity -= capacity;
    
#if defined Template_Array_Is_Buffer
    if (array->count > array->capacity)
        array->count = array->capacity;
#endif
}

INTERNAL void
free(Memory_Allocator *allocator, Template_Array_Name *array)
{
    if (array->count) {
        free(allocator, array->data);
        *array = {};
    }
}

INTERNAL usize
byte_count_of(Template_Array_Name array)
{
    return array.count * sizeof(Template_Array_Data_Type);
}

INTERNAL usize
byte_capacity(Template_Array_Name array)
{
    return array.capacity * sizeof(Template_Array_Data_Type);
}

INTERNAL Template_Array_Data_Type *
one_past_last(Template_Array_Name array)
{
    return array + array.count;
}

INTERNAL usize
try_index_of(Template_Array_Name array, Template_Array_Data_Type *item)
{
    usize index = cast_v(usize, item - array.data);
    return index;
}

INTERNAL Template_Array_Size_Type
index_of(Template_Array_Name array, Template_Array_Data_Type *item)
{
    usize index = try_index_of(array, item);
    assert(index < array.count);
    return cast_v(Template_Array_Size_Type, index);
}

#if defined Template_Array_Is_Buffer

INTERNAL Template_Array_Size_Type
remaining_count(Template_Array_Name buffer)
{
    return buffer.capacity - buffer.count;
}

INTERNAL Template_Array_Size_Type
remaining_byte_count_of(Template_Array_Name buffer)
{
    return (buffer.capacity - buffer.count) * sizeof(Template_Array_Data_Type);
}

INTERNAL Template_Array_Data_Type *
push(Template_Array_Name *buffer, Template_Array_Size_Type count = 1)
{
    assert(buffer->count + count <= buffer->capacity);
    Template_Array_Data_Type *result = buffer->data + buffer->count;
    buffer->count += count;
    return result;
}

INTERNAL Template_Array_Data_Type *
push(Memory_Allocator *allocator, Template_Array_Name *buffer, Template_Array_Size_Type count = 1, bool double_capacity_on_grow = true)
{
    if (buffer->count + count > buffer->capacity)
    {
        Template_Array_Size_Type grow_count;
        if (double_capacity_on_grow)
            grow_count = (buffer->count + count) * 2 - buffer->capacity;
        else
            grow_count =  (buffer->count + count) - buffer->capacity;
        
        grow(allocator, buffer, grow_count);
    }
    
    return push(buffer, count);
}

INTERNAL Template_Array_Data_Type *
pop(Template_Array_Name *buffer, Template_Array_Size_Type count = 1)
{
    assert(count <= buffer->count);
    Template_Array_Data_Type *result = buffer->data + buffer->count;
    buffer->count -= count;
    
    return result;
}


INTERNAL void
pop(Memory_Allocator *allocator, Template_Array_Name *buffer, Template_Array_Size_Type count = 1, bool half_capacity_on_quarter_count = true)
{
    pop(buffer, count);
    
    if (half_capacity_on_quarter_count)
    {
        if (buffer->count < (buffer->capacity >> 2))
            shrink(allocator, buffer, buffer->capacity >> 1);
    }
    else
        shrink(allocator, buffer, buffer->capacity - buffer->count);
}

#undef Template_Array_Is_Buffer

#else

INTERNAL Template_Array_Data_Type *
advance(Template_Array_Name *iterator, Template_Array_Size_Type count = 1) {
    assert(count <= iterator->count);
    Template_Array_Data_Type *result = iterator->data;
    iterator->data  += count;
    iterator->count -= count;
    return result;
}

#endif

#undef Template_Array_Name
#undef Template_Array_Data_Type
#undef Template_Array_Size_Type
