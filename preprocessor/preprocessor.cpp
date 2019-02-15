
-------------------------------------------

TODO: make this work

def start function(Memory_Allocator *allocator) {
	co_call(allocator, factorial_iterative(12));
}

-------------------------------------------


#include "win32_platform.h"
#include "memory_c_allocator.h"
#include "memory_growing_stack.h"

//#include "preprocessor_test.cpp"

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

struct Function {
    string identifier;
    Declarations arguments;
    Types return_types;
    bool is_coroutine;
};

#define Template_Array_Name      Functions
#define Template_Array_Data_Type Function
#include "template_array.h"


struct Best_Token {
    string *it;
    string best;
    u32 index;
    u32 count;
};

Best_Token begin_best_token(string *it) {
    return { it };
}

void test_token(Best_Token *token, string end) {
    auto a = get_token_until_first_in_set(*token->it, end);
    if (!token->best.count || (a.count && a.count < token->best.count)) {
        token->best = a;
        token->index = token->count;
    }
    
    token->count++;
}

bool end(Best_Token *token) {
    advance(token->it, token->best.count);
    return (token->best.count > 0);
}

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

Function * find_function(Functions functions, string identifier) {
    for_array_item(function, functions) {
        if (function->identifier == identifier)
            return function;
    }
    
    return null;
}

struct Scope {
    Declarations declarations;
};

#define Template_Array_Name      Scopes
#define Template_Array_Data_Type Scope
#include "template_array.h"

string replace_expression(Memory_Allocator *allocator, string expression, Functions functions, Function *function, Scopes scopes) {
    string result = {};
    
    while (expression.count)
    {
        auto identifier = get_identifier(&expression, S("_"));
        if (identifier.count) {
            auto subroutine = find_function(functions, identifier);
            
            if (!function->is_coroutine) {
                if (!subroutine || !subroutine->is_coroutine) {
                    write(allocator, &result, S("%"), f(identifier));
                    continue;
                }
                
                if (subroutine && subroutine->is_coroutine) {
                    UNREACHABLE_CODE;
                    continue;
                }
            }
            
            if (subroutine && !subroutine->is_coroutine) {
                write(allocator, &result, S("%"), f(identifier));
                continue;
            }
            
            if (subroutine) {
                UNREACHABLE_CODE;
            }
            
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
                write(allocator, &result, S("CO_ENTRY(%, %)"), f(best_declaration->type.identifier), f(best_byte_offset));
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

void add_label(String_Buffer *switch_table, String_Buffer *body, u32 *label_count) {
    write(switch_table, S("case %:\r\ngoto co_label%;\r\n\r\n"), f(*label_count), f(*label_count));
    
    write(body, S("co_label%:\r\n\r\n"), f(*label_count));
    
    (*label_count)++;
}

u32 scopes_byte_count_of(Scopes scopes) {
    // Coroutine_Fuction (u8*) and coroutine state (u32), just any pointer size will do (u8*)
    u32 byte_count = sizeof(u8*) + sizeof(u32);
    
    for_array_item(scope, scopes) {
        for_array_item(declaration, scope->declarations) {
            assert(declaration->type.byte_count);
            byte_count += declaration->type.byte_count;
        }
    }
    
    return byte_count;
}

u32 return_value_byte_offset_of(Function *function, u32 return_value_index) {
    assert(return_value_index < function->return_types.count);
    
    u32 byte_offset = 0;
    for (u32 i = 0; i < return_value_index; i++) {
        assert(function->return_types[i].byte_count);
        byte_offset += function->return_types[i].byte_count;
    }
    
    return byte_offset;
}

void write_call_subroutine(Memory_Allocator *allocator, String_Buffer *switch_table, String_Buffer *body, string *it, Function *subroutine, string_array left_expressions, u32 *label_count, Functions functions, Function *function, Scopes scopes)
{
    assert(subroutine->is_coroutine);
    skip_space(it);
    
    assert(!left_expressions.count || (left_expressions.count == subroutine->return_types.count));
    
    {
        write(body, S("\r\n{ // call %\r\n"), f(subroutine->identifier));
        skip(it, S("("));
        skip_space(it);
        
        write(body, S("CO_STATE_ENTRY = %;\r\n"), f(*label_count));
        
        for (u32 i = 0; i < subroutine->arguments.count; i++) {
            string expression;
            if (i == subroutine->arguments.count - 1) {
                expression = skip_until_first_in_set(it, S(")"), true);
            }
            else {
                expression = skip_until_first_in_set(it, S(","), true);
            }
            
            string expression_prime = replace_expression(allocator, expression, functions, function, scopes);
            defer { free_array(allocator, &expression_prime); };
            
            write(body, S("% arg% = %;\r\n"), f(subroutine->arguments[i].type.identifier), f(i), f(expression_prime));
        }
        
        write(body, S("CO_PUSH_COROUTINE(%);\r\n"), f(subroutine->identifier));
        
        for (u32 i = 0; i < subroutine->arguments.count; i++) {
            write(body, S("CO_PUSH(%) = arg%;\r\n"), f(subroutine->arguments[i].type.identifier), f(i));
        }
        
        write(body, S("return Coroutine_Continue;\r\n"));
        write(body, S("}\r\n"));
    }
    
    add_label(switch_table, body, label_count);
    
    {
        write(body, S("{ // read results\r\n"));
        
        if (left_expressions.count) {
            for (u32 i = 0; i < subroutine->return_types.count; i++) {
                string expression_prime = replace_expression(allocator, left_expressions[i], functions, function, scopes);
                defer { free_array(allocator, &expression_prime); };
                
                write(body, S("% = CO_ENTRY(%, %);\r\n"), f(expression_prime), f(subroutine->return_types[i].identifier), f(scopes_byte_count_of(scopes) + return_value_byte_offset_of(subroutine, i)));
                
                skip_space(it);
            }
        }
        
        // pop return values in return
        for (u32 i = 0; i < subroutine->return_types.count; i++)
            write(body, S("CO_POP(%); // pop return value %\r\n"), f(subroutine->return_types[subroutine->return_types.count - i - 1].identifier), f(subroutine->return_types.count - i - 1));
        
        write(body, S("}\r\n\r\n"));
    }
}

void write_signature(String_Buffer *output, Function function) {
    if (function.is_coroutine) {
        write(output, S("COROUTINE_DEC(%)"), f(function.identifier));
    }
    else {
        
        if (function.return_types.count) {
            write(output, S("% "), f(function.return_types[0].identifier));
        }
        else {
            write(output, S("void "));
        }
        
        write(output, S("%("), f(function.identifier));
        
        if (function.return_types.count > 1) {
            for (u32 i = 1; i < function.return_types.count; i++) {
                if (function.arguments.count || (i < function.return_types.count - 1))
                    write(output, S("% *_out%, "), f(function.return_types[i].identifier), f(i));
                else
                    write(output, S("% *_out%)"), f(function.return_types[i].identifier), f(i));
            }
        }
        
        for (u32 i = 0; i < function.arguments.count; i++) {
            if (i < function.arguments.count - 1)
                write(output, S("% %, "), f(function.arguments[i].type.identifier), f(function.arguments[i].identifier));
            else
                write(output, S("% %)"), f(function.arguments[i].type.identifier), f(function.arguments[i].identifier));
        }
        
        if ((function.arguments.count == 0) && (function.return_types.count <= 1))
            write(output, S(")"));
    }
}

string parse_next_expression(string *it, string end_token, bool *is_end) {
    string start = *it;
    
    LOOP {
        auto token = begin_best_token(it);
        test_token(&token, S("("));
        test_token(&token, S(","));
        test_token(&token, end_token);
        end(&token);
        
        switch (token.index) {
            case 0: {
                skip_scope(it, S("("), S(")"));
            } break;
            
            case 1: {
                string expression = sub_string(start, *it);
                skip(it, S(","));
                skip_space(it);
                *is_end = false;
                return expression;
            } break;
            
            case 2: {
                string expression = sub_string(start, *it);
                skip(it, end_token);
                skip_space(it);
                *is_end = true;
                return expression;
            } break;
            
            CASES_COMPLETE;
        }
    }
}

void parse_call_subroutine_or_expression(Memory_Allocator *allocator, String_Buffer *switch_table, String_Buffer *body, string *it, string_array left_expressions, u32 *label_count, Functions functions, Function *function, Scopes scopes) {
    string test = *it;
    auto function_identifier = get_identifier(&test, S("_"));
    if (function_identifier.count) {
        skip_space(&test);
        
        auto subroutine = find_function(functions, function_identifier);
        if (subroutine) {
            *it = test;
            
            if (subroutine->is_coroutine) {
                write_call_subroutine(allocator, switch_table, body, it, subroutine, left_expressions, label_count, functions, function, scopes);
                skip(it, S(";"));
            }
            else {
                assert(left_expressions.count == subroutine->return_types.count);
                
                skip(it, S("("));
                skip_space(it);
                
                {
                    string expression_prime = replace_expression(allocator, left_expressions[0], functions, function, scopes);
                    defer { free_array(allocator, &expression_prime); };
                    write(body, S("% = %("), f(expression_prime), f(subroutine->identifier));
                }
                
                for (u32 i = 1; i < subroutine->return_types.count; i++) {
                    string expression_prime = replace_expression(allocator, left_expressions[i], functions, function, scopes);
                    defer { free_array(allocator, &expression_prime); };
                    
                    if (i == subroutine->return_types.count + subroutine->arguments.count - 1)
                        write(body, S("&(%)"), f(expression_prime));
                    else
                        write(body, S("&(%), "), f(expression_prime));
                }
                
                auto rest = skip_until_first_in_set(it, S(";"), true);
                
                string rest_prime = replace_expression(allocator, rest, functions, function, scopes);
                defer { free_array(allocator, &rest_prime); };
                
                write(body, S("%;\r\n"), f(rest_prime));
            }
            
            return;
        }
    }
    
    // not a function call
    {
        u32 left_index = 0;
        bool is_end = false;
        
        while (!is_end) {
            auto expression = parse_next_expression(it, S(";"), &is_end);
            assert(expression.count);
            
            string expression_prime = replace_expression(allocator, expression, functions, function, scopes);
            defer { free_array(allocator, &expression_prime); };
            
            if (left_expressions.count) {
                string left_expression_prime = replace_expression(allocator, left_expressions[left_index], functions, function, scopes);
                defer { free_array(allocator, &left_expression_prime); };
                
                write(body, S("% = %;\r\n"), f(left_expression_prime), f(expression_prime));
            }
            else {
                write(body, S("%;\r\n"), f(expression_prime));
            }
            
            left_index++;
        }
    }
}

int main() {
    auto win32_allocator = make_win32_allocator();
    init_win32_allocator();
    init_c_allocator();
    init_Memory_Growing_Stack_allocators();
    auto allocator = C_Allocator;
    
    auto transient_memory = make_memory_growing_stack(&win32_allocator);
    
    auto source = win32_read_entire_file(S("preprocessor_test.mcpp"), allocator);
    
    Types types = {};
    Functions functions = {};
    
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
            
            if (try_skip(&it, S("def"))) {
                skip_space(&it);
                
                string identifier = get_identifier(&it, S("_"));
                assert(identifier.count);
                skip_space(&it);
                
                bool is_coroutine = try_skip(&it, S("coroutine"));
                if (is_coroutine || try_skip(&it, S("function"))) {
                    skip_space(&it);
                    
                    Function function = {};
                    function.identifier = identifier;
                    function.is_coroutine = is_coroutine;
                    
                    skip(&it, S("("));
                    
                    bool previous_was_comma = false;
                    LOOP {
                        skip_space(&it);
                        
                        if (!previous_was_comma) {
                            if (try_skip(&it, S(")"))) {
                                skip_space(&it);
                                break;
                            }
                        }
                        
                        Declaration declaration = {};
                        
                        declaration.identifier = get_identifier(&it, S("_"));
                        assert(declaration.identifier.count);
                        skip_space(&it);
                        
                        skip(&it, S(":"));
                        skip_space(&it);
                        
                        declaration.type = parse_type(&it, types);
                        assert(declaration.type.identifier.count);
                        skip_space(&it);
                        
                        *grow(allocator, &function.arguments) = declaration;
                        
                        previous_was_comma = try_skip(&it, S(","));
                    }
                    
                    if (try_skip(&it, S("->"))) {
                        skip_space(&it);
                        
                        skip(&it, S("("));
                        
                        bool previous_was_comma = false;
                        LOOP {
                            skip_space(&it);
                            
                            if (!previous_was_comma) {
                                if (try_skip(&it, S(")"))) {
                                    skip_space(&it);
                                    break;
                                }
                            }
                            
                            Type type = parse_type(&it, types);
                            assert(type.identifier.count);
                            skip_space(&it);
                            
                            *grow(allocator, &function.return_types) = type;
                            
                            previous_was_comma = try_skip(&it, S(","));
                        }
                    }
                    
                    *grow(allocator, &functions) = function;
                    
                    skip_scope(&it, S("{"), S("}"));
                    
                    continue;
                }
                
                if (try_skip(&it, S("struct"))) {
                    skip_space(&it);
                    
                    *grow(allocator, &types) = { identifier, 0 };
                    
                    skip_scope(&it, S("{"), S("}"));
                    
                    continue;
                }
                
                UNREACHABLE_CODE;
            }
            
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
        }
    }
    
    String_Buffer output = { allocator };
    
    // forward declare coroutines
    if (functions.count) {
        write(&output, S("#include \"preprocessor_coroutines.h\"\r\n\r\n"));
        
        for_array_item(function, functions) {
            write_signature(&output, *function);
            write(&output, S(";\r\n"));
        }
        
        write(&output, S("\r\n"));
    }
    
    {
        auto it = source;
        while (it.count) {
            skip_space(&it);
            
            if (try_skip(&it, S("def"))) {
                skip_space(&it);
                
                string identifier = get_identifier(&it, S("_"));
                assert(identifier.count);
                skip_space(&it);
                
                bool is_coroutine = try_skip(&it, S("coroutine"));
                if (is_coroutine || try_skip(&it, S("function"))) {
                    skip_space(&it);
                    
                    auto function = find_function(functions, identifier);
                    write_signature(&output, *function);
                    write(&output, S(" {\r\n"));
                    
                    auto switch_table = new_write(allocator, S("switch (CO_STATE_ENTRY) {\r\n"));
                    defer { free(&switch_table); };
                    
                    String_Buffer body = { allocator };
                    defer { free(&body); };
                    
                    defer { 
                        if (function->is_coroutine) 
                            write(&output, S("%default:\r\nassert(0);\r\n}\r\n\r\n%\r\n\r\n"), f(switch_table.buffer), f(body.buffer)); 
                        else
                            write(&output, S("%\r\n\r\n"), f(body.buffer));
                    };
                    
                    Scopes scopes = {};
                    defer { free_array(allocator, &scopes); };
                    
                    *grow(allocator, &scopes) = {};
                    
                    for_array_item(argument, function->arguments) {
                        *grow(allocator, &scopes[scopes.count - 1].declarations) = *argument;
                        
                        if (function->is_coroutine)
                            write(&body, S("// argument: def %: %;\r\n"), f(argument->identifier), f(argument->type.identifier));
                    }
                    
                    u32 label_count = 0;
                    
                    if (function->is_coroutine)
                        add_label(&switch_table, &body, &label_count);
                    
                    skip_until_first_in_set(&it, S("{"), true);
                    
                    while (scopes.count) {
                        skip_space(&it);
                        
                        while (try_skip(&it, S(";")))
                            skip_space(&it);
                        
                        if (try_skip(&it, S("}"))) {
                            if (function->is_coroutine) {
                                for (u32 i = 0; i < scopes[scopes.count - 1].declarations.count; i++ ) {
                                    auto declaration = scopes[scopes.count - 1].declarations + scopes[scopes.count - 1].declarations.count - i - 1;
                                    
                                    write(&body, S("CO_POP(%); // local variable % \r\n"), f(declaration->type.identifier), f(declaration->identifier));
                                }
                                
                                
                                if (scopes.count == 0)
                                    write(&body, S("\r\nassert(0);\r\nreturn Coroutine_Abort;\r\n"));
                            }
                            
                            free_array(allocator, &scopes[scopes.count - 1].declarations);
                            shrink(allocator, &scopes);
                            
                            write(&body, S("}\r\n\r\n"));
                            continue;
                        }
                        
                        if (try_skip(&it, S("{"))) {
                            *grow(allocator, &scopes) = {};
                            
                            write(&body, S("{\r\n"));
                            continue;
                        }
                        
                        bool is_return = try_skip(&it, S("return"));
                        
                        bool is_yield;
                        if (!is_return)
                            is_yield = try_skip(&it, S("yield"));
                        else
                            is_yield = false;
                        
                        if (is_return || is_yield) {
                            skip_space(&it);
                            
                            if (!function->is_coroutine) {
                                assert(!is_yield);
                                
                                if (!function->return_types.count) {
                                    skip(&it, S(";"));
                                    write(&body, S("return;"));
                                    continue;
                                }
                                
                                bool is_end;
                                string first_expression = parse_next_expression(&it, S(";"), &is_end);
                                assert(is_end == (function->return_types.count == 1));
                                
                                for (u32 i = 1; i < function->return_types.count; i++) {
                                    bool is_end;
                                    string expression = parse_next_expression(&it, S(";"), &is_end);
                                    assert(expression.count);
                                    assert(is_end == (i == function->return_types.count - 1));
                                    
                                    string expression_prime = replace_expression(allocator, expression, functions, function, scopes);
                                    defer { free_array(allocator, &expression_prime); };
                                    
                                    write(&body, S("*_out% = %;\r\n"), f(i), f(expression_prime));
                                }
                                
                                string first_expression_prime = replace_expression(allocator, first_expression, functions, function, scopes);
                                defer { free_array(allocator, &first_expression_prime); };
                                
                                write(&body, S("return %;\r\n"), f(first_expression_prime));
                                continue;
                            }
                            
                            if (is_return)
                                write(&body, S("\r\n{ // return\r\n"));
                            else
                                write(&body, S("\r\n{ // yield\r\n"));
                            
                            for (u32 i = 0; i < function->return_types.count; i++) {
                                bool is_end;
                                string expression = parse_next_expression(&it, S(";"), &is_end);
                                assert(expression.count);
                                assert(is_end == (i == function->return_types.count - 1));
                                
                                string expression_prime = replace_expression(allocator, expression, functions, function, scopes);
                                defer { free_array(allocator, &expression_prime); };
                                
                                write(&body, S("% return% = %;\r\n"), f(function->return_types[i].identifier), f(i), f(expression_prime));
                            }
                            
                            if (is_return) {
                                write(&body, S("CO_POP_COROUTINE;\r\n"));
                            }
                            
                            for (u32 i = 0; i < function->return_types.count; i++) {
                                write(&body, S("CO_PUSH(%) = return%;\r\n"), f(function->return_types[i].identifier), f(i));
                            }
                            
                            if (is_return)
                                write(&body, S("return Coroutine_Continue;\r\n}\r\n\r\n"));
                            else {
                                
                                write(&body, S("CO_STATE_ENTRY = %;\r\n"), f(label_count));
                                write(&body, S("return Coroutine_Wait;\r\n}\r\n\r\n"));
                                
                                add_label(&switch_table, &body, &label_count);
                            }
                            
                            continue;
                        }
                        
                        if (try_skip(&it, S("var")))
                        {
                            string_array left_expressions = {};
                            defer { free_array(allocator, &left_expressions); };
                            
                            LOOP {
                                skip_space(&it);
                                
                                auto identifier = get_identifier(&it, S("_"));
                                assert(identifier.count);
                                skip_space(&it);
                                
                                skip(&it, S(":"));
                                skip_space(&it);
                                
                                auto type = parse_type(&it, types);
                                assert(type.identifier.count);
                                skip_space(&it);
                                
                                *grow(allocator, &scopes[scopes.count - 1].declarations) = { type, identifier };
                                *grow(allocator, &left_expressions) = identifier;
                                
                                if (function->is_coroutine)
                                    write(&body, S("CO_PUSH(%); // local variable %\r\n"), f(type.identifier), f(identifier));
                                else
                                    write(&body, S("% %;\r\n"), f(type.identifier), f(identifier));
                                
                                if (!try_skip(&it, S(","))) {
                                    break;
                                }
                            }
                            
                            if (try_skip(&it, S("="))) {
                                skip_space(&it);
                                parse_call_subroutine_or_expression(allocator, &switch_table, &body, &it, left_expressions, &label_count, functions, function, scopes);
                            }
                            else {
                                skip(&it, S(";"));
                            }
                            
                            continue;
                        }
                        
                        {
                            string start = it;
                            bool is_directive = try_skip(&it, S("if"));
                            
                            if (!is_directive)
                                is_directive = try_skip(&it, S("switch"));
                            
                            if (!is_directive)
                                is_directive = try_skip(&it, S("while"));
                            
                            if (!is_directive)
                                is_directive = try_skip(&it, S("loop"));
                            
                            
                            if (is_directive) {
                                string directive = sub_string(start, it);
                                skip_space(&it);
                                
                                string expression = skip_until_first_in_set(&it, S("{"));
                                assert(expression.count);
                                
                                auto expression_prime = replace_expression(allocator, expression, functions, function, scopes);
                                defer { free_array(allocator, &expression_prime); };
                                
                                write(&body, S("% (%) "), f(directive), f(expression_prime));
                                continue;
                            }
                        }
                        
                        {
                            parse_call_subroutine_or_expression(allocator, &switch_table, &body, &it, {}, &label_count, functions, function, scopes);
                            continue;
                        }
                        
                        UNREACHABLE_CODE;
                    }
                    
                    continue;
                }
                
                if (try_skip(&it, S("struct"))) {
                    skip_space(&it);
                    
                    *grow(allocator, &types) = { identifier, 0 };
                    
                    skip_scope(&it, S("{"), S("}"));
                    
                    continue;
                }
                
                UNREACHABLE_CODE;
            }
        }
    }
    
    win32_write_entire_file(S("preprocessor_test.cpp"), output.buffer);
    
#if 0
    auto cs = begin(&transient_memory.allocator, factorial_recursive);
    {
        auto call_stack = &cs;
        CO_PUSH(u32) = 5;
        
        bool do_continue = true;
        while (do_continue) {
            auto dir = step(call_stack, null);
            
            if ((dir == Coroutine_End) || (dir == Coroutine_Abort))
                do_continue = false;
            
            switch (dir) {
                case Coroutine_Wait:
                case Coroutine_End: 
                {
                    u8_array head;
                    head.count = sizeof(u32);
                    head.data = one_past_last(cs.buffer) - head.count;
                    
                    auto n = *next_item(&head, u32);
                    
                    printf("factorial_iteratrive = %u\n", n);
                    
                    CO_POP(u32);
                } break;
            }
        }
    }
    end(cs);
#endif
    
    return 1;
}