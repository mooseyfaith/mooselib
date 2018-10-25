
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  template_array.h                                                          //
//                                                                            //
//  Tafil Kajtazi - 19.09.2018                                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "basic.h"

#if !defined Template_Array_Type
#  error Template_Array_Type needs to be defined befor including buffer_template.h
#endif

#if !defined Template_Array_Data_Type
#  error Template_Array_Data_Type needs to be defined befor including buffer_template.h
#endif

#if !defined Template_Array_Size_Type
#  define Template_Array_Size_Type usize
#endif

struct Template_Array_Type {
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
grow(Memory_Allocator *allocator, Template_Array_Type *array, Template_Array_Size_Type capacity = 1)
{
    if (array->capacity)
        REALLOCATE_ARRAY(allocator, &array->data, array->capacity + capacity);
    else
        array->data = ALLOCATE_ARRAY(allocator, Template_Array_Data_Type, capacity);
    
    Template_Array_Data_Type *result = array->data + array->capacity;
    array->capacity += capacity;
    return result;
}

INTERNAL void
shrink(Memory_Allocator *allocator, Template_Array_Type *array, Template_Array_Size_Type capacity = 1)
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
free(Memory_Allocator *allocator, Template_Array_Type *array)
{
    if (array->count) {
        free(allocator, array->data);
        *array = {};
    }
}

INTERNAL usize
byte_count(Template_Array_Type array)
{
    return array.count * sizeof(Template_Array_Data_Type);
}

INTERNAL usize
byte_capacity(Template_Array_Type array)
{
    return array.capacity * sizeof(Template_Array_Data_Type);
}

INTERNAL Template_Array_Data_Type *
one_past_last(Template_Array_Type array)
{
    return array + array.count;
}

INTERNAL usize
try_index_of(Template_Array_Type array, Template_Array_Data_Type *item)
{
    usize index = cast_v(usize, item - array.data);
    return index;
}

INTERNAL Template_Array_Size_Type
index_of(Template_Array_Type array, Template_Array_Data_Type *item)
{
    usize index = try_index_of(array, item);
    assert(index < array.count);
    return cast_v(Template_Array_Size_Type, index);
}

#if defined Template_Array_Is_Buffer

INTERNAL Template_Array_Size_Type
remaining_count(Template_Array_Type buffer)
{
    return buffer.capacity - buffer.count;
}

INTERNAL Template_Array_Size_Type
remaining_byte_count(Template_Array_Type buffer)
{
    return (buffer.capacity - buffer.count) * sizeof(Template_Array_Data_Type);
}

INTERNAL Template_Array_Data_Type *
push(Template_Array_Type *buffer, Template_Array_Size_Type count = 1)
{
    assert(buffer->count + count <= buffer->capacity);
    Template_Array_Data_Type *result = buffer->data + buffer->count;
    buffer->count += count;
    return result;
}

INTERNAL Template_Array_Data_Type *
push(Memory_Allocator *allocator, Template_Array_Type *buffer, Template_Array_Size_Type count = 1)
{
    if (buffer->count + count > buffer->capacity) {
        grow(allocator, buffer, buffer->count + count - buffer->capacity);
    }
    
    return push(buffer, count);
}

INTERNAL Template_Array_Data_Type *
pop(Template_Array_Type *buffer, Template_Array_Size_Type count = 1)
{
    assert(count <= buffer->count);
    Template_Array_Data_Type *result = buffer->data + buffer->count;
    buffer->count -= count;
    return result;
}

#undef Template_Array_Is_Buffer

#else

INTERNAL Template_Array_Data_Type *
advance(Template_Array_Type *iterator, Template_Array_Size_Type count = 1) {
    assert(count <= iterator->count);
    Template_Array_Data_Type *result = iterator->data;
    iterator->data  += count;
    iterator->count -= count;
    return result;
}

#endif

#undef Template_Array_Type
#undef Template_Array_Data_Type
#undef Template_Array_Size_Type
