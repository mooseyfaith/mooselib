#if !defined MD5_H
#define MD5_H

struct MD5_Value {
    union {
        struct { u32 a, b, c, d; };
        u32 abcd[4];
        u8 bytes[16];
    };
    
    u8 buffer[64];
    u32 buffer_count;
    
    u64 byte_count;
};

u32 md5_k[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

INTERNAL u32 rotate_left(u32 value, u32 index) {
    return (value << index) | (value >> (32 - index));
}

INTERNAL void md5_advance(MD5_Value *value, u32 f, u32 left_rotation) {
    u32 old_d = value->d;
    value->d = value->c;
    value->c = value->b;
    value->b = value->b + rotate_left(value->a + f, left_rotation);
    value->a = old_d;
}

INTERNAL u32 swap_endian(u32 value) {
    return (value << 24) | ((value << 8) & 0x00ff0000) | ((value >> 8) & 0x0000ff00) | ((value >> 24));
}

INTERNAL void md5_message(MD5_Value *value, u32 m[16])
{
    MD5_Value message_value = *value;
    
    u32 *k = md5_k;
    
    for (u32 i = 0; i < 16; ++i) {
        u32 f = (message_value.b & message_value.c) | ((~message_value.b) & message_value.d);
        u32 g = i;
        
        u32 left_rotation[] = {  7, 12, 17, 22 };
        md5_advance(&message_value, f + *(k++) + m[g], left_rotation[i % 4]);
    }
    
    for (u32 i = 0; i < 16; ++i) {
        u32 f = (message_value.d & message_value.b) | ((~message_value.d) & message_value.c);
        u32 g = MOD(5 * i + 1, 16);
        
        u32 left_rotation[] = {  5,  9, 14, 20 };
        md5_advance(&message_value, f + *(k++) + m[g], left_rotation[i % 4]);
    }
    
    for (u32 i = 0; i < 16; ++i) {
        u32 f = message_value.b ^ message_value.c ^ message_value.d;
        u32 g = MOD(3 * i + 5, 16);
        
        u32 left_rotation[] = {  4, 11, 16, 23 };
        md5_advance(&message_value, f + *(k++) + m[g], left_rotation[i % 4]);
    }
    
    for (u32 i = 0; i < 16; ++i) {
        u32 f = message_value.c ^ (message_value.b | (~message_value.d));
        u32 g = MOD(7 * i, 16);
        
        u32 left_rotation[] = {  6, 10, 15, 21 };
        md5_advance(&message_value, f + *(k++) + m[g], left_rotation[i % 4]);
    }
    
    for (u32 i = 0; i < 4; ++i)
        value->abcd[i] += message_value.abcd[i];
}

MD5_Value md5_begin() {
    MD5_Value result = {
        0x67452301,
        0xefcdab89,
        0x98badcfe,
        0x10325476,
    };
    
    result.byte_count = 0;
    result.buffer_count = 0;
    
    return result;
}

void md5_advance(MD5_Value *value, u8 *data, usize data_count, bool is_last_chunk = false) {
    value->byte_count += data_count;
    
    if (value->buffer_count) {
        u32 copy_count = MIN(64 - value->buffer_count, data_count);
        copy(value->buffer + value->buffer_count, data, copy_count);
        
        value->buffer_count += copy_count;
        data                += copy_count;
        data_count          -= copy_count;
        
        if (value->buffer_count == 64) {
            md5_message(value, cast_p(u32, value->buffer));
            value->buffer_count = 0;
        }
        else if ((data_count == 0) && is_last_chunk) {
            data = value->buffer;
            data_count = value->buffer_count;
        }
    }
    
    while (data_count >= 64) {
        md5_message(value, cast_p(u32, data));
        data       += 64;
        data_count -= 64;
    }
    
    if (is_last_chunk) {
        u8 last_chunk[64] = {};
        
        if (data_count)
            COPY(last_chunk, data, data_count);
        
        // add '1' bit to stream
        last_chunk[data_count] = (1 << 7);
        
        if (data_count + 1 + sizeof(u64) > 64) {
            md5_message(value, cast_p(u32, last_chunk));
            reset(last_chunk, 64);
        }
        
        *cast_p(u64, last_chunk + 64 - sizeof(u64)) = value->byte_count * 8;
        
        md5_message(value, cast_p(u32, last_chunk));
    }
    else if (data_count) {
        copy(value->buffer, data, data_count);
        value->buffer_count = data_count;
    }
}

MD5_Value md5(u8 *data, usize data_count) {
    MD5_Value result = {
        0x67452301,
        0xefcdab89,
        0x98badcfe,
        0x10325476,
    };
    
    union Chunk {
        u8 data[64];
        u32 m[16];
    };
    
    for (Chunk *chunk = cast_p(Chunk, data), *one_past_last_chunk = cast_p(Chunk, data + data_count);
         chunk + 1 <= one_past_last_chunk;
         ++chunk)
    {
        md5_message(&result, chunk->m);
    }
    
    {
        Chunk last_chunk = {};
        
        u32 offset = data_count % sizeof(Chunk);
        if (offset)
            COPY(last_chunk.data, data + data_count - offset, offset);
        
        // add '1' bit to stream
        last_chunk.data[offset] = (1 << 7);
        
        if (offset + 1 + sizeof(u64) > sizeof(Chunk)) {
            md5_message(&result, last_chunk.m);
            last_chunk = {};
        }
        
        *cast_p(u64, last_chunk.data + sizeof(last_chunk) - sizeof(u64)) = cast_v(u64, data_count) * 8;
        
        md5_message(&result, last_chunk.m);
    }
    
    return result;
}

string make_md5_string(Memory_Allocator *allocator, MD5_Value value, u8 first_symbol_after_9 = 'a') {
    string result = write(C_Allocator, S("%%%%"),
                          f(swap_endian(value.abcd[0]), 8, '0', 16, first_symbol_after_9),
                          f(swap_endian(value.abcd[1]), 8, '0', 16, first_symbol_after_9),
                          f(swap_endian(value.abcd[2]), 8, '0', 16, first_symbol_after_9),
                          f(swap_endian(value.abcd[3]), 8, '0', 16, first_symbol_after_9));
    
    return result;
}

#endif // MD5_H