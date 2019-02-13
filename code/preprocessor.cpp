
#include "win32_platform.h"
#include "memory_c_allocator.h"
#include "memory_growing_stack.h"

auto White_Space = S(" \t\r\n\0");
void skip_space(string *iterator) {
    skip_set(iterator, White_Space);
}

struct Type {
    string identifier;
    u32 byte_count;
};

#define Template_Array_Name      Types
#define Template_Array_Data_Type Type
#include "template_array.h"

struct Declaration {
    Type type;
    string identifier;
};

#define Template_Array_Name      Declarations
#define Template_Array_Data_Type Declaration
#include "template_array.h"

struct Coroutine {
    string identifier;
    Declarations arguments;
    Types return_types;
};

#define Template_Array_Name      Coroutines
#define Template_Array_Data_Type Coroutine
#include "template_array.h"

Type * find_type(Types types, string identifier) {
    for_array_item(type, types) {
        if (identifier == type->identifier)
            return type;
    }
    
    return null;
}

Type parse_type(string *it, Types types) {
    string start = *it;
    
    Type *base_type = null;
    for_array_item(type, types) {
        if (try_skip(it, type->identifier)) {
            base_type = type;;
            break;
        }
    }
    
    if (!base_type)
        return {};
    
    auto byte_count = base_type->byte_count;
    while (it->count) {
        skip_space(it);
        
        if (!try_skip(it, S("*")))
            break;
        
        byte_count = sizeof(u8*);
    }
    
    return { sub_string(start, *it), byte_count };
}

// does not care for comments or #if's
string skip_scope(string *it, string open_bracket, string closed_bracket) {
    if (!try_skip(it, open_bracket))
        return {};
    
    auto start = *it;
    
    u32 count = 1;
    
    while (count) {
        auto closed = get_token_until_first_in_set(*it, closed_bracket);
        auto opened = get_token_until_first_in_set(*it, open_bracket);
        
        if (!opened.count || (closed.count < opened.count)) {
            assert(closed.count);
            count--;
            advance(it, closed.count);
            skip(it, closed_bracket);
        }
        else {
            assert(opened.count);
            count++;
            advance(it, opened.count);
            skip(it, open_bracket);
        }
    }
    
    return sub_string(start, *it);
}

Coroutine * get_coroutine(Coroutines coroutines, string identifier) {
    for_array_item(coroutine, coroutines) {
        if (coroutine->identifier == identifier)
            return coroutine;
    }
    
    return null;
}

struct Scope {
    Declarations declarations;
};

#define Template_Array_Name      Scopes
#define Template_Array_Data_Type Scope
#include "template_array.h"

void write_co_read_state(Memory_Allocator *allocator, string *body, Scopes scopes) {
    return;
    
    write(allocator, body, S("auto head = co_begin_read_state(call_stack);\r\n"));
    
    for (u32 i = 0; i < scopes[scopes.count - 1].declarations.count; i++ ) {
        auto declaration = scopes[scopes.count - 1].declarations + i;
        
        write(allocator, body, S("% = next_item(&head, %);\r\n"), f(declaration->identifier), f(declaration->type.identifier));
    }
}

string replace_state_identifiers(Memory_Allocator *allocator, string expression, Scopes scopes) {
    string result = {};
    
    while (expression.count) {
        auto identifier = get_identifier(&expression, S("_"));
        if (identifier.count) {
            Declaration *best_declaration = null;
            
            u32 byte_offset = sizeof(u8*) + sizeof(u32); // Coroutine_Fuction and coroutine state, just any pointer size will do (u8*)
            u32 best_byte_offset = 0;
            for(u32 scope_index = 0; scope_index < scopes.count; scope_index++) {
                auto scope = scopes + scope_index;
                
                for_array_item(it, scope->declarations) {
                    if (identifier == it->identifier) {
                        best_declaration = it;
                        best_byte_offset = byte_offset;
                    }
                    
                    assert(it->type.byte_count);
                    byte_offset += it->type.byte_count;
                }
            }
            
            if (best_declaration) {
                write(allocator, &result, S("(*(%*)(call_stack->buffer.data + call_stack->current_byte_index + %))"), f(best_declaration->type.identifier), f(best_byte_offset));
            }
            else {
                write(allocator, &result, S("%"), f(identifier));
            }
        }
        else {
            while (expression.count) {
                u32 byte_count;
                u32 head = utf8_head(expression, &byte_count);
                
                if (is_letter(head) || head == '_')
                    break;
                
                advance(&expression, byte_count);
                write_utf8(allocator, &result, head);
            }
        }
    }
    
    return result;
}

u32 return_value_byte_offset_of(Coroutine *coroutine, u32 return_value_index) {
    assert(return_value_index < coroutine->return_types.count);
    
    u32 byte_offset = 0;
    for (u32 i = 0; i < return_value_index; i++) {
        assert(coroutine->return_types[i].byte_count);
        byte_offset += coroutine->return_types[i].byte_count;
    }
    
    return byte_offset;
}

u32 scopes_byte_count_of(Scopes scopes) {
    u32 byte_count = 0;
    
    for_array_item(scope, scopes) {
        for_array_item(declaration, scope->declarations) {
            assert(declaration->type.byte_count);
            byte_count += declaration->type.byte_count;
        }
    }
    
    return byte_count;
}
#if 0
//co_call(allocator, user_data) returns = function(arguments)
{
    Coroutine_Stack call_stack = { allocator, {}, 0 };
    grow_item(allocator, &call_stack.buffer, u32) = 0;
    *grow_item(allocator, &call_stack.buffer, Coroutine_Function) = function;
    // push arguments
    //*grow_item(allocator, &call_stack.buffer, ..) = ..;
    while (step(&call_stack, user_data));
    if (call_stack->current_byte_index == 0) {
        // set return values
    }
    else {
        //was abborted
    }
}
#endif

int main() {
    auto win32_allocator = make_win32_allocator();
    init_win32_allocator();
    init_c_allocator();
    init_Memory_Growing_Stack_allocators();
    auto allocator = C_Allocator;
    
    auto transient_memory = make_memory_growing_stack(&win32_allocator);
    
    auto source = win32_read_entire_file(S("preprocessor_test.mcpp"), allocator);
    
    Types types = {};
    Coroutines coroutines = {};
    
#define add_base_type(name) *grow(allocator, &types) = { S(# name), sizeof(name) };
    
    add_base_type(any);
    
    add_base_type(u8);
    add_base_type(u16);
    add_base_type(u32);
    add_base_type(u64);
    add_base_type(usize);
    
    add_base_type(s8);
    add_base_type(s16);
    add_base_type(s32);
    add_base_type(s64);
    
    add_base_type(f32);
    add_base_type(f64);
    
    add_base_type(string);
    add_base_type(u8_array);
    add_base_type(u8_buffer);
    add_base_type(Memory_Allocator);
    
#undef add_base_type
    
    {
        auto it = source;
        while (it.count) {
            skip_space(&it);
            
            if (try_skip(&it, S("type"))) {
                skip_space(&it);
                
                assert(!parse_type(&it, types).identifier.count);
                string type_name = get_identifier(&it, S("_"));
                assert(type_name.count);
                skip_space(&it);
                
                auto byte_count = PARSE_UNSIGNED_INTEGER(&it, 32);
                
                *grow(allocator, &types) = { type_name, byte_count };
                
                skip(&it, S(";"));
                continue;
            }
            
            if (try_skip(&it, S("struct"))) {
                skip_space(&it);
                
                if (!parse_type(&it, types).identifier.count) {
                    string type_name = get_identifier(&it, S("_"));
                    assert(type_name.count);
                    
                    *grow(allocator, &types) = { type_name, 0 };
                }
                
                skip_space(&it);
                
                // skipping scope is optional :D
                skip_scope(&it, S("{"), S("}"));
                skip_space(&it);
                skip(&it, S(";"));
                continue;
            }
            
            if (try_skip(&it, S("typedef"))) {
                skip_space(&it);
                
                assert(!parse_type(&it, types).identifier.count);
                
                skip_until_first_in_set(&it, White_Space, true);
                string type_name = get_identifier(&it, S("_"));
                assert(type_name.count);
                skip_space(&it);
                
                *grow(allocator, &types) = { type_name, 0 };
                
                skip(&it, S(";"));
                continue;
            }
            
            if (try_skip(&it, S("coroutine"))) {
                skip_space(&it);
                
                Coroutine coroutine = {};
                
                LOOP {
                    auto return_type = parse_type(&it, types);
                    assert(return_type.identifier.count);
                    skip_space(&it);
                    
                    *grow(allocator, &coroutine.return_types) = return_type;
                    
                    if (!try_skip(&it, S(",")))
                        break;
                    
                    skip_space(&it);
                }
                
                string function_name = get_identifier(&it, S("_"));
                assert(function_name.count);
                
                assert(!get_coroutine(coroutines, function_name));
                coroutine.identifier = function_name;
                
                skip(&it, S("("));
                skip_space(&it);
                
                bool expecting_argument = false;
                LOOP {
                    auto argument_type = parse_type(&it, types);
                    if (!expecting_argument && !argument_type.identifier.count) {
                        skip(&it, S(")"));
                        skip_space(&it);
                        break;
                    }
                    
                    assert(argument_type.identifier.count);
                    skip_space(&it);
                    
                    string identifier = get_identifier(&it, S("_"));
                    assert(identifier.count);
                    
                    *grow(allocator, &coroutine.arguments) = { argument_type, identifier };
                    
                    if (try_skip(&it, S(","))) {
                        expecting_argument = true;
                        skip_space(&it);
                        continue;
                    }
                    
                    skip(&it, S(")"));
                    break;
                }
                
                *grow(allocator, &coroutines) = coroutine;
                
                skip_space(&it);
                assert(skip_scope(&it, S("{"), S("}")).count);
                
                continue;
            }
        }
    }
    
    {
        string output = {};
        
        // forward declare coroutines
        if (coroutines.count) {
            write(allocator, &output, S("#include \"u8_buffer.h\"\r\n\r\n"));
            
            write(allocator, &output, S(
                "struct Coroutine_Stack {\r\n"
                "Memory_Allocator *allocator;\r\n"
                "u8_array buffer;\r\n"
                "u32 current_byte_index;\r\n"
                "};\r\n\r\n"
                ));
            
            write(allocator, &output, S("typedef (*Coroutine_Function)(Coroutine_Stack *call_stack, any user_data);\r\n\r\n"));
            
            for_array_item(coroutine, coroutines) {
                write(allocator, &output, S("bool %(Coroutine_Stack *call_stack, any user_data);\r\n"), f(coroutine->identifier));
            }
        }
        
        write(allocator, &output, S("\r\n"));
        
        auto it = source;
        while (it.count) {
            skip_space(&it);
            
            if (try_skip(&it, S("type"))) {
                skip_until_first_in_set(&it, S(";"));
                continue;
            }
            
            if (try_skip(&it, S("coroutine"))) {
                skip_space(&it);
                
                LOOP {
                    auto return_type = parse_type(&it, types);
                    assert(return_type.identifier.count);
                    skip_space(&it);
                    
                    if (!try_skip(&it, S(",")))
                        break;
                    
                    skip_space(&it);
                }
                
                string function_name = get_identifier(&it, S("_"));
                assert(function_name.count);
                
                auto coroutine = get_coroutine(coroutines, function_name);
                assert(coroutine);
                
                skip_until_first_in_set(&it, S("{"), true);
                skip_space(&it);
                
                write(allocator, &output, S("bool %(Coroutine_Stack *call_stack, any user_data) {\r\n"), f(coroutine->identifier));
                
                string switch_table = write(allocator, S("switch (*(u32 *)(call_stack->buffer.data + call_stack->current_byte_index + sizeof(Coroutine_Function))) { // switch on coroutine state\r\n"));
                defer { free_array(allocator, &switch_table); };
                
                write(allocator, &switch_table, S("case 0:\r\ngoto co_label0;\r\n\r\n"));
                
                string body = {};
                defer { free_array(allocator, &body); };
                
                defer { write(allocator, &output, S("%default:\r\nassert(0);\r\n}\r\n\r\n%\r\n\r\n"), f(switch_table), f(body)); };
                
                Scopes scopes = {};
                defer { free_array(allocator, &scopes); };
                
                *grow(allocator, &scopes) = {};
                
                for_array_item(argument, coroutine->arguments) {
                    *grow(allocator, &scopes[scopes.count - 1].declarations) = *argument;
                    
                    write(allocator, &body, S("// argument: % %;\r\n"), f(argument->type.identifier), f(argument->identifier));
                }
                
                u32 label_count = 0;
                
                write(allocator, &body, S("\r\nco_label%:\r\n"), f(label_count));
                
                while (scopes.count) {
                    skip_space(&it);
                    
                    while (try_skip(&it, S(";")))
                        skip_space(&it);
                    
                    if (try_skip(&it, S("}"))) {
                        for (u32 i = 0; i < scopes[scopes.count - 1].declarations.count; i++ ) {
                            auto declaration = scopes[scopes.count - 1].declarations + scopes[scopes.count - 1].declarations.count - i - 1;
                            
                            write(allocator, &body, S("co_pop(call_stack, %); // local variable % \r\n"), f(declaration->type.identifier), f(declaration->identifier));
                        }
                        
                        free_array(allocator, &scopes[scopes.count - 1].declarations);
                        shrink(allocator, &scopes);
                        
                        if (scopes.count == 0)
                            write(allocator, &body, S("\r\nassert(0);\r\nreturn false;\r\n"));
                        
                        write(allocator, &body, S("}\r\n"));
                        continue;
                    }
                    
                    if (try_skip(&it, S("{"))) {
                        *grow(allocator, &scopes) = {};
                        
                        write(allocator, &body, S("{\r\n"));
                        continue;
                    }
                    
                    if (try_skip(&it, S("co_yield"))) {
                        skip_space(&it);
                        
                        string_array left_expressions = {};
                        defer { free_array(&transient_memory.allocator, &left_expressions); };
                        
                        bool previous_was_comma = false;
                        LOOP {
                            string tests[3];
                            tests[0] = get_token_until_first_in_set(it, S(","));
                            tests[1] = get_token_until_first_in_set(it, S("="));
                            tests[2] = get_token_until_first_in_set(it, S("("));
                            
                            u32 best_test_index = 0;
                            for (u32 a = 0; a < ARRAY_COUNT(tests); a++) {
                                if (tests[a].count && (tests[best_test_index].count > tests[a].count)) {
                                    best_test_index = a;
                                }
                            }
                            
                            assert(tests[best_test_index].count);
                            
                            // we have nothing to be set
                            if (best_test_index == 2) {
                                assert(!previous_was_comma);
                                break;
                            }
                            
                            advance(&it, tests[best_test_index].count);
                            skip_space(&it);
                            
                            *grow(&transient_memory.allocator, &left_expressions) = tests[best_test_index];
                            
                            // break on '='
                            if (best_test_index == 1) {
                                skip(&it, S("="));
                                skip_space(&it);
                                break;
                            }
                            
                            skip(&it, S(","));
                            skip_space(&it);
                            previous_was_comma = true;
                        }
                        
                        auto identifier = get_identifier(&it, S("_"));
                        assert(identifier.count);
                        skip_space(&it);
                        
                        auto subroutine = get_coroutine(coroutines, identifier);
                        assert(subroutine);
                        
                        assert(!left_expressions.count || (left_expressions.count == subroutine->return_types.count));
                        
                        label_count++;
                        
                        {
                            write(allocator, &body, S("\r\n{ // call %\r\n"), f(subroutine->identifier));
                            skip(&it, S("("));
                            skip_space(&it);
                            
                            write(allocator, &switch_table, S("case %:\r\ngoto co_label%;\t\n\r\n"), f(label_count), f(label_count));
                            
                            write(allocator, &body, S("*(u32 *)(call_stack->buffer.data + call_stack->current_byte_index + sizeof(Coroutine_Function)) = %; // set coroutine state\r\n"), f(label_count));
                            
                            for (u32 i = 0; i < subroutine->arguments.count; i++) {
                                string expression;
                                if (i == subroutine->arguments.count - 1) {
                                    expression = skip_until_first_in_set(&it, S(")"), true);
                                }
                                else {
                                    expression = skip_until_first_in_set(&it, S(","), true);
                                }
                                
                                string expression_prime = replace_state_identifiers(allocator, expression, scopes);
                                defer { free_array(allocator, &expression_prime); };
                                
                                write(allocator, &body, S("% arg% = %;\r\n"), f(subroutine->arguments[i].type.identifier), f(i), f(expression_prime));
                            }
                            
                            write(allocator, &body, S("*grow_item(call_stack->allocator, &call_stack->buffer, u32) = call_stack->current_byte_index;\r\n"));
                            write(allocator, &body, S("call_stack->current_byte_index = byte_count_of(call_stack->buffer);\r\n"));
                            write(allocator, &body, S("*grow_item(call_stack->allocator, &call_stack->buffer, Coroutine_Function) = %;\r\n"), f(subroutine->identifier));
                            write(allocator, &body, S("*grow_item(call_stack->allocator, &call_stack->buffer, u32) = 0; // couroutine state co_state = initial state\r\n"));
                            
                            for (u32 i = 0; i < subroutine->arguments.count; i++) {
                                write(allocator, &body, S("*grow_item(call_stack->allocator, &call_stack->buffer, %) = arg%;\r\n"), f(subroutine->arguments[i].type.identifier), f(i));
                            }
                            
                            write(allocator, &body, S("return true;\r\n"));
                            write(allocator, &body, S("}\r\n"));
                        }
                        
                        write(allocator, &body, S("\r\nco_label%:\r\n"), f(label_count));
                        
                        {
                            write(allocator, &body, S("{ // read results\r\n"));
                            write_co_read_state(allocator, &body, scopes);
                            
                            if (left_expressions.count) {
                                for (u32 i = 0; i < subroutine->return_types.count; i++) {
                                    string expression_prime = replace_state_identifiers(allocator, left_expressions[i], scopes);
                                    defer { free_array(allocator, &expression_prime); };
                                    
                                    write(allocator, &body, S("% = (*(%)(call_stack->current_byte_index + %));\r\n"), f(expression_prime), f(subroutine->return_types[i].identifier), f(scopes_byte_count_of(scopes) + return_value_byte_offset_of(subroutine, i)));
                                    
                                    skip_space(&it);
                                }
                            }
                            
                            // pop return values in return
                            for (u32 i = 0; i < subroutine->return_types.count; i++)
                                write(allocator, &body, S("co_pop(call_stack, %); // pop return value %\r\n"), f(subroutine->return_types[subroutine->return_types.count - i - 1].identifier), f(subroutine->return_types.count - i - 1));
                            
                            write(allocator, &body, S("}\r\n\r\n"));
                            
                            // reread state
                            //write_co_read_state(allocator, &body, scopes);
                        }
                        
                        continue;
                    }
                    
                    if (try_skip(&it, S("co_return"))) {
                        skip_space(&it);
                        
                        write(allocator, &body, S("\r\n{ // return\r\n"));
                        
                        for (u32 i = 0; i < coroutine->return_types.count; i++) {
                            string expression;
                            if (i == coroutine->return_types.count - 1) {
                                expression = skip_until_first_in_set(&it, S(";"), true);
                            }
                            else {
                                auto start = it;
                                
                                LOOP {
                                    auto a = get_token_until_first_in_set(it, S("("));
                                    auto b = get_token_until_first_in_set(it, S(","));
                                    
                                    if (a.count && (a.count < b.count)) {
                                        advance(&it, a.count);
                                        skip_scope(&it, S("("), S(")"));
                                    }
                                    else {
                                        assert(b.count);
                                        advance(&it, b.count);
                                        expression = sub_string(start, it);
                                        skip(&it, S(","));
                                        skip_space(&it);
                                        break;
                                    }
                                }
                            }
                            
                            assert(expression.count);
                            string expression_prime = replace_state_identifiers(allocator, expression, scopes);
                            defer { free_array(allocator, &expression_prime); };
                            
                            write(allocator, &body, S("% return% = %;\r\n"), f(coroutine->return_types[i].identifier), f(i), f(expression_prime));
                        }
                        
                        write(allocator, &body, S("call_stack->current_byte_index = *(u32 *)(call_stack->buffer + call_stack->current_byte_index - sizeof(u32)); // read previous call entry\r\n"));
                        write(allocator, &body, S("shrink(call_stack->allocator, &call_stack->buffer, byte_count_of(call_stack->buffer) - call_stack->current_byte_index +sizeof(u32)); // pop couroutine state\r\n"));
                        
                        for (u32 i = 0; i < coroutine->return_types.count; i++) {
                            write(allocator, &body, S("*grow_item(call_stack->allocator, &call_stack->buffer, %) = return%;\r\n"), f(coroutine->return_types[i].identifier), f(i));
                        }
                        
                        write(allocator, &body, S("return true;\r\n}\r\n\r\n"));
                        
                        continue;
                    }
                    
                    auto type = parse_type(&it, types);
                    if (type.identifier.count) {
                        skip_space(&it);
                        auto identifier = get_identifier(&it, S("_"));
                        skip_space(&it);
                        *grow(allocator, &scopes[scopes.count - 1].declarations) = { type, identifier };
                        
                        write(allocator, &body, S("grow_item(call_stack->allocator, &call_stack->buffer, %); // local variable %\r\n"), f(type.identifier), f(identifier));
                        
                        //write_co_read_state(allocator, &body, scopes);
                        
                        if (try_skip(&it, S("="))) {
                            skip_space(&it);
                            auto expression = skip_until_first_in_set(&it, S(";"));
                            
                            string expression_prime = replace_state_identifiers(allocator, expression, scopes);
                            defer { free_array(allocator, &expression_prime); };
                            
                            string identifier_prime = replace_state_identifiers(allocator, identifier, scopes);
                            defer { free_array(allocator, &identifier_prime); };
                            
                            write(allocator, &body, S("*% = %;\r\n"), f(identifier_prime), f(expression_prime));
                        }
                        
                        skip(&it, S(";"));
                        
                        continue;
                    }
                    
                    auto something = get_token_until_first_in_set(it, S("{};"));
                    
                    string expression_prime = replace_state_identifiers(allocator, something, scopes);
                    defer { free_array(allocator, &expression_prime); };
                    
                    write(allocator, &body, S("%"), f(expression_prime));
                    advance(&it, something.count);
                    
                    if (try_skip(&it, S(";"))) {
                        write(allocator, &body, S(";\r\n"));
                    }
                }
                
                skip_space(&it);
                
                continue;
            }
        }
        
        win32_write_entire_file(S("preprocessor_test.cpp"), output);
    }
    
    return 1;
}