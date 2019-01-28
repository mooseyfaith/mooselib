////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  u8_buffer.h                                                               //
//                                                                            //
//  Tafil Kajtazi - 19.09.2018                                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#if !defined U8_BUFFER_H
#define U8_BUFFER_H

#define Template_Array_Name      u8_array
#define Template_Array_Data_Type u8
#include "template_array.h"

#define Template_Array_Name      u8_buffer
#define Template_Array_Data_Type u8
#define Template_Array_Is_Buffer
#include "template_array.h"

#define Template_Array_Name      u16_array
#define Template_Array_Data_Type u16
#include "template_array.h"

#define Template_Array_Name      u16_buffer
#define Template_Array_Data_Type u16
#define Template_Array_Is_Buffer
#include "template_array.h"

#define Template_Array_Name      u32_array
#define Template_Array_Data_Type u32
#include "template_array.h"

#define Template_Array_Name      u32_buffer
#define Template_Array_Data_Type u32
#define Template_Array_Is_Buffer
#include "template_array.h"

#define Template_Array_Name      u64_array
#define Template_Array_Data_Type u64
#include "template_array.h"

#define Template_Array_Name      u64_buffer
#define Template_Array_Data_Type u64
#define Template_Array_Is_Buffer
#include "template_array.h"

#define grow_items(allocator, array, type, count) cast_p(type, grow(allocator, array, sizeof(type) * count))
#define grow_item(allocator, array, type)         grow_items(allocator, array, type, 1)

#define shrink_items(allocator, array, type, count) shrink(allocator, array, sizeof(type) * count)
#define shrink_item(allocator, array, type)         shrink_items(allocator, array, type, 1)

#define item_count(buffer, type)     ((buffer).count / sizeof(type))
#define item_at(buffer, type, index) cast_p(type, (buffer).data + sizeof(type) * index)

#define pack_items(buffer, type, count) cast_p(type, push(buffer, sizeof(type) * count))
#define pack_item(buffer, value)        { *pack_items(buffer, decltype(value), 1) = value; }

#define peek_item(buffer, type)           cast_p(type, (buffer).data + (buffer).count - sizeof(type))
#define unpack_items(buffer, type, count) cast_p(type, pop(buffer, sizeof(type) * count))
#define unpack_item(buffer, type)         unpack_items(buffer, type, 1)

#define next_items(iterator, type, count) cast_p(type, advance(iterator, sizeof(type) * count))
#define next_item(iterator, type)         next_items(iterator, type, 1)

#endif // U8_BUFFER_H
