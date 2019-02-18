#if !defined STRING_H
#define STRING_H

//#include "basic.h"

#include "u8_buffer.h"

typedef u8_array string;

#define Template_Array_Name      string_array
#define Template_Array_Data_Type string
#include "template_array.h"

#define Template_Array_Name      string_buffer
#define Template_Array_Data_Type string
#define Template_Array_Is_Buffer
#include "template_array.h"

#define EMPTY_STRING string{}

#define S(c_string_literal_or_char_array) string{ cast_p(u8, const_cast<char *> (c_string_literal_or_char_array)), ARRAY_COUNT(c_string_literal_or_char_array) - 1 }

//#define S(c_string_literal_or_char_array) MAKE_STRING(c_string_literal_or_char_array)

// plaese make shure your string->data is 0-terminated
#define TO_C_STR(string) cast_p(char, (string)->data)

// printf("%.s", FORMAT_S(&string));
#define FORMAT_S(string) cast_v(int, (string)->count), cast_p(char, (string)->data)

inline usize c_string_length(c_const_string text) {
    assert(text);
    auto it = text;
    
    while (*it)
        ++it;
    
    return cast_v(u32, it - text);
}

// use S(..) macro if you pass string literal or char array
inline string make_string(c_const_string text) {
    return { cast_p(u8, const_cast<char *> (text)), c_string_length(text) };
}

inline string write_string(u8 *buffer, c_const_string text, usize debug_buffer_count) {
    auto c_it = text;
    auto it = buffer;
    
    while (*c_it) 
        *(it++) = *(c_it++);
    
    assert(cast_v(u32, c_it - text) <= debug_buffer_count);
    return { buffer, cast_v(usize, c_it - text) };
}

inline void write_c_string(char *destination, u32 destination_capacity, string source)
{
    assert(destination_capacity >= source.count + 1);
    u8 *destination_it = cast_p(u8, destination);
    
    for (u8 *source_it = source.data, *end = one_past_last(source); source_it != end; ++source_it, ++destination_it)
        *destination_it = *source_it;
    
    *destination_it = '\0';
}

inline string make_string(Memory_Allocator *allocator, string other) {
    string buffer = ALLOCATE_ARRAY_INFO(allocator, u8, other.count);
    //copy(&buffer, other);
    copy(buffer.data, other.data, byte_count_of(other));
    
    return buffer;
}

inline string sub_string(string start, string end) {
    assert((start.data <= end.data) && (end.data <= start.data + start.count));
    return { start.data, cast_v(usize, end.data - start.data) };
}

INTERNAL string
left(u8_buffer buffer, usize left_count)
{
    assert(left_count < buffer.count);
    return { buffer.data, left_count };
}

INTERNAL string
right(u8_buffer buffer, usize left_count)
{
    assert(left_count < buffer.count);
    return { one_past_last(buffer) - left_count, buffer.count - left_count };
}

INTERNAL bool operator==(string a, string b) {
    if (a.count != b.count)
        return false;
    
    for (u32 i = 0; i < a.count; ++i) {
        if (a[i] != b[i])
            return false;
    }
    
    return true;
}

INTERNAL bool operator!=(string a, string b) {
    return !(a == b);
    
}

u32 write_utf8(u8 buffer[4], u32 utf32_code)
{
    if (utf32_code <= 0x7F) {
        buffer[0] = utf32_code;
        return 1;
    }
    
    if (utf32_code <= 0x7FF) {
        buffer[0] = 0xC0 | (utf32_code >> 6);            /* 110xxxxx */
        buffer[1] = 0x80 | (utf32_code & 0x3F);          /* 10xxxxxx */
        return 2;
    }
    
    if (utf32_code <= 0xFFFF) {
        buffer[1] = 0xE0 | (utf32_code >> 12);           /* 1110xxxx */
        buffer[2] = 0x80 | ((utf32_code >> 6) & 0x3F);   /* 10xxxxxx */
        buffer[3] = 0x80 | (utf32_code & 0x3F);          /* 10xxxxxx */
        return 3;
    }
    
    if (utf32_code <= 0x10FFFF) {
        buffer[0] = 0xF0 | (utf32_code >> 18);           /* 11110xxx */
        buffer[1] = 0x80 | ((utf32_code >> 12) & 0x3F);  /* 10xxxxxx */
        buffer[2] = 0x80 | ((utf32_code >> 6) & 0x3F);   /* 10xxxxxx */
        buffer[3] = 0x80 | (utf32_code & 0x3F);          /* 10xxxxxx */
        return 4;
    }
    
    assert(0, "invalid utf32 code");
    return 0;
}

void write_utf8(Memory_Allocator *allocator, string *text, u32 utf32_code) {
    u8 buffer[4];
    u32 byte_count = write_utf8(buffer, utf32_code);
    
    auto dest = grow(allocator, text, byte_count);
    copy(dest, buffer, byte_count);
}

u32 utf8_head(string text, OPTIONAL u32 *byte_count = null) {
    assert(text.count);
    
    if (text[0] <= 0x7F) {
        if (byte_count)
            *byte_count = 1;
        
        return text[0];
    }
    
    u8 mask = 0xF0;
    u32 part_count = 3;
    
    while (part_count && ((text[0] & mask) != mask)) {
        mask <<= 1;
        part_count--;
    }
    
    assert(part_count);
    
    if (byte_count)
        *byte_count = part_count + 1;
    
    assert(text.count >= part_count + 1);
    
    u32 result = (text[0] & ~mask) << (6 * part_count);
    u32 index = 1;
    while (part_count) {
        part_count--;
        result |= (text[index] & ~0x80) << (6 * part_count);
        index++;
    } 
    
    return result;
}

u32 utf8_last_char_count(string text) {
    if (!text.count)
        return 0;
    
    usize i = 1;
    
    while ((i <= text.count) && ((text[text.count - i] & 0xC0) == 0x80))
        ++i;
    
    return i;
}

u32 utf8_tail(string text, OPTIONAL u32 *byte_count = null) {
    auto count = utf8_last_char_count(text);
    
    if (byte_count)
        *byte_count = count;
    
    string tail = { one_past_last(text) - count, count };
    return utf8_head(tail);
}

u32 utf8_advance(string *iterator) {
    u32 byte_count;
    u32 result = utf8_head(*iterator, &byte_count);
    advance(iterator, byte_count);
    return result;
}

usize utf8_count(string text) {
    usize count = 0;
    while (text.count) {
        utf8_advance(&text);
        count++;
    }
    
    return count;
}

// parsing

inline bool is_in_range(u32 token, u32 from, u32 to) {
    return (from <= token) && (token <= to);
}

inline bool is_letter(u32 token) {
    return is_in_range(token, 'a', 'z') || is_in_range(token, 'A', 'Z');
}

inline bool is_digit(u32 token) {
    return is_in_range(token, '0', '9');
}

bool is_contained(u32 token, string set)
{
    auto it = set;
    
    while (it.count) {
        u32 set_token = utf8_advance(&it);
        
        if (token == set_token)
            return true;
    }
    
    return false;
}

string get_identifier(string *it, string letter_extenstions) {
    if (!it->count)
        return {};
    
    auto start = *it;
    
    u32 byte_count;
    u32 token = utf8_head(*it, &byte_count);
    
    if ((!is_letter(token) && !is_contained(token, letter_extenstions)))
        return {};
    
    advance(it, byte_count);
    
    while (it->count) {
        u32 token = utf8_head(*it, &byte_count);
        
        if (!is_letter(token) && !is_digit(token) && !is_contained(token, letter_extenstions))
            break;
        
        advance(it, byte_count);
    }
    
    return sub_string(start, *it);
}

string skip_set(string *it, string set) {
    auto start = *it;
    
    while (it->count) {
        u32 byte_count;
        u32 token = utf8_head(*it, &byte_count);
        
        if (!is_contained(token, set))
            break;
        
        advance(it, byte_count);
    }
    
    return sub_string(start, *it);
}

string skip_range(string *it, u8 from, u8 to) {
    auto start = *it;
    
    while (it->count) {
        u32 byte_count;
        u32 token = utf8_head(*it, &byte_count);
        
        if (!is_in_range(token, from, to))
            break;
        
        advance(it, byte_count);
    }
    
    return sub_string(start, *it);
}

string get_token_until_first_in_set(string text, string set, bool *found, bool include_end = true)
{
    auto it = text;
    
    *found = false;
    while (it.count) {
        u32 byte_count;
        u32 token = utf8_head(it, &byte_count);
        
        if (is_contained(token, set)) {
            *found = true;
            break;
        }
        
        advance(&it, byte_count);
    }
    
    if (include_end || *found)
        return sub_string(text, it);
    
    return {};
}

string skip_until_first_in_set(string *it, string set, bool do_skip_set = false, bool include_end = true)
{
    bool found;
    string token = get_token_until_first_in_set(*it, set, &found, include_end);
    if (found || include_end) {
        advance(it, token.count);
        
        if (found && do_skip_set)
            skip_set(it, set);
    }
    
    return token;
}

bool starts_with(string text, string prefix) {
    if (text.count < prefix.count)
        return false;
    
    u8 *head = text.data;
    u8 *prefix_it = prefix.data;
    u8 *prefix_end = one_past_last(prefix);
    
    while (prefix_it != prefix_end) {
        if (*(head++) != *(prefix_it++))
            return false;
    }
    
    return true;
}

bool try_skip(string *it, string prefix) {
    if (starts_with(*it, prefix)) {
        advance(it, prefix.count);
        return true;
    }
    
    return false;
}

inline void skip(string *it, string prefix) {
    bool skipped = try_skip(it, prefix);
    assert(skipped);
}

string get_token_until_first(string text, string pattern, bool include_end = true) {
    string it = text;
    
    bool found = false;
    while (it.count) {
        if (starts_with(it, pattern)) {
            found = true;
            break;
        }
        
        utf8_advance(&it);
    }
    
    if (include_end || found)
        return sub_string(text, it);
    
    return {};
}

string skip_until_first(string *it, string pattern, bool skip_pattern = false, bool include_end = true) {
    string token = get_token_until_first(*it, pattern, include_end);
    if (token.count)
        advance(it, token.count);
    
    if (it->count && skip_pattern)
        skip(it, pattern);
    
    return token;
}

// parsing

inline string try_parse_quoted_string(string *iterator, bool *ok, u32 quot_symbol = '\"', u32 escape_symbol = '\\') {
    *ok = false;
    
    auto start = *iterator;
    
    {
        u32 byte_count;
        if (utf8_head(*iterator, &byte_count) != quot_symbol)
            return {};
        advance(iterator, byte_count);
    }
    
    bool was_escaping = false;
    
    while (iterator->count) {
        u32 token_byte_count;
        u32 token = utf8_head(*iterator, &token_byte_count);
        was_escaping = (token == escape_symbol);
        
        if (!was_escaping && (token == quot_symbol)) {
            *ok = true;
            auto text = sub_string(start, *iterator);
            advance(&text, token_byte_count);
            advance(iterator, token_byte_count);
            
            return text;
        }
        
        advance(iterator, token_byte_count);
    }
    
    *iterator = start;
    return {};
}

inline string parse_quoted_string(string *it, u32 quot_symbol = '\"', u32 escape_symbol = '\\') {
    bool ok;
    auto result = try_parse_quoted_string(it, &ok, quot_symbol, escape_symbol);
    assert(ok);
    
    return result;
}

#define PARSE_UNSIGNED_INTEGER(it, bits, ...) cast_v(CHAIN(u, bits), parse_unsigned_integer(it, bits, ##__VA_ARGS__))
#define PARSE_SIGNED_INTEGER(it, bits, ...) cast_v(CHAIN(s, bits), parse_signed_integer(it, bits, ##__VA_ARGS__))

#define TRY_PARSE_UNSIGNED_INTEGER(it, ok, bits, ...) cast_v(CHAIN(u, bits), try_parse_unsigned_integer(it, ok, bits, ##__VA_ARGS__))
#define TRY_PARSE_SIGNED_INTEGER(it, ok, bits, ...) cast_v(CHAIN(s, bits), try_parse_signed_integer(it, ok, bits, ##__VA_ARGS__))

inline u8 try_get_digit(u8 c, bool *ok, u8 base = 10) {
    u8 digit;
    if (c >= 'a')
        digit = c - 'a' + 10;
    else if (c >= 'A')
        digit = c + 'A' + 10;
    else
        digit = c - '0';
    
    *ok = (digit < base);
    
    return digit;
}

inline u64 try_parse_unsigned_integer(string *it, bool *ok, u8 bits = 64, u8 base = 10, u32 *optional_digit_count = NULL) {
    assert(bits && bits <= 64);
    
    if (!it->count) {
        *ok = false;
        return 0;
    }
    
    auto head = *it;
    
    // would be nice, but vs does something i cant explain, it evaluates to 0 ..
    // if bits == 64 => 1 << 64 == 0 => 0 - 1 is highest value, should work
    u64 max_value;
    if (bits < 64)
        max_value = (cast_v(u64, 1) << bits) - 1;
    else
        max_value = ~cast_v(u64, 0);
    
    u64 value = try_get_digit(head[0], ok, base);
    advance(&head);
    
    if (!*ok)
        return 0;	
    
    while (head.count) {
        bool digit_okay;
        u8 digit = try_get_digit(head[0], &digit_okay, base);
        
        if (!digit_okay)
            break;
        
        u64 new_value = value * base + digit;
        
        if ((new_value > max_value) || (value > new_value)) {
            *ok = false;
            return -1;
        }
        
        value = new_value;
        advance(&head);
    }
    
    if (optional_digit_count)
        *optional_digit_count = it->count - head.count;
    
    *it = head;
    *ok = true;
    
    return value;
}

inline u64 parse_unsigned_integer(string *it, u8 bits = 64, u8 base = 10, u32 *optional_digit_count = NULL) {
    bool ok;
    u64 value = try_parse_unsigned_integer(it, &ok, bits, base, optional_digit_count);
    assert(ok);
    return value;
}

inline s64 try_parse_signed_integer(string *it, bool *ok, u8 bits = 64, u32 *optional_digit_count = NULL) {
    assert(bits && bits <= 64);
    
    string head = *it;
    
    bool is_negative;
    if (head.count && (head[0] == '-')) {
        is_negative = true;
        advance(&head);
    }
    else {
        if (head.count && (head[0] == '+')) // may start with a plus
            advance(&head);
        
        is_negative = false;
    }
    
    u64 value = try_parse_unsigned_integer(&head, ok, bits, 10, optional_digit_count);
    
    if (!*ok)
        return cast_v(s64, value);
    
    if (is_negative) {
        if (value > (cast_v(u64, 1) << (bits - 1))) {
            *ok = false;
            return -1;
        }
        
        *it = head;
        
        return -cast_v(s64, value);
    }
    else {
        if (value >= (cast_v(u64, 1) << (bits - 1))) {
            *ok = false;
            return -1;
        }
        
        *it = head;
        
        return cast_v(s64, value);
    }
}

inline s64 parse_signed_integer(string *it, u8 bits = 64, u32 *optional_digit_count = NULL) {
    bool ok;
    s64 value = try_parse_signed_integer(it, &ok, bits, optional_digit_count);
    assert(ok);
    return value;
}

inline f64 try_parse_f64(string *it, bool *ok) {
    f64 result;
    
    string test_it = *it;
    
    bool is_negative;
    if (test_it.count && (test_it.data[0] == '-')) {
        is_negative = true;
        advance(&test_it);
    }
    else {
        is_negative = false;
    }
    
    u64 hole_part = try_parse_unsigned_integer(&test_it, ok, 64);
    if (!*ok)
        return 0.0;
    
    result = cast_v(f64, hole_part);
    
    if (test_it.data[0] == '.') {
        advance(&test_it);
        u32 fractional_part_digit_count;
        u64 fractional_part = try_parse_unsigned_integer(&test_it, ok, 64, 10, &fractional_part_digit_count);
        if (!*ok)
            return 0.0;
        
        f64 fraction = cast_v(f64, fractional_part);		
        
        while (fractional_part_digit_count) {
            fraction /= 10.0;
            --fractional_part_digit_count;
        }
        
        result += fraction;
    }
    
    if (test_it.data[0] == 'e') {
        s64 exponent_part = try_parse_signed_integer(&test_it, ok, 64);
        if (!*ok)
            return 0.0;
        
        if (exponent_part > 0) {
            while (exponent_part) {
                result *= 10.0;
                --exponent_part;
            }
        } else {
            while (exponent_part) {
                result /= 10.0;
                ++exponent_part;
            }
        }
    }
    
    *it = test_it;
    
    if (is_negative)
        return -result;
    else
        return result;
}

inline f64 parse_f64(string *it) {
    bool ok;
    f64 value = try_parse_f64(it, &ok);
    assert(ok);
    return value;
}

// writing

#define STRING_WRITE_DEC(name) void name(Memory_Allocator *allocator, string *output, va_list *va_args)
typedef STRING_WRITE_DEC((*String_Write_Function));


string write_va(Memory_Allocator *allocator, string *output, string *format_it, va_list va_args, bool *is_escaping, bool double_capacity_on_grow = true)
{
    auto old_count = output->count;
    
    while (format_it->count)
    {
        if (*is_escaping)
        {
            *grow(allocator, output) = (*format_it)[0];
            advance(format_it);
            *is_escaping = false;
            
            continue;
        }
        
        if ((*format_it)[0] == '\\')
        {
            *is_escaping = true;
            advance(format_it);
            
            continue;
        }
        
        if ((*format_it)[0] == '%')
        {
            va_list peek_args = va_args;
            auto string_write = *va_arg(peek_args, String_Write_Function*);
            
            // if you crash here, you probably haven't wrapped your arguments with f(..)
            // we are trying to call a function, wich will be returned by f(..)
            string_write(allocator, output, &va_args);
        }
        else
        {
            *grow(allocator, output) = (*format_it)[0];
        }
        
        advance(format_it);
    }
    
    return { output->data + old_count, output->count - old_count };
}

INTERNAL string write_va(Memory_Allocator *allocator, string *output, string format, va_list va_args, bool double_capacity_on_grow = true) 
{
    bool is_escaping = false;
    auto text = write_va(allocator, output, &format, va_args, &is_escaping, double_capacity_on_grow);
    return text;
}

struct String_Buffer {
    Memory_Allocator *allocator;
    string buffer;
};

INTERNAL string write_va(String_Buffer *buffer, string format, va_list va_args, bool double_capacity_on_grow = true) 
{
    bool is_escaping = false;
    auto text = write_va(buffer->allocator, &buffer->buffer, &format, va_args, &is_escaping, double_capacity_on_grow);
    return text;
}

INTERNAL String_Buffer new_write_buffer_va(Memory_Allocator *allocator, string format, va_list va_args, bool double_capacity_on_grow = true) 
{
    String_Buffer buffer = { allocator };
    auto text = write_va(&buffer, format, va_args, double_capacity_on_grow);
    return buffer;
}

INTERNAL String_Buffer new_write_buffer(Memory_Allocator *allocator, string format, ...)
{
    va_list va_args;
    va_start(va_args, format);
    auto result = new_write_buffer_va(allocator, format, va_args, false);
    va_end(va_args);
    return result;
}

INTERNAL void free(String_Buffer *buffer) {
    free_array(buffer->allocator, &buffer->buffer);
}

INTERNAL string write(Memory_Allocator *allocator, string *output, string format, ...) {
    va_list va_args;
    va_start(va_args, format);
    bool is_escaping = false;
    string text = write_va(allocator, output, &format, va_args, &is_escaping, false);
    va_end(va_args);
    
    return text;
}

INTERNAL string new_write(Memory_Allocator *allocator, string format, ...) {
    va_list va_args;
    va_start(va_args, format);
    bool is_escaping = false;
    string output = {};
    string text = write_va(allocator, &output, &format, va_args, &is_escaping, false);
    va_end(va_args);
    
    return text;
}

INTERNAL string write(String_Buffer *buffer, string format, ...) {
    va_list va_args;
    va_start(va_args, format);
    string text = write_va(buffer, format, va_args, false);
    va_end(va_args);
    
    return text;
}

struct Format_Info_integer64 {
    String_Write_Function write;
    union { u64 uvalue; s64 svalue; };
    bool is_signed;
    u32 max_digit_count;
    u8 padding_symbol;
    u8 base;
    u8 first_symbol_after_9;
};

u32 get_digit_count(u64 value, u64 base) {
    u32 result = 0;
    while (value) {
        value /= base;
        ++result;
    }
    
    // if value is 0, reserve at least 1 digit
    if (!result)
        result = 1;
    
    return result;
}

STRING_WRITE_DEC(write_integer64)
{
    auto format_info = &va_arg(*va_args, Format_Info_integer64);
    
    u64 value;
    if (format_info->is_signed) {
        
        if (format_info->svalue < 0) {
            *grow(allocator, output) = '-';
            value = -format_info->svalue;
        }
        else
            value = format_info->svalue;
    }
    else {
        value = format_info->uvalue;
    }
    
    u32 digit_count = get_digit_count( value, format_info->base);
    
    assert(!format_info->max_digit_count || (digit_count <= format_info->max_digit_count));
    
    u32 max_digit_count;
    if (format_info->max_digit_count)
        max_digit_count = format_info->max_digit_count;
    else
        max_digit_count = digit_count;
    
    // writing from end to begin :D
    u32 end = max_digit_count;
    auto it = grow(allocator, output, max_digit_count);
    
    while (value) {
        u8 digit = value % format_info->base;
        value /= format_info->base;
        
        if (digit < 10)
            it[end - 1] = '0' + digit;
        else
            it[end - 1] = format_info->first_symbol_after_9 + (digit - 10);
        
        --end;
    }
    
    // if value is 0, write at least 0
    if (!format_info->uvalue) {
        it[end - 1] = '0';
        --end;
    }
    
    while (end) {
        it[end - 1] = format_info->padding_symbol;
        --end;
    }
}

inline Format_Info_integer64 f(u64 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return { write_integer64, value, false, max_digit_count, padding_symbol, base, first_symbol_after_9 };
}

inline Format_Info_integer64 f(u32 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return f(cast_v(u64, value), max_digit_count, padding_symbol, base, first_symbol_after_9);
}

inline Format_Info_integer64 f(u16 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return f(cast_v(u64, value), max_digit_count, padding_symbol, base, first_symbol_after_9);
}

inline Format_Info_integer64 f(u8 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return f(cast_v(u64, value), max_digit_count, padding_symbol, base, first_symbol_after_9);
}

inline Format_Info_integer64 f(s64 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A')
{
    Format_Info_integer64 result;
    result.write = write_integer64;
    result.svalue = value;
    result.is_signed = true;
    result.max_digit_count = max_digit_count;
    result.padding_symbol = padding_symbol;
    result.base = base;
    result.first_symbol_after_9 = first_symbol_after_9;
    return result;
}

inline Format_Info_integer64 f(s32 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return f(cast_v(s64, value), max_digit_count, padding_symbol, base, first_symbol_after_9);
}


inline Format_Info_integer64 f(s16 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return f(cast_v(s64, value), max_digit_count, padding_symbol, base, first_symbol_after_9);
}

inline Format_Info_integer64 f(s8 value, u32 max_digit_count = 0, u8 padding_symbol = '0', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return f(cast_v(s64, value), max_digit_count, padding_symbol, base, first_symbol_after_9);
}

struct Format_Info_string {
    String_Write_Function write;
    string text;
    u32 prepend_symbol_count;
    u8 prepend_symbol;
};

STRING_WRITE_DEC(write_string_info)
{
    auto format_info = &va_arg(*va_args, Format_Info_string);
    
    auto byte_count = format_info->prepend_symbol_count + format_info->text.count;
    
    if (!byte_count)
        return;
    
    auto it = grow(allocator, output, byte_count);
    reset(it, format_info->prepend_symbol_count, format_info->prepend_symbol);
    copy(it + format_info->prepend_symbol_count, format_info->text.data, format_info->text.count);
}

inline Format_Info_string f_indent(u32 count, u8 symbol = ' ') {
    return { write_string_info, {}, count, symbol };
    
}

inline Format_Info_string f(string text, u32 prepend_symbol_count = 0, u8 prepend_symbol = ' ') {
    return { write_string_info, text, prepend_symbol_count, prepend_symbol };
}

struct Format_Info_f64 {
    String_Write_Function write;
    union {
        f64 value;
        
        u64 encoding;
        
        struct {
            u64 mantisa  : 52;
            u64 exponent : 11;
            u64 sign     :  1;
        };
    };
    
    u32 max_digit_count;
    u32 max_fraction_digit_count;
    u8 fraction_symbol;
    u8 base;
    u8 first_symbol_after_9;
};

STRING_WRITE_DEC(write_f64)
{
    auto format_info = &va_arg(*va_args, Format_Info_f64);
    
    if (format_info->sign)
        *grow(allocator, output) = '-';
    
    if (format_info->exponent == 0)
    {
        if (format_info->mantisa == 0)
        {
            auto it = grow(allocator, output, 3);
            
            it[0] = '0';
            it[1] = format_info->fraction_symbol;
            it[2] = '0';
            return;
        }
        
        // subnormals are not implemented yet
        UNREACHABLE_CODE;
    }
    
    if (format_info->exponent == ((1 << 11) - 1))
    {
        string text;
        if (format_info->mantisa == 0)
            text = S("infinity");
        else 
            text = S("not-a-number");
        
        auto it = grow(allocator, output, text.count);
        copy(it, text.data, text.count);
        return;
    }
    
    u64 value = (u64(1) << 52) | format_info->mantisa;
    
    u64 x = value;
    s64 exponent = format_info->exponent - 1023 - 52;
    s64 base_exponent = 0;
    
    if (exponent > 0) {
        while (exponent) {
            u64 next_x = x << 1;
            while ((next_x >> 1) != x) {
                u64 y = ((x % format_info->base) << 1) / format_info->base;
                x /= format_info->base;
                x |= y;
                
                ++base_exponent;
            }
            
            x <<= 1;
            --exponent;
        }
    }
    else {
        while (exponent) {
            while (x & 1) {
                u64 next_x = x * format_info->base;
                if ((next_x / format_info->base) == x) {
                    x = next_x;
                    --base_exponent;
                }
                else {
                    break;
                }
            }
            
            x >>= 1;
            ++exponent;
        }
    }
    
    u64 digit_count = get_digit_count(x, format_info->base);
    
    s64 whole_digit_count = digit_count + base_exponent;
    s64 fraction_digit_count = digit_count - whole_digit_count;
    
    // print with exponent
    if (format_info->max_digit_count && (whole_digit_count > format_info->max_digit_count)) {
        assert(0);
    }
    
    u64 whole_value = x;
    {
        s64 i = base_exponent;
        while (i < 0) {
            whole_value /= format_info->base;
            ++i;
        }
        
        //auto whole_info = f(whole_value, 0, '0', format_info->base, format_info->first_symbol_after_9);
        //write_formatted_64(it, &whole_info);
        
        write(allocator, output, S("%"), f(whole_value, 0, '0', format_info->base, format_info->first_symbol_after_9));
    }
    
    *grow(allocator, output) = format_info->fraction_symbol;
    
    u64 fraction_value = whole_value;
    s64 i = fraction_digit_count;
    while (i) {
        fraction_value *= format_info->base;
        --i;
    }
    
    fraction_value = x - fraction_value;
    
    fraction_digit_count = MAX(fraction_digit_count, 1);
    
    if (format_info->max_digit_count)
        fraction_digit_count = MIN(fraction_digit_count, format_info->max_digit_count - whole_digit_count);
    
    if (format_info->max_fraction_digit_count)
        fraction_digit_count = MIN(fraction_digit_count, format_info->max_fraction_digit_count);
    
    while (-base_exponent > fraction_digit_count) {
        fraction_value /= format_info->base;
        ++base_exponent;
    }
    
    //auto fraction_info = f(fraction_value, -base_exponent, '0', format_info->base, format_info->first_symbol_after_9);
    //write_formatted_64(it, &fraction_info);
    write(allocator, output, S("%"), f(fraction_value, -base_exponent, '0', format_info->base, format_info->first_symbol_after_9));
}

inline Format_Info_f64 f(f64 value, u32 max_fraction_digit_count = 4, u8 fraction_symbol = '.', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    Format_Info_f64 result;
    result.write = write_f64;
    result.value = value;
    result.max_digit_count = 0;
    result.max_fraction_digit_count = max_fraction_digit_count;
    result.fraction_symbol = fraction_symbol;
    result.base = base;
    result.first_symbol_after_9 = first_symbol_after_9;
    return result;
}

inline Format_Info_f64 f(f32 value, u32 max_fraction_digit_count = 4, u8 fraction_symbol = '.', u8 base = 10, u8 first_symbol_after_9 = 'A') {
    return f(cast_v(f64, value), max_fraction_digit_count, fraction_symbol, base, first_symbol_after_9);
}

#endif
