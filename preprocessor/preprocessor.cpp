
//TODO: add template support array and list :D

#if 0

def start function(Memory_Allocator *allocator) {
	// run while ignoring yields, wors
	var x: = run allocator, factorial_iterative(12);
    
    // TODO
    // not supported yet
    var x: u32 = run allocator, factorial_iterative(12) {
        factorial_iterative {
            var x: u32 = yield_values;
            // overide arguments
            resume(x - 1);
            // or resume without overriding arguments
            // resume ;
        }
    }
    else {
        //handle abort...
    }
}

#endif

#include "win32_platform.h"
#include "memory_c_allocator.h"
#include "memory_growing_stack.h"
#include "preprocessor_coroutines.h"

struct Text_Iterator {
    string text;
    u32 line;
};

void advance_line_count(Text_Iterator *it, string skipped_text, String_Buffer *body) {
    while (skipped_text.count) {
        u32 head = utf8_advance(&skipped_text);
        if (head == '\n') {
            it->line++;
            
            if (body)
                write(body, S("#line %\r\n"), f(it->line));
        }
    }
}

string skip_until_first_in_set(Text_Iterator *it, string set, String_Buffer *body, bool do_skip_set = false, bool include_end = true)
{
    auto result = skip_until_first_in_set(&it->text, set, do_skip_set, include_end);
    advance_line_count(it, result, body);
    
    return result;
}

bool try_skip(Text_Iterator *it, string prefix, String_Buffer *body) {
    bool result = try_skip(&it->text, prefix);
    if (result)
        advance_line_count(it, prefix, body);
    
    return result;
}

string skip_scope(Text_Iterator *it, string open_bracket, string closed_bracket, String_Buffer *body);

void skip_space(Text_Iterator *it, String_Buffer *body) {
    auto White_Space = S(" \t\r\0");
    
    while (it->text.count) {
        auto start = it->text;
        
        if (try_skip(&it->text, S("//"))) {
            skip_until_first_in_set(&it->text, S("\n"), true);
            it->line++;
        }
        
        skip_scope(it, S("/*"), S("*/"), body);
        
        u32 line = it->line;
        string skipped = skip_set(&it->text, S("\n"));
        advance_line_count(it, skipped, body);
        
        skip_set(&it->text, White_Space);
        
        if (it->text.data == start.data)
            break;
    }
}

struct Type;

#define Template_Array_Name      Types
#define Template_Array_Data_Type Type*
#include "template_array.h"

#define Template_Tree_Name      Type_Node
#define Template_Tree_Data_Type Type*
#define Template_Tree_Data_Name type
#include "template_tree.h"

struct Declaration {
    Type *type;
    string identifier;
};

#define Template_Array_Name      Declarations
#define Template_Array_Data_Type Declaration
#include "template_array.h"

struct Structure;

#define Template_Array_Name      Structures
#define Template_Array_Data_Type Structure*
#include "template_array.h"

struct Structure {
    enum Kind {
        Kind_null,
        Kind_root,
        Kind_sub_structure,
        Kind_field,
        Kind_Count
    } kind;
    
    bool is_kind;
    
    union {
        struct {
            string identifier;
            u32 byte_count;
            Types dependencies;
        } root;
        
        struct {
            string identifier;
            u32 byte_count;
        } sub_structure;
        
        Declaration field;
    };
};

#define Template_Tree_Name      Structure_Node
#define Template_Tree_Data_Type Structure
#define Template_Tree_Data_Name structure
#include "template_tree.h"

// yes an array of an array of Type*
// used for depth first search iteration
#define Template_Array_Name      Types_Array
#define Template_Array_Data_Type Types
#include "template_array.h"

struct Type {
    enum Kind {
        Kind_null,
        Kind_base_type,
        Kind_indirection,
        Kind_structure_node,
        Kind_structure_field,
        Kind_Count,
    } kind;
    
    union {
        struct {
            string identifier;
            u32 byte_count;
            Types dependencies;
        } base_type;
        
        struct {
            Type *type;
            u32 level;
        } indirection;
        
        Structure_Node structure_node;
    };
};

u32 byte_count_of(Type type) {
    switch (type.kind) {
        case_kind(Type, base_type) {
            assert(type.base_type.byte_count);
            return type.base_type.byte_count;
        }
        
        case_kind(Type, indirection) {
            return sizeof(u8*); // just any pointer size
        }
        
        CASES_COMPLETE;
        return 0;
    }
}

struct Type_Format_Info {
    String_Write_Function write_type;
    Type type;
};

STRING_WRITE_DEC(write_type) {
    auto format_info = &va_arg(*va_args, Type_Format_Info);
    
    auto type = &format_info->type;
    auto indirection = try_kind_of(type, indirection);
    if (indirection)
        type = indirection->type;
    
    switch (type->kind) {
        case_kind(Type, base_type) {
            write(allocator, output, S("%"), f(type->base_type.identifier));
        } break;
        
        case_kind(Type, structure_node) {
            write(allocator, output, S("%"), f(type->structure_node.structure.root.identifier));
        } break;
        
        CASES_COMPLETE;
    }
    
    if (indirection)
        write(allocator, output, S("%"), f_indent(indirection->level, '*'));
}

Type_Format_Info f(Type type) {
    return { write_type, type };
}

struct Function {
    string identifier;
    Declarations arguments;
    Types return_types;
    bool is_coroutine;
    u32 local_byte_count;
};

#define Template_Array_Name      Functions
#define Template_Array_Data_Type Function
#include "template_array.h"

struct Best_Token_Iterator {
    string *text_iterator;
    string best_token;
    string best_end;
    u32 best_index;
    u32 count;
};

Best_Token_Iterator begin_best_token(string *text_iterator) {
    return { text_iterator, {}, {}, u32_max };
}

void test_token(Best_Token_Iterator *iterator, string end) {
    bool found;
    auto a = get_token_until_first_in_set(*iterator->text_iterator, end, &found, false);
    if (found && ((iterator->best_index == u32_max) || (a.count < iterator->best_token.count))) {
        iterator->best_token = a;
        iterator->best_end = end;
        iterator->best_index = iterator->count;
    }
    
    iterator->count++;
}

bool end(Best_Token_Iterator *iterator, bool skip_end = true) {
    advance(iterator->text_iterator, iterator->best_token.count);
    
    if (skip_end)
        advance(iterator->text_iterator, iterator->best_end.count);
    
    return (iterator->best_token.count > 0);
}

Type * find_or_create_base_type(Memory_Allocator *allocator, string identifier, Types *types) {
    for_array_item(type, *types) {
        auto base_type = try_kind_of((*type), base_type);
        if (base_type) {
            if (base_type->identifier == identifier)
                return *type;
            
            continue;
        }
        
#if 0        
        auto structure = try_kind_of((*type), structure);
        if (structure) {
            auto scope = kind_of(structure, scope);
            if (scope->identifier == identifier)
                return *type;
            
            continue;
        }
#else
        auto structure_node = try_kind_of((*type), structure_node);
        if (structure_node) {
            if (structure_node->structure.root.identifier == identifier)
                return *type;
            
            continue;
        }
#endif
        
        UNREACHABLE_CODE;
    }
    
    auto result = ALLOCATE(allocator, Type);
    *grow(allocator, types) = result;
    *result = make_kind(Type, base_type, identifier, 0);
    
    return result;
}

Type * parse_type(Memory_Allocator *allocator, Text_Iterator *it, Types *types, String_Buffer *body) {
    string start = it->text;
    
    bool ok = false;
    defer { if (!ok) it->text = start; };
    
    u32 indirection_count = 0;
    while (try_skip(&it->text, S("*"))) {
        indirection_count++;
    }
    
    skip_space(it, body);
    
    auto identifier = get_identifier(&it->text, S("_"));
    if (!identifier.count) {
        return null;
    }
    
    ok = true;
    
    Type *base_type = find_or_create_base_type(allocator, identifier, types);
    
    if (indirection_count) {
        auto type = ALLOCATE(allocator, Type);
        *type = make_kind(Type, indirection, base_type, indirection_count);
        return type;
    }
    
    return base_type;
}

// does not care for comments or #if's
string skip_scope(Text_Iterator *it, string open_bracket, string closed_bracket, String_Buffer *body) {
    if (!try_skip(&it->text, open_bracket))
        return {};
    
    auto start = it->text;
    
    u32 count = 1;
    
    while (count) {
        auto token = begin_best_token(&it->text);
        test_token(&token, closed_bracket);
        test_token(&token, open_bracket);
        test_token(&token, S("\n"));
        end(&token);
        
        switch (token.best_index) {
            case 0: {
                count--;
            } break;
            
            case 1: {
                count++;
            } break;
            
            case 2: {
                it->line++;
                if (body)
                    write(body, S("#line %\r\n"), f(it->line));
            } break;
            
            CASES_COMPLETE;
        }
    }
    
    return sub_string(start, it->text);
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
    u32 max_byte_count;
    u32 byte_count;
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
                    assert(0, "you can not call coroutine as an expression within none-coroutine");
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
                    
                    byte_offset += byte_count_of(*it->type);
                }
            }
            
            if (best_declaration) {
                write(allocator, &result, S("CO_ENTRY(%, %)"), f(*best_declaration->type), f(best_byte_offset));
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
            byte_count += byte_count_of(*declaration->type);
        }
    }
    
    return byte_count;
}

// assuming return values are allways on top off the stack
// we return the offset from the end
u32 return_value_byte_offset_of(Function *function, u32 return_value_index) {
    assert(return_value_index < function->return_types.count);
    
    u32 byte_offset = 0;
    for (u32 i = 0; i < return_value_index; i++) {
        byte_offset += byte_count_of(*function->return_types[i]);
    }
    
    u32 byte_count = byte_offset;
    for (u32 i = return_value_index; i < function->return_types.count; i++) {
        byte_count += byte_count_of(*function->return_types[i]);
    }
    
    return byte_count - byte_offset;
}

u32 byte_count_of(Function function) {
    u32 byte_count = 0;
    
    for_array_item(it, function.arguments) {
        byte_count += byte_count_of(*it->type);
    }
    
    byte_count += function.local_byte_count;
    
    for_array_item(it, function.return_types) {
        byte_count += byte_count_of(**it);
    }
    
    return byte_count;
}

void write_read_coroutine_results(Memory_Allocator *allocator, String_Buffer *body, Function *subroutine, string_array left_expressions, Functions functions, Function *function, Scopes scopes) {
    write(body, S("{ // read results\r\n"));
    
    if (left_expressions.count) {
        for (u32 i = 0; i < subroutine->return_types.count; i++) {
            string expression_prime = replace_expression(allocator, left_expressions[i], functions, function, scopes);
            defer { free_array(allocator, &expression_prime); };
            
            write(body, S("% = CO_RESULT(%, %);\r\n"), f(expression_prime), f(*subroutine->return_types[i]), f(return_value_byte_offset_of(subroutine, i)));
        }
    }
    
    write(body, S("co_pop_coroutine(call_stack, %);\r\n"), f(byte_count_of(*subroutine) + sizeof(Coroutine_Function) + 2 * sizeof(u32)));
    
    write(body, S("}\r\n\r\n"));
}

void write_call_subroutine(Memory_Allocator *allocator, String_Buffer *switch_table, String_Buffer *body, Text_Iterator *it, Function *subroutine, string_array left_expressions, u32 *label_count, Functions functions, Function *function, Scopes scopes)
{
    assert(subroutine->is_coroutine);
    skip_space(it, body);
    
    assert(!left_expressions.count || (left_expressions.count == subroutine->return_types.count));
    
    {
        write(body, S("\r\n{ // call %\r\n"), f(subroutine->identifier));
        skip(&it->text, S("("));
        skip_space(it, body);
        
        write(body, S("CO_STATE_ENTRY = %;\r\n"), f(*label_count));
        
        for (u32 i = 0; i < subroutine->arguments.count; i++) {
            string expression;
            if (i == subroutine->arguments.count - 1) {
                expression = skip_until_first_in_set(it, S(")"), body, true);
            }
            else {
                expression = skip_until_first_in_set(it, S(","), body, true);
            }
            
            string expression_prime = replace_expression(allocator, expression, functions, function, scopes);
            defer { free_array(allocator, &expression_prime); };
            
            write(body, S("% arg% = %;\r\n"), f(*subroutine->arguments[i].type), f(i), f(expression_prime));
        }
        
        write(body, S("u8_array it = co_push_coroutine(call_stack, %, %);\r\n"), f(subroutine->identifier), f(byte_count_of(*subroutine) + 2 * sizeof(u32) + sizeof(Coroutine_Function)));
        
        for (u32 i = 0; i < subroutine->arguments.count; i++) {
            write(body, S("*next_item(&it, %) = arg%;\r\n"), f(*subroutine->arguments[i].type), f(i));
        }
        
        write(body, S("return Coroutine_Continue;\r\n"));
        write(body, S("}\r\n"));
    }
    
    add_label(switch_table, body, label_count);
    
    write_read_coroutine_results(allocator, body, subroutine, left_expressions, functions, function, scopes);
}

void write_signature(String_Buffer *output, Function function) {
    if (function.is_coroutine) {
        write(output, S("COROUTINE_DEC(%)"), f(function.identifier));
    }
    else {
        
        if (function.return_types.count) {
            write(output, S("% "), f(*function.return_types[0]));
        }
        else {
            write(output, S("void "));
        }
        
        write(output, S("%("), f(function.identifier));
        
        if (function.return_types.count > 1) {
            for (u32 i = 1; i < function.return_types.count; i++) {
                if (function.arguments.count || (i < function.return_types.count - 1))
                    write(output, S("% *_out%, "), f(*function.return_types[i]), f(i));
                else
                    write(output, S("% *_out%)"), f(*function.return_types[i]), f(i));
            }
        }
        
        for (u32 i = 0; i < function.arguments.count; i++) {
            if (i < function.arguments.count - 1)
                write(output, S("% %, "), f(*function.arguments[i].type), f(function.arguments[i].identifier));
            else
                write(output, S("% %)"), f(*function.arguments[i].type), f(function.arguments[i].identifier));
        }
        
        if ((function.arguments.count == 0) && (function.return_types.count <= 1))
            write(output, S(")"));
    }
}

string parse_next_expression(Text_Iterator *it, string end_token, bool *is_end, String_Buffer *body) {
    string start = it->text;
    
    LOOP {
        auto token = begin_best_token(&it->text);
        test_token(&token, S("("));
        test_token(&token, S(","));
        test_token(&token, end_token);
        test_token(&token, S("\n"));
        end(&token, false);
        
        switch (token.best_index) {
            case 0: {
                skip_scope(it, S("("), S(")"), body);
            } break;
            
            case 1: {
                string expression = sub_string(start, it->text);
                skip(&it->text, S(","));
                skip_space(it, body);
                *is_end = false;
                return expression;
            } break;
            
            case 2: {
                string expression = sub_string(start, it->text);
                skip(&it->text, end_token);
                skip_space(it, body);
                *is_end = true;
                return expression;
            } break;
            
            case 3: {
                skip(&it->text, S("\n"));
                if (body)
                    write(body, S("#line %\r\n"), f(it->line));
            } break;
            
            CASES_COMPLETE;
        }
    }
}

void parse_call_subroutine_or_expression(Memory_Allocator *allocator, String_Buffer *switch_table, String_Buffer *body, Text_Iterator *it, string_array left_expressions, u32 *label_count, Functions functions, Function *function, Scopes scopes) {
    auto test = *it;
    auto function_identifier = get_identifier(&test.text, S("_"));
    if (function_identifier.count) {
        skip_space(&test, body);
        
        auto subroutine = find_function(functions, function_identifier);
        if (subroutine) {
            *it = test;
            
            if (subroutine->is_coroutine) {
                write_call_subroutine(allocator, switch_table, body, it, subroutine, left_expressions, label_count, functions, function, scopes);
                skip(&it->text, S(";"));
            }
            else {
                bool use_dummy_returns = false;
                
                String_Buffer dummy_return_values = { allocator };
                defer { 
                    if (use_dummy_returns) {
                        free_array(allocator, &left_expressions);
                        free(&dummy_return_values);
                        write(body, S("}\r\n"));
                    }
                };
                
                if (!left_expressions.count && (subroutine->return_types.count > 1)) {
                    use_dummy_returns = true;
                    write(body, S("{\r\n"));
                    
                    for (u32 i = 1; i < subroutine->return_types.count; i++) {
                        *grow(allocator, &left_expressions) = write(&dummy_return_values, S("_ignored%"), f(i));
                        write(body, S("% _ignored%;\r\n"), f(*subroutine->return_types[i]), f(i));
                    }
                }
                
                assert(use_dummy_returns || (left_expressions.count == subroutine->return_types.count));
                
                skip(&it->text, S("("));
                skip_space(it, body);
                
                if (use_dummy_returns) {
                    write(body, S("%("), f(subroutine->identifier));
                }
                else {
                    string expression_prime = replace_expression(allocator, left_expressions[0], functions, function, scopes);
                    defer { free_array(allocator, &expression_prime); };
                    
                    write(body, S("% = %("), f(expression_prime), f(subroutine->identifier));
                }
                
                for (u32 i = 1; i < subroutine->return_types.count; i++) {
                    string expression;
                    if (use_dummy_returns)
                        expression = left_expressions[i - 1];
                    else
                        expression = left_expressions[i];
                    
                    string expression_prime = replace_expression(allocator, expression, functions, function, scopes);
                    defer { free_array(allocator, &expression_prime); };
                    
                    if (i == subroutine->return_types.count + subroutine->arguments.count - 1)
                        write(body, S("&(%)"), f(expression_prime));
                    else
                        write(body, S("&(%), "), f(expression_prime));
                }
                
                auto rest = skip_until_first_in_set(it, S(";"), body, true);
                
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
            auto expression = parse_next_expression(it, S(";"), &is_end, body);
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

bool parse_variable(Text_Iterator *it, Memory_Allocator *allocator, Declarations *declarations, String_Buffer *body, Types *types, Scope *scope = null) {
    if (!try_skip(it, S("var"), body))
        return false;
    
    LOOP {
        skip_space(it, body);
        
        auto identifier = get_identifier(&it->text, S("_"));
        assert(identifier.count);
        skip_space(it, body);
        
        skip(&it->text, S(":"));
        skip_space(it, body);
        
        Type *type = parse_type(allocator, it, types, body);
        assert(types);
        skip_space(it, body);
        
        if (scope) {
            scope->byte_count += byte_count_of(*type);
            scope->max_byte_count = MAX(scope->max_byte_count, scope->byte_count);
        }
        
        *grow(allocator, declarations) = { type, identifier };
        
        if (!try_skip(it, S(","), body)) {
            break;
        }
    }
    
    skip_until_first_in_set(it, S(";"), body, true);
    return true;
}

bool is_absolute_path(string path) {
    while (path.count) {
        u32 head = utf8_advance(&path);
        
        if (head == '\\') {
            return false;
        }
        
        if (head == ':') {
            return true;
        }
    }
    
    return false;
}

string to_forward_slashed_path(Memory_Allocator *allocator, string path) {
    string result = ALLOCATE_ARRAY_INFO(allocator, u8, path.count);
    copy(result, path, byte_count_of(path));
    auto it = result;
    
    while (it.count) {
        u32 byte_count;
        u32 head = utf8_head(it, &byte_count);
        
        if (head == '\\') {
            assert(byte_count == 1);
            it[0] = '/';
        }
        
        advance(&it, byte_count);
    }
    
    return result;
}

void write_tree(Memory_Allocator *allocator, Type_Node root) {
    write_line_out(allocator, S("TREE:\n\n"));
    // skip root, it contains no info
    auto current = &root;
    usize depth = 0;
    auto last_depth = depth;
    
    if (current) {
        auto type = current->type;
        
        if (type)
            write_line_out(allocator, S("root: %"), f_indent(depth * 0), f(*type));
        else
            write_line_out(allocator, S("root: no type"), f_indent(depth * 0));
        
        next(&current, &depth);
    }
    
    while (current) {
        auto type = current->type;
        
        string foo = {};
        defer { free_array(allocator, &foo); };
        
        auto test = current->parent;
        
        while (test && test->parent) {
            string indent;
            
            if (test->next)
                indent = S("| ");
            else
                indent = S("  ");
            
            grow(allocator, &foo, indent.count);
            copy(foo + indent.count, foo + 0, foo.count - indent.count);
            copy(foo + 0, indent + 0, indent.count);
            
            test = test->parent;
        }
        
        write_out(allocator, S("%"), f(foo));
        
        if (type)
            write_line_out(allocator, S("|-%"), f(*type));
        else
            write_line_out(allocator, S("|-no type"));
        
        next(&current, &depth);
    }
}

MAIN_DEC {
    Win32_Platform_API win32_api;
    init_win32_api(&win32_api);
    auto platform_api = &win32_api.platform_api;
    init_c_allocator();
    init_Memory_Growing_Stack_allocators();
    auto allocator = C_Allocator;
    
    auto transient_memory = make_memory_growing_stack(platform_api->allocator);
    auto transient_allocator = &transient_memory.allocator;
    
    string working_directory = platform_api->get_working_directory(platform_api);
    
    WRITE_LINE_OUT(transient_allocator, working_directory);
    
    if (argument_count < 2) {
        write_line_out(transient_allocator, S("requires a file path to be preprocessed as it's first parameter"));
        return 0;
    }
    
    string source_file_name = make_string(arguments[1]);
    string source_file_extension = {};
    // remove .source_file_extension
    while (source_file_name.count) {
        u32 byte_count;
        u32 tail = utf8_tail(source_file_name, &byte_count);
        
        source_file_name.count -= byte_count;
        if (tail == '.') {
            source_file_extension.data = one_past_last(source_file_name) + byte_count;
            break;
        }
        
        source_file_extension.count += byte_count;
    }
    
    string source_file_path = new_write(transient_allocator, S("%.%"), f(source_file_name), f(source_file_extension));
    
    auto source = platform_api->read_entire_file(source_file_path, allocator);
    if (!source.count) {
        write_line_out(transient_allocator, S("could not open \"%\""), f(source_file_path));
        return 0;
    }
    
    string preprocesor_path;
    {
        usize path_count = 0;
        for (u32 i = 0; arguments[0][i]; ++i) {
            if (arguments[0][i] == '\\')
                path_count = i;
        }
        
        preprocesor_path = { cast_p(u8, arguments[0]), path_count };
    }
    
    Types types = {};
    Functions functions = {};
    
#define add_base_type(name) { \
        auto type = ALLOCATE(allocator, Type); \
        *type = make_kind(Type, base_type, S(# name), sizeof(name)); \
        *grow(allocator, &types) = type; \
    }
    
    add_base_type(any);
    add_base_type(bool);
    
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
    add_base_type(Memory_Growing_Stack);
    
#undef add_base_type
    
    {
        Text_Iterator it = { source, 1 };
        String_Buffer *empty_body = null;
        while (it.text.count) {
            skip_space(&it, empty_body);
            
            if (try_skip(&it, S("def"), empty_body)) {
                skip_space(&it, empty_body);
                
                string identifier = get_identifier(&it.text, S("_"));
                assert(identifier.count);
                skip_space(&it, empty_body);
                
                bool is_coroutine = try_skip(&it, S("coroutine"), empty_body);
                if (is_coroutine || try_skip(&it, S("function"), empty_body)) {
                    skip_space(&it, empty_body);
                    
                    Function function = {};
                    function.identifier = identifier;
                    function.is_coroutine = is_coroutine;
                    
                    skip(&it.text, S("("));
                    
                    bool previous_was_comma = false;
                    LOOP {
                        skip_space(&it, empty_body);
                        
                        if (!previous_was_comma) {
                            if (try_skip(&it, S(")"), empty_body)) {
                                skip_space(&it, empty_body);
                                break;
                            }
                        }
                        
                        Declaration declaration = {};
                        
                        declaration.identifier = get_identifier(&it.text, S("_"));
                        assert(declaration.identifier.count);
                        skip_space(&it, empty_body);
                        
                        skip(&it.text, S(":"));
                        skip_space(&it, empty_body);
                        
                        declaration.type = parse_type(allocator, &it, &types, empty_body);
                        assert(declaration.type);
                        skip_space(&it, empty_body);
                        
                        *grow(allocator, &function.arguments) = declaration;
                        
                        previous_was_comma = try_skip(&it, S(","), empty_body);
                    }
                    
                    if (try_skip(&it, S("->"), empty_body)) {
                        skip_space(&it, empty_body);
                        
                        skip(&it.text, S("("));
                        
                        bool previous_was_comma = false;
                        LOOP {
                            skip_space(&it, empty_body);
                            
                            if (!previous_was_comma) {
                                if (try_skip(&it, S(")"), empty_body)) {
                                    skip_space(&it, empty_body);
                                    break;
                                }
                            }
                            
                            Type *type = parse_type(allocator, &it, &types, empty_body);
                            assert(type);
                            skip_space(&it, empty_body);
                            
                            *grow(allocator, &function.return_types) = type;
                            
                            previous_was_comma = try_skip(&it, S(","), empty_body);
                        }
                    }
                    
                    skip(&it.text, S("{"));
                    
                    Scopes scopes = {};
                    *grow(allocator, &scopes) = {};
                    
                    while (scopes.count)
                    {
                        skip_space(&it, empty_body);
                        
                        if (try_skip(&it, S("{"), empty_body)) {
                            *grow(allocator, &scopes) = {};
                            continue;
                        }
                        
                        if (try_skip(&it, S("}"), empty_body)) {
                            u32 scope_byte_count = scopes[scopes.count - 1].max_byte_count;
                            
                            free_array(allocator, &scopes[scopes.count - 1].declarations);
                            shrink(allocator, &scopes);
                            
                            if (scopes.count) {
                                scopes[scopes.count - 1].max_byte_count = MAX(scopes[scopes.count - 1].max_byte_count, scopes[scopes.count - 1].byte_count + scope_byte_count);
                            }
                            else {
                                function.local_byte_count = scope_byte_count;
                            }
                            continue;
                        }
                        
                        if (parse_variable(&it, allocator, &scopes[scopes.count - 1].declarations, empty_body, &types, scopes + scopes.count - 1))
                            continue;
                        
                        LOOP {
                            auto token = begin_best_token(&it.text);
                            test_token(&token, S("{"));
                            test_token(&token, S("}"));
                            test_token(&token, S(";"));
                            test_token(&token, S("\n"));
                            end(&token, false);
                            
                            assert(token.best_index < token.count);
                            
                            if (token.best_index == 3) {
                                skip_space(&it, empty_body);
                                it.line++;
                                continue;
                            }
                            
                            if (token.best_index == 2)
                                skip(&it.text, S(";"));
                            
                            break;
                        }
                        
                        continue;
                    }
                    
                    *grow(allocator, &functions) = function;
                    
                    continue;
                }
                
                if (try_skip(&it, S("struct"), empty_body)) {
                    skip_space(&it, empty_body);
                    
                    // this type could have been used by an other struct,
                    // so use the same type and just add dependencies and sizes
                    auto struct_type = find_or_create_base_type(allocator, identifier, &types);
                    // is not allready declared
                    assert(is_kind(*struct_type, base_type) && !struct_type->base_type.byte_count);
                    // turn it to a structure,
#if 0
                    *struct_type = make_kind(Type, structure);
#else
                    *struct_type = make_kind(Type, structure_node);
                    struct_type->structure_node.structure = make_kind(Structure, root, identifier);
#endif
                    
                    skip(&it.text, S("{"));
                    skip_space(&it, empty_body);
                    
                    assert(identifier.count);
                    //struct_type->structure = make_kind(Structure, scope, identifier);
                    auto current = &struct_type->structure_node;
                    
                    while (current) {
                        skip_space(&it, empty_body);
                        
                        if (try_skip(&it, S("}"), empty_body)) {
                            current = current->parent;
                            //current_scope = current_scope->scope.parent;
                            
                            continue;
                        }
                        
                        bool is_kind = try_skip(&it, S("kind"), empty_body);
                        bool is_var = !is_kind && try_skip(&it, S("var"), empty_body);
                        if (is_var || is_kind) {
                            skip_space(&it, empty_body);
                            
                            string identifier = get_identifier(&it.text, S("_"));
                            assert(identifier.count);
                            skip_space(&it, empty_body);
                            
                            skip(&it.text, S(":"));
                            skip_space(&it, empty_body);
                            
                            auto field_node = ALLOCATE(allocator, Structure_Node);
                            *field_node = {};
                            
                            if (try_skip(&it.text, S("struct"))) {
                                skip_space(&it, empty_body);
                                skip(&it.text, S("{"));
                                
                                field_node->structure = make_kind(Structure, sub_structure, identifier);
                                attach(current, field_node);
                                
                                current = field_node;
                            }
                            else {
                                Type *type = parse_type(allocator, &it, &types, empty_body);
                                assert(type);
                                
                                if (is_kind(*type, base_type) || is_kind(*type, structure_node))
                                    *grow(allocator, &struct_type->structure_node.structure.root.dependencies) = type;
                                
                                field_node->structure = make_kind(Structure, field);
                                field_node->structure.field = { type, identifier };
                                attach(current, field_node);
                                
                                skip_space(&it, empty_body);
                                skip(&it.text, S(";"));
                            }
                            
                            field_node->structure.is_kind = is_kind;
                            
                            continue;
                        }
                        
                        UNREACHABLE_CODE;
                    }
                    
                    continue;
                }
                
                UNREACHABLE_CODE
            } // "def"
            
            if (try_skip(&it, S("type"), empty_body)) {
                skip_space(&it, empty_body);
                
                Type *type = parse_type(allocator, &it, &types, empty_body);
                // its a new type
                assert(type && is_kind(*type, base_type) && !type->base_type.byte_count);
                skip_space(&it, empty_body);
                
                type->base_type.byte_count = PARSE_UNSIGNED_INTEGER(&it.text, 32);
                skip_space(&it, empty_body);
                skip(&it.text, S(";"));
                continue;
            }
            
            UNREACHABLE_CODE
        }
    }
    
    String_Buffer output = { allocator };
    
    Type_Node type_dependency_root = {};
    
    for_array_item(self, types) {
        Type_Node *iterator = &type_dependency_root;
        auto self_node = find_next_node(&iterator, *self);
        // make shure there are no double entries
        assert(!iterator || !find_next_node(&iterator, *self));
        
        if (!self_node) {
            self_node = ALLOCATE(allocator, Type_Node);
            *self_node = { *self };
            attach(&type_dependency_root, self_node);
        }
        
        Types *dependencies;
        switch ((*self)->kind) {
            case_kind(Type, base_type) {
                dependencies = &(*self)->base_type.dependencies;
            } break;
            
            case_kind(Type, structure_node) {
                dependencies = &(*self)->structure_node.structure.root.dependencies;
            } break;
            
            CASES_COMPLETE;
        }
        
        for_array_item(dependency, *dependencies) {
            assert(*dependency != *self);
            
            Type_Node *iterator = &type_dependency_root;
            auto dependency_node = find_next_node(&iterator, *dependency);
            // make shure there are no double entries
            assert(!iterator || !find_next_node(&iterator, *dependency));
            
            if (dependency_node && is_ancesotor(self_node, dependency_node)) {
                write_line_out(transient_allocator, S("ERROR cyclic struct dependency, % and % depend on each other"), f(**self), f(**dependency));
                
                write_tree(transient_allocator, type_dependency_root);
                assert(0, "circular dependency");
            }
            
            if (!dependency_node) {
                dependency_node = ALLOCATE(allocator, Type_Node);
                *dependency_node = { *dependency };
                
                attach(self_node->parent, dependency_node);
                move(dependency_node, self_node);
            }
            else if (!is_ancesotor(dependency_node, self_node)) {
                
                Type_Node *root = self_node;
                while (root->parent != &type_dependency_root) {
                    if (is_ancesotor(root->parent, dependency_node))
                        break;
                    
                    root = root->parent;
                }
                
                // move the all ancesotrs of self under dependency
                move(dependency_node, root);
            }
        }
    }
    
    { 
        write_line_out(transient_allocator, S("type dependencies\n\n"));
        write_tree(transient_allocator, type_dependency_root);
    }
    
    
    write(&output, S("#include <stdio.h>\r\n"));
    write(&output, S("#include \"basic.h\"\r\n"));
    write(&output, S("#include \"mo_string.h\"\r\n"));
    write(&output, S("#include \"memory_growing_stack.h\"\r\n\r\n"));
    
    // forward declare coroutines
    if (functions.count) {
        write(&output, S("#include \"preprocessor_coroutines.h\"\r\n\r\n"));
        
        for_array_item(function, functions) {
            write_signature(&output, *function);
            write(&output, S(";\r\n"));
        }
        
        write(&output, S("\r\n"));
    }
    else {
        
    }
    
    for_array_item(it, types) {
        auto structure = try_kind_of(*it, structure_node);
        
        if (structure)
            write(&output, S("struct %;\r\n"), f(**it));
    }
    
    write(&output, S("\r\n"));
    
    {
        // skip root, its does not contain a type
        auto current_type = type_dependency_root.children.head;
        
        while (current_type) {
            auto type = current_type->type;
            defer { next(&current_type); };
            
            if (!is_kind(*type, structure_node))
                continue;
            
            auto structure_node = kind_of(type, structure_node);
            
            assert(is_kind(structure_node->structure, root));
            
            write(&output, S("struct % {\r\n"), f(structure_node->structure.root.identifier));
            
            {
                // TODO: check if there are any kinds at all
                auto kind_enums = new_write_buffer(allocator, S("enum Kind {\r\nKind_null,\r\n"));
                defer{ free(&kind_enums); };
                
                auto field_buffer = new_write_buffer(allocator, S(""));
                defer{ free(&field_buffer); };
                
                auto kind_union = new_write_buffer(allocator, S("union {\r\n"));
                defer{ free(&kind_union); };
                
                auto current = structure_node;
                bool did_enter = true, did_leave = current && !current->children.head;
                
                bool parent_is_kind = false;
                
                while (current) {
                    defer { advance(&current, &did_enter, &did_leave); };
                    
                    if (is_kind(current->structure, sub_structure))
                        parent_is_kind = current->structure.is_kind;
                    
                    bool is_kind = current->structure.is_kind || parent_is_kind;
                    
                    switch (current->structure.kind) {
                        case_kind(Structure, root) {
                        } break;
                        
                        case_kind(Structure, sub_structure) {
                            if (did_enter) {
                                if (current->structure.is_kind) {
                                    write(&kind_enums, S("Kind_%,\r\n"), f(current->structure.sub_structure.identifier));
                                }
                                
                                if (is_kind) {
                                    write(&kind_union, S("struct {\r\n"));
                                }
                                else
                                    write(&field_buffer, S("struct {\r\n"));
                            }
                            
                            if (did_leave) {
                                if (is_kind)
                                    write(&kind_union, S("} %;\r\n\r\n"), f(current->structure.sub_structure.identifier));
                                else
                                    write(&field_buffer, S("} %;\r\n\r\n"), f(current->structure.sub_structure.identifier));
                            }
                        } break;
                        
                        case_kind(Structure, field) {
                            assert(did_enter && did_leave);
                            
                            if (current->structure.is_kind) {
                                write(&kind_enums, S("Kind_%,\r\n"), f(current->structure.field.identifier));
                            }
                            
                            if (is_kind) {
                                write(&kind_union, S("% %;\r\n"), f(*current->structure.field.type), f(current->structure.field.identifier));
                            }
                            else
                                write(&field_buffer, S("% %;\r\n"), f(*current->structure.field.type), f(current->structure.field.identifier));
                        } break;
                        
                        CASES_COMPLETE;
                    }
                }
                
                write(&kind_enums, S("Kind_Count,\r\n} kind;\r\n"));
                write(&kind_union, S("};\r\n"));
                
                write(&output, S("%\r\n%\r\n%};\r\n\r\n"), f(kind_enums.buffer), f(field_buffer.buffer), f(kind_union.buffer));
            }
        }
    }
    
    write(&output, S("\r\n"));
    
    {
        Text_Iterator it = { source, 1 };
        
        string file_path = to_forward_slashed_path(allocator, source_file_path);
        write(&output, S("#line 1 \"%\"\r\n\r\n"), f(file_path));
        
        while (it.text.count) {
            skip_space(&it, &output);
            
            if (try_skip(&it, S("def"), &output)) {
                skip_space(&it, &output);
                
                string identifier = get_identifier(&it.text, S("_"));
                assert(identifier.count);
                skip_space(&it, &output);
                
                if (try_skip(&it, S("struct"), &output)) {
                    skip_space(&it, &output);
                    
                    skip_scope(&it, S("{"), S("}"), &output);
                    
                    continue;
                }
                
                bool is_coroutine = try_skip(&it, S("coroutine"), &output);
                if (is_coroutine || try_skip(&it, S("function"), &output)) {
                    skip_space(&it, &output);
                    
                    auto function = find_function(functions, identifier);
                    write_signature(&output, *function);
                    write(&output, S(" {\r\n"));
                    
                    auto switch_table = new_write_buffer(allocator, S("switch (CO_STATE_ENTRY) {\r\n"));
                    defer { free(&switch_table); };
                    
                    String_Buffer body = { allocator };
                    defer { free(&body); };
                    
                    defer { 
                        if (function->is_coroutine) 
                            write(&output, S("%default:\r\nassert(0);\r\n}\r\n\r\n%"), f(switch_table.buffer), f(body.buffer)); 
                        else
                            write(&output, S("%\r\n\r\n"), f(body.buffer));
                    };
                    
                    Scopes scopes = {};
                    defer { free_array(allocator, &scopes); };
                    
                    *grow(allocator, &scopes) = {};
                    
                    for_array_item(argument, function->arguments) {
                        *grow(allocator, &scopes[scopes.count - 1].declarations) = *argument;
                        
                        if (function->is_coroutine)
                            write(&body, S("// argument: def %: %;\r\n"), f(argument->identifier), f(*argument->type));
                    }
                    
                    u32 label_count = 0;
                    
                    if (function->is_coroutine)
                        add_label(&switch_table, &body, &label_count);
                    
                    skip_until_first_in_set(&it, S("{"), &output, true);
                    
                    while (scopes.count) {
                        skip_space(&it, &body);
                        
                        while (try_skip(&it, S(";"), &body))
                            skip_space(&it, &body);
                        
                        if (try_skip(&it, S("}"), &body)) {
                            if (function->is_coroutine) {
                                if (scopes.count == 1)
                                    write(&body, S("\r\nassert(0);\r\nreturn Coroutine_Abort;\r\n"));
                            }
                            
                            free_array(allocator, &scopes[scopes.count - 1].declarations);
                            shrink(allocator, &scopes);
                            
                            write(&body, S("}\r\n\r\n"));
                            continue;
                        }
                        
                        if (try_skip(&it, S("{"), &body)) {
                            *grow(allocator, &scopes) = {};
                            
                            write(&body, S("{\r\n"));
                            continue;
                        }
                        
                        bool is_return = try_skip(&it, S("return"), &body);
                        
                        bool is_yield;
                        if (!is_return)
                            is_yield = try_skip(&it, S("yield"), &body);
                        else
                            is_yield = false;
                        
                        if (is_return || is_yield) {
                            skip_space(&it, &body);
                            
                            if (!function->is_coroutine) {
                                assert(!is_yield);
                                
                                if (!function->return_types.count) {
                                    skip(&it.text, S(";"));
                                    write(&body, S("return;"));
                                    continue;
                                }
                                
                                bool is_end;
                                string first_expression = parse_next_expression(&it, S(";"), &is_end, &body);
                                assert(is_end == (function->return_types.count == 1));
                                
                                for (u32 i = 1; i < function->return_types.count; i++) {
                                    bool is_end;
                                    string expression = parse_next_expression(&it, S(";"), &is_end, &body);
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
                                write(&body, S("{ // return\r\n"));
                            else
                                write(&body, S("{ // yield\r\n"));
                            
                            for (u32 i = 0; i < function->return_types.count; i++) {
                                bool is_end;
                                string expression = parse_next_expression(&it, S(";"), &is_end, &body);
                                assert(expression.count);
                                assert(is_end == (i == function->return_types.count - 1));
                                
                                string expression_prime = replace_expression(allocator, expression, functions, function, scopes);
                                defer { free_array(allocator, &expression_prime); };
                                
                                write(&body, S("CO_RESULT(%, %) = %;\r\n"), f(*function->return_types[i]), f(return_value_byte_offset_of(function, i)), f(expression_prime));
                            }
                            
                            if (is_return) {
                                write(&body, S("CO_STATE_ENTRY = u32_max;\r\n"));
                                write(&body, S("call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;\r\n"));
                                write(&body, S("return Coroutine_Continue;\r\n}\r\n\r\n"));
                            }
                            else {
                                
                                write(&body, S("CO_STATE_ENTRY = %;\r\n"), f(label_count));
                                write(&body, S("return Coroutine_Wait;\r\n}\r\n\r\n"));
                                
                                add_label(&switch_table, &body, &label_count);
                            }
                            
                            continue;
                        }
                        
                        if (try_skip(&it, S("var"), &body))
                        {
                            string_array left_expressions = {};
                            defer { free_array(allocator, &left_expressions); };
                            
                            LOOP {
                                skip_space(&it, &body);
                                
                                auto identifier = get_identifier(&it.text, S("_"));
                                assert(identifier.count);
                                skip_space(&it, &body);
                                
                                skip(&it.text, S(":"));
                                skip_space(&it, &body);
                                
                                Type *type = parse_type(allocator, &it, &types, &body);
                                assert(type);
                                skip_space(&it, &body);
                                
                                *grow(allocator, &scopes[scopes.count - 1].declarations) = { type, identifier };
                                *grow(allocator, &left_expressions) = identifier;
                                
                                if (!function->is_coroutine)
                                    write(&body, S("% %;\r\n"), f(*type), f(identifier));
                                
                                if (!try_skip(&it, S(","), &body)) {
                                    break;
                                }
                            }
                            
                            if (try_skip(&it, S("="), &body)) {
                                skip_space(&it, &body);
                                
                                if (try_skip(&it, S("run"), &body)) {
                                    skip_space(&it, &body);
                                    
                                    // get allocator expression
                                    
                                    bool is_end;
                                    auto allocator_expression = parse_next_expression(&it, S(";"), &is_end, &body);
                                    assert(!is_end);
                                    
                                    string subroutine_identifier = get_identifier(&it.text, S("_"));
                                    auto subroutine = find_function(functions, subroutine_identifier);
                                    assert(subroutine && subroutine->is_coroutine);
                                    skip_space(&it, &body);
                                    
                                    skip(&it.text, S("("));
                                    
                                    string_array arguments = {};
                                    defer { free_array(allocator, &arguments); };
                                    
                                    LOOP {
                                        skip_space(&it, &body);
                                        bool is_end;
                                        auto expression = parse_next_expression(&it, S(")"), &is_end, &body);
                                        
                                        *grow(allocator, &arguments) = expression;
                                        
                                        if (is_end) {
                                            skip_space(&it, &body);
                                            break;
                                        }
                                    }
                                    
                                    assert(arguments.count == subroutine->arguments.count);
                                    
                                    skip(&it.text, S(";"));
                                    
                                    write(&body, S("{\r\n"));
                                    
                                    {
                                        string allocator_expression_prime = replace_expression(allocator, allocator_expression, functions, function, scopes);
                                        defer { free_array(allocator, &allocator_expression_prime); };
                                        
                                        write(&body, S("Memory_Growing_Stack *_allocator = %;\r\n"), f(allocator_expression_prime));
                                        write(&body, S("Coroutine_Stack _call_stack = { _allocator };\r\n"));
                                    }
                                    
                                    // CO_ macros work on call_stack pointer
                                    write(&body, S("auto call_stack = &_call_stack;\r\n"));
                                    write(&body, S("auto it = co_push_coroutine(call_stack, %, %);\r\n"), f(subroutine->identifier), f(byte_count_of(*subroutine) + 2 * sizeof(u32) + sizeof(Coroutine_Function)));
                                    
                                    for (u32 i = 0; i < arguments.count; i++) {
                                        string argument_prime = replace_expression(allocator, arguments[i], functions, function, scopes);
                                        defer { free_array(allocator, &argument_prime); };
                                        
                                        write(&body, S("*next_item(&it, %) = %;\r\n"), f(*subroutine->arguments[i].type), f(argument_prime));
                                    }
                                    
                                    write(&body, S(
                                        "\r\n"
                                        "if (!run_without_yielding(call_stack)) {\r\n"
                                        "assert(0, \"todo: handle Coroutine_Abort\");\r\n"
                                        "}\r\n\r\n"
                                        ));
                                    
                                    write_read_coroutine_results(allocator, &body, subroutine, left_expressions, functions, function, scopes);
                                    
                                    write(&body, S("free_array(_allocator, &call_stack->buffer);\r\n"));
                                    
                                    write(&body, S("}\r\n"));
                                    
                                    // check scope later ...
                                    
                                    continue;
                                }
                                
                                parse_call_subroutine_or_expression(allocator, &switch_table, &body, &it, left_expressions, &label_count, functions, function, scopes);
                            }
                            else {
                                skip(&it.text, S(";"));
                            }
                            
                            continue;
                        }
                        
                        {
                            string start = it.text;
                            bool is_directive = try_skip(&it, S("if"), &body);
                            
                            if (!is_directive)
                                is_directive = try_skip(&it, S("switch"), &body);
                            
                            if (!is_directive)
                                is_directive = try_skip(&it, S("while"), &body);
                            
                            if (!is_directive)
                                is_directive = try_skip(&it, S("loop"), &body);
                            
                            if (is_directive) {
                                string directive = sub_string(start, it.text);
                                skip_space(&it, &body);
                                
                                string expression = skip_until_first_in_set(&it, S("{"), &body);
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
                
                UNREACHABLE_CODE;
            }
            
            if (try_skip(&it, S("type"), &output)) {
                skip_until_first_in_set(&it, S(";"), &output, true);
                continue;
            }
            
        }
    }
    
#if INCLUDE_TEST
    printf("factorial(3) = %u\n", run_factorial(&transient_memory, 3));
    s32 x;
    div(&x, 123, 13);
#else
    
    string out_file_path = new_write(allocator, S("%.cpp"), f(source_file_name));
    
    platform_api->write_entire_file(out_file_path, output.buffer);
    
    string mooselib_path;
    if (is_absolute_path(preprocesor_path)) {
        mooselib_path = new_write(allocator, S("%\\\\..\\\\..\\\\code"), f(preprocesor_path));
    }
    else {
        mooselib_path = new_write(allocator, S("%\\\\%\\\\..\\\\..\\\\code"), f(working_directory), f(preprocesor_path));
    }
    
    string command = new_write(allocator, S("cl /c \"%\" /I \"%\" /Zi /nologo /EHsc /link /INCREMENTAL:NO"), f(out_file_path), f(mooselib_path));
    
    u32 exit_code;
    bool ok;
    string message = platform_api->run_command(&exit_code, &ok, platform_api, command, allocator);
    
    printf("\n\n%.*s\n", FORMAT_S(&message));
    
#endif
    
    return 1;
}