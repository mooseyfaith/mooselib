
//TODO: coroutines need to use new struct sizes and alignment, probably just broken now
//TODO: list should not depend on its data_type, since only list::Entry depends on it
//      maybe by adding the abbility to include code

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
    
    
    auto test = *iterator->text_iterator;
    while (test.count) {
        u32 byte_count;
        u32 head = utf8_head(test, &byte_count);
        
        if (starts_with(test, end)) {
            found = true;
            break;
        }
        
        advance(&test, byte_count);
    }
    
    auto a = sub_string(*iterator->text_iterator, test);
    
    //auto a = get_token_until_first_in_set(*iterator->text_iterator, end, &found, false);
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

struct Tokenizer {
    Memory_Allocator *allocator;
    string_array expected_tokens;
    string text;
    string line_start;
    u32 line_count;
    u32 column_count;
};

enum Token_Kind {
    // keysymbols
    Token_Scope_Open,
    Token_Scope_Close,
    Token_Parenthesis_Open,
    Token_Parenthesis_Close,
    Token_Comma,
    Token_Semicolon,
    Token_Colon,
    Token_Doublecolon,
    Token_Star,
    Token_Equals,
    Token_Return_Arrow,
    
    // keywords
    Token_Var,
    Token_Def,
    Token_Function,
    Token_Coroutine,
    Token_Struct,
    Token_Type,
    Token_If,
    Token_Else,
    Token_Loop,
    Token_Return,
    Token_Continue,
    Token_Break,
    
    // ignored (debug)
    Token_Whitespace,
    Token_Newline,
    Token_Comment,
    
    // info
    Token_Identifier,
    Token_Operator,
    Token_String_Literal,
    Token_Number_Unsigned_Literal,
    Token_Number_Signed_Literal,
    Token_Number_Float_Literal,
    Token_Unparsed_Expression,
    
    // should never appear
    Token_Invalid,
};

struct Token {
    string text;
    u32 line_count;
    u32 column_count;
};

auto White_Space = S(" \t\r\0");
auto Operator_Symbols = S("+-*/%|&~<>=²³!?^");

struct Tokenizer_Test {
    Tokenizer *it;
    string text;
    string line_start;
    u32 line_count;
    u32 column_count;
};

Tokenizer_Test begin(Tokenizer *it) {
    return { it, it->text, it->line_start, it->line_count, it->column_count };
}

void skip_space(Tokenizer *it) {
    while (it->text.count) {
        u32 byte_count;
        u32 head = utf8_head(it->text, &byte_count);
        
        if (head == '\n') {
            it->line_count++;
            it->column_count = 1;
            advance(&it->text, byte_count);
            it->line_start = it->text;
            continue;
        }
        
        if (is_contained(head, White_Space)) {
            advance(&it->text, byte_count);
            it->column_count++;
            continue;
        }
        
        break;
    }
}

Token end(Tokenizer_Test test) {
    defer {
        while (test.text.data != test.it->text.data) {
            u32 head = utf8_advance(&test.text);
            if (head == '\n') {
                test.it->line_count++;
                test.it->column_count = 1;
                test.it->line_start = test.text;
            }
            else {
                test.it->column_count++;
            }
        }
        
        skip_space(test.it);
    };
    
    return { sub_string(test.text, test.it->text), test.line_count, test.column_count };
}

void revert(Tokenizer_Test test) {
    test.it->text         = test.text;
    test.it->line_start   = test.line_start;
    test.it->line_count   = test.line_count;
    test.it->column_count = test.column_count;
}

Tokenizer make_tokenizer(Memory_Allocator *allocator, string source) {
    Tokenizer result;
    result.allocator = allocator;
    result.expected_tokens = {};
    result.text = source;
    result.line_start = source;
    result.line_count = 1;
    result.column_count = 1;
    
    skip_space(&result);
    
    return result;
}

struct Report_Context {
    Memory_Allocator *allocator;
    string file_path;
    Tokenizer *tokenizer;
};

void write_file_position(Memory_Allocator *allocator, string *output, string file_path, u32 line_count, u32 column_count) {
    write(allocator, output, S("%(%:%)"), f(file_path), f(line_count), f(column_count));
}

void write_marked_line(Memory_Allocator *allocator, string *output, string line, u32 line_count, u32 column_count, u32 indent_count)
{
    string pretty_line = {};
    free_array(allocator, &pretty_line);
    
    u32 pretty_column_count = 0;
    
    assert(column_count);
    column_count--;
    
    while (line.count) {
        u32 byte_count;
        u32 head = utf8_head(line, &byte_count);
        
        if (head == '\t') {
            write(allocator, &pretty_line, S("  "));
            
            if (column_count)
                pretty_column_count += 2;
        }
        else {
            write(allocator, &pretty_line, S("%"), f(sub(line, 0, byte_count)));
            
            if (column_count)
                pretty_column_count++;
        }
        
        advance(&line, byte_count);
        if (column_count)
            column_count--;
    }
    
    write(allocator, output, S("%: %%"), f(line_count), f_indent(indent_count), f(pretty_line));
    
    if (indent_count + pretty_column_count > 0)
        write(allocator, output, S("%: %~\n"), f(line_count), f_indent(indent_count + pretty_column_count));
    else
        write(allocator, output, S("%: ~\n"), f(line_count));
}


void report(Report_Context *context, u32 line_count, u32 column_count, string format, ...) {
    auto line_end = context->tokenizer->line_start;
    skip_until_first_in_set(&line_end, S("\n"), true);
    auto line = sub_string(context->tokenizer->line_start, line_end);
    
    string output = {};
    write_file_position(context->allocator, &output, context->file_path, line_count, column_count);
    write_out(context->allocator, S("% error: "), f(output));
    free_array(context->allocator, &output);
    
    va_list va_args;
    va_start(va_args, format);
    write_out_va(context->allocator, format, va_args);
    va_end(va_args);
    
    write_out(context->allocator, S("\n\n"));
    
    write_marked_line(context->allocator, &output, line, line_count, column_count, 2);
    write_out(context->allocator, S("%"), f(output));
    free_array(context->allocator, &output);
    
    UNREACHABLE_CODE; // easier to debug
    exit(0);
}

#define EXPECT(context, condition, format, ...) \
if (!(condition)) { \
    report(context, (context)->tokenizer->line_count, (context)->tokenizer->column_count, format, __VA_ARGS__); \
}

bool try_skip(Tokenizer *it, string pattern) {
    auto test = begin(it);
    
    if (try_skip(&it->text, pattern)) {
        end(test);
        free_array(it->allocator, &it->expected_tokens);
        
        return true;
    }
    
    revert(test);
    *grow(it->allocator, &it->expected_tokens) = pattern;
    
    return false;
}

void skip(Report_Context *context, string pattern) {
    bool ok = try_skip(context->tokenizer, pattern);
    
    if (!ok) {
        report(context, context->tokenizer->line_count, context->tokenizer->column_count, S("expected '%'"), f(pattern));
        // report will force exit
    }
    
    free_array(context->tokenizer->allocator, &context->tokenizer->expected_tokens);
}

Token get_identifier(Report_Context *context) {
    auto test = begin(context->tokenizer);
    string identifier = get_identifier(&context->tokenizer->text, S("_"));
    EXPECT(context, identifier.count, S("expected identifier"));
    
    return end(test);
}

void expected_tokens(Report_Context *context) {
    string output = {};
    defer { free_array(context->allocator, &output); };
    
    write(context->allocator, &output, S("unexpected token, where expecting:\n\n"));
    
    for_array_item(token, context->tokenizer->expected_tokens) {
        if (token == one_past_last(context->tokenizer->expected_tokens) - 1)
            write(context->allocator, &output, S("    '%'\n"), f(*token));
        else
            write(context->allocator, &output, S("    '%' or\n"), f(*token));
    }
    
    report(context, context->tokenizer->line_count, context->tokenizer->column_count, S("%"), f(output));
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
    Token identifier;
    Token unparsed_expression;
};

#define Template_Array_Name      Declarations
#define Template_Array_Data_Type Declaration
#include "template_array.h"



struct Ast;

#define Template_Array_Name      Ast_Array
#define Template_Array_Data_Type Ast*
#include "template_array.h"

struct Ast {
    enum Kind {
        Kind_null,
        
        Kind_definition,
        Kind_declaration,
        
        Kind_temporary_declarations,
        
        Kind_structure,
        Kind_structure_kind,
        
        Kind_function,
        
        Kind_comment,
        
        Kind_return_statement,
        Kind_yield_statement,
        
        Kind_break_statement, 
        Kind_continue_statement,
        
        Kind_conditional_branch,
        Kind_conditional_loop,
        
        Kind_scope, 
        Kind_loop,
        
        Kind_unparsed_expression,
        
        Kind_Count,
    } kind;
    
    TEMPLATE_TREE_DEC(Ast);
    
    union {
        struct {
            Token identifier;
            Type *type;
            Ast *unparsed_expression;
        } declaration, structure_kind;
        
        struct {
            bool as_structure_kinds;
            
            enum State {
                State_Parse_Identifier,
                State_Check_Continue,
                State_Parse_Expressions,
            } state;
            
        } temporary_declarations;
        
        struct {
            Token identifier;
        } definition;
        
        struct {
            Type *type;
        } structure;
        
        struct {
            Ast *parent_function;
            bool is_coroutine;
            Ast_Array arguments;
            Types return_types;
            Ast *body;
        } function;
        
        Token comment;
        
        struct {} return_statement, yield_statement;
        
        struct {} break_statement, continue_statement;
        
        struct {
        } conditional_branch;
        
        struct {
            Ast *parent_loop;
        } loop, conditional_loop;
        
        struct {
            Ast *parent_scope;
        } scope;
        
        Token unparsed_expression;
    };
};

#define Template_Tree_Name      Ast
#define Template_Tree_Struct_Is_Declared
#include "template_tree.h"

struct Structure {
    enum Kind {
        Kind_null,
        Kind_def_structure,
        Kind_var_structure,
        Kind_field,
        Kind_Count
    } kind;
    
    TEMPLATE_TREE_DEC(Structure);
    
    bool is_kind;
    
    union {
        struct {
            Token identifier;
            u32 byte_count;
            u32 byte_alignment;
            
            u32 max_kind_byte_count;
            u32 max_kind_byte_alignment;
            u32 kind_count;
            
            Types dependencies;
        } var_structure, def_structure;
        
        Declaration field;
    };
};

#define Template_Tree_Name      Structure
#define Template_Tree_Struct_Is_Declared
#include "template_tree.h"

void align_to(u32 *byte_count, u32 byte_alignment) {
    if (byte_alignment)
        (*byte_count) += (byte_alignment - (*byte_count % byte_alignment)) % byte_alignment;
}

void update_byte_count_and_alignment(Structure *node, u32 child_byte_count, u32 child_byte_alignment) {
    assert(!is_kind(*node, field));
    
    node->def_structure.byte_alignment = MAX(node->def_structure.byte_alignment, child_byte_alignment);
    align_to(&node->def_structure.byte_count, child_byte_alignment);
    node->def_structure.byte_count += child_byte_count;
}

// yes an array of an array of Type*
// used for depth first search iteration
#define Template_Array_Name      Types_Array
#define Template_Array_Data_Type Types
#include "template_array.h"

struct Ast;

struct Type {
    enum Kind {
        Kind_null,
        Kind_undeclared_type,
        Kind_base_type,
        Kind_indirection,
        Kind_structure,
        
        Kind_list,
        
        Kind_Count,
    } kind;
    
    union {
        struct {
            Token identifier;
            Types dependencies;
        } undeclared_type;
        
        struct {
            Token identifier;
            u32 byte_count;
            u32 byte_alignment;
        } base_type;
        
        struct {
            Type *type;
            u32 level;
        } indirection;
        
        struct {
            Ast *node;
            Types dependencies;
        } structure;
        
        struct {
            Token identifier;
            Declaration entry_data;
            Type *count_type;
            bool with_tail, with_double_links;
            u32 byte_count, byte_alignment;
        } list;
    };
};

u32 byte_count_and_alignment_of(u32 *out_byte_alignment, Type type) {
    switch (type.kind) {
        case_kind(Type, base_type) {
            assert(type.base_type.byte_count);
            assert(type.base_type.byte_alignment);
            *out_byte_alignment = type.base_type.byte_alignment;
            return type.base_type.byte_count;
        }
        
#if 0        
        case_kind(Type, structure_node) {
            assert(type.structure->def_structure.byte_count);
            assert(type.structure->def_structure.byte_alignment);
            *out_byte_alignment = type.structure_node.def_structure.byte_alignment;
            return type.structure_node.def_structure.byte_count;
        }
#endif
        
        case_kind(Type, list) {
            assert(type.list.byte_count);
            assert(type.list.byte_alignment);
            *out_byte_alignment = type.list.byte_alignment;
            return type.list.byte_count;
        }
        
        case_kind(Type, indirection) {
            // all pointers have same byte_count and byte_alignment
            *out_byte_alignment = alignof(u8*);
            return sizeof(u8*);
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
        case_kind(Type, undeclared_type) {
            write(allocator, output, S("%"), f(type->undeclared_type.identifier.text));
        } break;
        
        case_kind(Type, base_type) {
            write(allocator, output, S("%"), f(type->base_type.identifier.text));
        } break;
        
        case_kind(Type, structure) {
            auto definition_node = type->structure.node->parent;
            assert(definition_node && is_kind(*definition_node, definition));
            
            write(allocator, output, S("%"), f(definition_node->definition.identifier.text));
        } break;
        
        case_kind(Type, list) {
            write(allocator, output, S("%"), f(type->list.identifier.text));
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
    Token identifier;
    bool is_coroutine;
    Declarations arguments;
    Types return_types;
    Structure local_variables;
};

#define Template_Array_Name      Functions
#define Template_Array_Data_Type Function*
#include "template_array.h"

Type * find_or_create_base_type(Memory_Allocator *allocator, string identifier, Types *types, bool *is_new_type = null)
{
    if (is_new_type)
        *is_new_type = false;
    
    for_array_item(type, *types) {
        auto undeclared_type = try_kind_of(*type, undeclared_type);
        if (undeclared_type) {
            if (undeclared_type->identifier.text == identifier)
                return *type;
            
            continue;
        }
        
        auto base_type = try_kind_of((*type), base_type);
        if (base_type) {
            if (base_type->identifier.text == identifier)
                return *type;
            
            continue;
        }
        
        auto structure = try_kind_of((*type), structure);
        if (structure) {
            auto definition_node = structure->node->parent;
            if (definition_node && is_kind(*definition_node, definition) && definition_node->definition.identifier.text == identifier)
                return *type;
            
            continue;
        }
        
        auto list = try_kind_of(*type, list);
        if (list) {
            if (list->identifier.text == identifier)
                return *type;
            
            continue;
        }
        
        UNREACHABLE_CODE;
    }
    
    if (is_new_type)
        *is_new_type = true;
    
    auto result = ALLOCATE(allocator, Type);
    *grow(allocator, types) = result;
    *result = make_kind(Type, undeclared_type, identifier);
    
    return result;
}

Type * parse_type(Memory_Allocator *allocator, Report_Context *context, Types *types, bool is_expected = false)
{
    auto test = begin(context->tokenizer);
    
    u32 indirection_count = 0;
    while (try_skip(&context->tokenizer->text, S("*"))) {
        skip_space(context->tokenizer);
        indirection_count++;
    }
    
    auto identifier = get_identifier(&context->tokenizer->text, S("_"));
    if (!identifier.count) {
        EXPECT(context, !is_expected, S("expected type"));
        revert(test);
        return null;
    }
    
    end(test);
    
    Type *base_type = find_or_create_base_type(allocator, identifier, types);
    
    if (indirection_count) {
        auto type = ALLOCATE(allocator, Type);
        *type = make_kind(Type, indirection, base_type, indirection_count);
        return type;
    }
    
    return base_type;
}


Function * find_function(Functions functions, string identifier) {
    for_array_item(function, functions) {
        if ((*function)->identifier.text == identifier)
            return *function;
    }
    
    return null;
}

string replace_expression(Memory_Allocator *allocator, string expression, Functions functions, Function *function, Structure *function_scope) {
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
            
            // search nearest declaration of identfier and its byte_offset in the call stack
            {
                auto current = function_scope;
                u32 byte_offset = 0;
                Declaration *found = null;
                while (current) {
                    defer { 
                        if (current->prev)
                            current = current->prev;
                        else
                            current = current->parent;
                    };
                    
                    auto field = try_kind_of(current, field);
                    if (field) {
                        if (!found && (field->identifier.text == identifier)) {
                            found = field;
                        }
                        
                        u32 byte_alignment;
                        byte_offset += byte_count_and_alignment_of(&byte_alignment, *field->type);
                    }
                }
                
                for_array_item(argument, function->arguments) {
                    if (!found && (argument->identifier.text == identifier)) {
                        found = argument;
                    }
                    
                    u32 byte_alignment;
                    byte_offset += byte_count_and_alignment_of(&byte_alignment, *argument->type);
                }
                
                if (found) {
                    write(allocator, &result, S("CO_ENTRY(%, %)"), f(*found->type), f(byte_offset));
                }
                else {
                    write(allocator, &result, S("%"), f(identifier));
                }
            }
        }
        else {
            while (expression.count) {
                u32 byte_count;
                u32 head = utf8_head(expression, &byte_count);
                
                if (head == '"') {
                    string text = parse_quoted_string(&expression);
                    write(allocator, &result, S("\"%\""), f(text));
                    continue;
                }
                
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

// assuming return values are allways on top off the stack
// we return the offset from the end
u32 return_value_byte_offset_of(Function *function, u32 return_value_index) {
    assert(return_value_index < function->return_types.count);
    
    u32 byte_offset = 0;
    for (u32 i = 0; i < return_value_index; i++) {
        u32 byte_alignment;
        byte_offset += byte_count_and_alignment_of(&byte_alignment, *function->return_types[i]);
    }
    
    u32 byte_count = byte_offset;
    for (u32 i = return_value_index; i < function->return_types.count; i++) {
        u32 byte_alignment;
        byte_count += byte_count_and_alignment_of(&byte_alignment, *function->return_types[i]);
    }
    
    return byte_count - byte_offset;
}

u32 byte_count_of(Function function) {
    u32 byte_count = 0;
    u32 byte_alignment = 1;
    
    for_array_item(it, function.arguments) {
        u32 byte_alignment;
        byte_count += byte_count_and_alignment_of(&byte_alignment, *it->type);
    }
    
    //HACK its a hack :D
    byte_count += function.local_variables.def_structure.max_kind_byte_count;
    
    for_array_item(it, function.return_types) {
        u32 byte_alignment;
        byte_count += byte_count_and_alignment_of(&byte_alignment, **it);
    }
    
    return byte_count;
}

void write_read_coroutine_results(Memory_Allocator *allocator, String_Buffer *body, Function *subroutine, string_array left_expressions, Functions functions, Function *function, Structure *function_scope) {
    write(body, S("{ // read results\r\n"));
    
    if (left_expressions.count) {
        for (u32 i = 0; i < subroutine->return_types.count; i++) {
            string expression_prime = replace_expression(allocator, left_expressions[i], functions, function, function_scope);
            defer { free_array(allocator, &expression_prime); };
            
            write(body, S("% = CO_RESULT(%, %);\r\n"), f(expression_prime), f(*subroutine->return_types[i]), f(return_value_byte_offset_of(subroutine, i)));
        }
    }
    
    write(body, S("co_pop_coroutine(call_stack, %);\r\n"), f(sizeof(Coroutine_Header) + byte_count_of(*subroutine)));
    
    write(body, S("}\r\n\r\n"));
}

#if 0
void write_call_subroutine(Memory_Allocator *allocator, String_Buffer *switch_table, String_Buffer *body, Text_Iterator *it, Function *subroutine, string_array left_expressions, u32 *label_count, Functions functions, Function *function, Structure *function_scope)
{
    assert(subroutine->is_coroutine);
    skip_space(it, body);
    
    assert(!left_expressions.count || (left_expressions.count == subroutine->return_types.count));
    
    {
        write(body, S("\r\n{ // call %\r\n"), f(subroutine->identifier.text));
        skip(&it->text, S("("));
        skip_space(it, body);
        
        write(body, S("CO_HEADER.current_label_index = %;\r\n"), f(*label_count));
        
        for (u32 i = 0; i < subroutine->arguments.count; i++) {
            string expression;
            if (i == subroutine->arguments.count - 1) {
                expression = skip_until_first_in_set(it, S(")"), body, true);
            }
            else {
                expression = skip_until_first_in_set(it, S(","), body, true);
            }
            
            string expression_prime = replace_expression(allocator, expression, functions, function, function_scope);
            defer { free_array(allocator, &expression_prime); };
            
            write(body, S("% arg% = %;\r\n"), f(*subroutine->arguments[i].type), f(i), f(expression_prime));
        }
        
        write(body, S("u8_array it = co_push_coroutine(call_stack, %, %);\r\n"), f(subroutine->identifier.text), f(sizeof(Coroutine_Header) + byte_count_of(*subroutine)));
        
        for (u32 i = 0; i < subroutine->arguments.count; i++) {
            write(body, S("*next_item(&it, %) = arg%;\r\n"), f(*subroutine->arguments[i].type), f(i));
        }
        
        write(body, S("return Coroutine_Continue;\r\n"));
        write(body, S("}\r\n"));
    }
    
    add_label(switch_table, body, label_count);
    
    write_read_coroutine_results(allocator, body, subroutine, left_expressions, functions, function, function_scope);
}

void write_signature(String_Buffer *output, Function function) {
    if (function.is_coroutine) {
        write(output, S("COROUTINE_DEC(%)"), f(function.identifier.text));
    }
    else {
        
        if (function.return_types.count) {
            write(output, S("% "), f(*function.return_types[0]));
        }
        else {
            write(output, S("void "));
        }
        
        write(output, S("%("), f(function.identifier.text));
        
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
                write(output, S("% %, "), f(*function.arguments[i].type), f(function.arguments[i].identifier.text));
            else
                write(output, S("% %)"), f(*function.arguments[i].type), f(function.arguments[i].identifier.text));
        }
        
        if ((function.arguments.count == 0) && (function.return_types.count <= 1))
            write(output, S(")"));
    }
}
#endif

#if 0
void parse_call_subroutine_or_expression(Memory_Allocator *allocator, String_Buffer *switch_table, String_Buffer *body, Text_Iterator *it, string_array left_expressions, u32 *label_count, Functions functions, Function *function, Structure *function_scope) {
    auto test = *it;
    auto function_identifier = get_identifier(&test.text, S("_"));
    if (function_identifier.count) {
        skip_space(&test, body);
        
        auto subroutine = find_function(functions, function_identifier);
        if (subroutine) {
            *it = test;
            
            if (subroutine->is_coroutine) {
                write_call_subroutine(allocator, switch_table, body, it, subroutine, left_expressions, label_count, functions, function, function_scope);
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
                    write(body, S("%("), f(subroutine->identifier.text));
                }
                else {
                    string expression_prime = replace_expression(allocator, left_expressions[0], functions, function, function_scope);
                    defer { free_array(allocator, &expression_prime); };
                    
                    write(body, S("% = %("), f(expression_prime), f(subroutine->identifier.text));
                }
                
                for (u32 i = 1; i < subroutine->return_types.count; i++) {
                    string expression;
                    if (use_dummy_returns)
                        expression = left_expressions[i - 1];
                    else
                        expression = left_expressions[i];
                    
                    string expression_prime = replace_expression(allocator, expression, functions, function, function_scope);
                    defer { free_array(allocator, &expression_prime); };
                    
                    if (i == subroutine->return_types.count + subroutine->arguments.count - 1)
                        write(body, S("&(%)"), f(expression_prime));
                    else
                        write(body, S("&(%), "), f(expression_prime));
                }
                
                auto rest = skip_until_first_in_set(it, S(";"), body, true);
                
                string rest_prime = replace_expression(allocator, rest, functions, function, function_scope);
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
            
            string expression_prime = replace_expression(allocator, expression, functions, function, function_scope);
            defer { free_array(allocator, &expression_prime); };
            
            if (left_expressions.count) {
                string left_expression_prime = replace_expression(allocator, left_expressions[left_index], functions, function, function_scope);
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
#endif

Token next_expression(Memory_Allocator *allocator, Report_Context *context, string terminal = S(",")) {
    
    auto test = begin(context->tokenizer);
    u32 paranthesis_depth = 0;
    u32 brackets_depth = 0;
    
    string_array close_tokens = {};
    defer { free_array(allocator, &close_tokens); };
    
    *grow(allocator, &close_tokens) = terminal;
    
    while (close_tokens.count) {
        auto best = begin_best_token(&context->tokenizer->text);
        test_token(&best, S("("));
        test_token(&best, S("{"));
        test_token(&best, close_tokens[close_tokens.count - 1]);
        end(&best, false);
        
        switch (best.best_index) {
            case 0: {
                *grow(allocator, &close_tokens) = S(")");
                skip(context, S("("));
            } break;
            
            case 1: {
                *grow(allocator, &close_tokens) = S("}");
                skip(context, S("{"));
            } break;
            
            case 2: {
                if (close_tokens.count > 1)
                    skip(context, close_tokens[close_tokens.count - 1]);
                
                shrink(allocator, &close_tokens);
            } break;
            
            default: {
                revert(test);
                
                EXPECT(context, 0, S(
                    "unexpected end of file, expected:\n\n"
                    "'(' or\n"
                    "'{' or\n"
                    "%\n"
                    ), f(close_tokens[close_tokens.count - 1]));
            }
        }
    }
    
    auto result = end(test);
    skip(context, terminal);
    
    return result;
}

bool parse_variables(Report_Context *context, Memory_Allocator *allocator, Declarations *declarations, Types *types) {
    if (!try_skip(context->tokenizer, S("var")))
        return false;
    
    bool parse_expressions = false;
    
    u32 offset = declarations->count;
    u32 declaration_count = 0;
    while (context->tokenizer->text.count) {
        declaration_count++;
        auto identifier = get_identifier(context);
        
        skip(context, S(":"));
        
        Type *type = parse_type(allocator, context, types, true);
        skip_space(context->tokenizer);
        
        *grow(allocator, declarations) = { type, identifier };
        
        if (try_skip(context->tokenizer, S("="))) {
            parse_expressions = true;
            break;
        }
        
        if (try_skip(context->tokenizer, S(";")))
            break;
        
        if (try_skip(context->tokenizer, S(",")))
            continue;
        
        expected_tokens(context);
    }
    
    if (parse_expressions) {
        // search unparse , seperated expressions
        for (u32 i = 0; i < declaration_count; i++) {
            
            auto test = begin(context->tokenizer);
            u32 paranthesis_depth = 0;
            u32 brackets_depth = 0;
            
            string_array close_tokens = {};
            defer { free_array(allocator, &close_tokens); };
            
            if (i == declaration_count - 1)
                *grow(allocator, &close_tokens) = S(";");
            else
                *grow(allocator, &close_tokens) = S(",");
            
            while (close_tokens.count) {
                auto best = begin_best_token(&context->tokenizer->text);
                test_token(&best, S("("));
                test_token(&best, S("{"));
                test_token(&best, close_tokens[close_tokens.count - 1]);
                end(&best, false);
                
                switch (best.best_index) {
                    case 0: {
                        *grow(allocator, &close_tokens) = S(")");
                        skip(context, S("("));
                    } break;
                    
                    case 1: {
                        *grow(allocator, &close_tokens) = S("}");
                        skip(context, S("{"));
                    } break;
                    
                    case 2: {
                        if (close_tokens.count > 1)
                            skip(context, close_tokens[close_tokens.count - 1]);
                        
                        shrink(allocator, &close_tokens);
                    } break;
                    
                    default: {
                        EXPECT(context, 0, S(
                            "unexpected end of file, expected:\n\n"
                            "'(' or\n"
                            "'{' or\n"
                            "%\n"
                            ), f(close_tokens[close_tokens.count - 1]));
                    }
                }
            }
            
            (*declarations)[offset + i].unparsed_expression = end(test);
        }
    }
    
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
    copy(result.data, path.data, byte_count_of(path));
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
            copy(foo.data + indent.count, foo.data, foo.count - indent.count);
            copy(foo.data, indent.data, indent.count);
            
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

Ast * parse_struct(Memory_Allocator *allocator, Report_Context *context, Token identifier = {}, Types *types = null) {
    if (!try_skip(context->tokenizer, S("struct")))
        return null;
    
    skip(context, S("{"));
    
    Type *struct_type;
    if (types) {
        // this type could have been used by an other struct,
        // so use the same type and just add dependencies and sizes
        struct_type = find_or_create_base_type(allocator, identifier.text, types);
        // is not allready declared
        EXPECT(context, is_kind(*struct_type, undeclared_type), S("type allready defined, here .__.\n"));
    }
    else {
        // anonymous struct, will not show up in types
        struct_type = ALLOCATE(allocator, Type);
    }
    
    auto structure_node = ALLOCATE(allocator, Ast);
    *structure_node = make_kind(Ast, structure, struct_type);
    // turn it to a structure type
    *struct_type = make_kind(Type, structure, structure_node);
    
    return structure_node;
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
    
    bool do_compile = false;
    if (argument_count >= 2) {
        string argument = make_string(arguments[2]);
        do_compile = (argument == S("-c"));
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
        *type = make_kind(Type, base_type, { S(# name) }, sizeof(name), alignof(name)); \
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
    
    add_base_type(Win32_Platform_API);
    add_base_type(Platform_API);
    
#undef add_base_type
    
    {
        Tokenizer it = make_tokenizer(allocator, source);
        Report_Context reporter = { allocator, source_file_path, &it };
        
        Ast *root = ALLOCATE(allocator, Ast);
        // TODO: add filescope? or/and global scope?
        *root = make_kind(Ast, scope);
        auto parent = root;
        
        Ast *top_function_node = null;
        Ast *top_loop_node = null;
        Ast *top_scope_node = null;
        
        while (it.text.count) {
            
            if (parent->parent && try_skip(&it, S("}"))) {
                bool do_continue = true;
                while (do_continue) {
                    parent = parent->parent;
                    
                    switch (parent->kind) {
                        case_kind(Ast, function) {
                            top_function_node = parent->function.parent_function;
                        } break;
                        
                        case_kind(Ast, conditional_loop);
                        case_kind(Ast, loop) {
                            top_loop_node  = parent->loop.parent_loop;
                        } break;
                        
                        case_kind(Ast, scope) {
                            top_scope_node = parent->scope.parent_scope;
                            do_continue = false;
                        } break;
                        
                        case_kind(Ast, temporary_declarations);
                        case_kind(Ast, structure) {
                            do_continue = false;
                        } break;
                    }
                }
                
                continue;
            }
            
            //assert(!parent || is_kind(*parent, scope) || is_kind(*parent, structure) || is_kind(*parent, declarations));
            
            if (is_kind(*parent, temporary_declarations)) {
                bool do_cleanup = false;
                
                switch (parent->temporary_declarations.state) {
                    case parent->temporary_declarations.State_Parse_Identifier: {
                        auto identifier = get_identifier(&reporter);
                        
                        skip(&reporter, S(":"));
                        
                        auto node = ALLOCATE(allocator, Ast);
                        if (parent->temporary_declarations.as_structure_kinds)
                            *node = make_kind(Ast, structure_kind, identifier);
                        else
                            *node = make_kind(Ast, declaration, identifier);
                        
                        // just temporary while processing
                        attach(parent, node);
                        
                        // create anonymous struct and its type
                        auto structure_node = parse_struct(allocator, &reporter);
                        if (structure_node) {
                            // just temporary while processing
                            attach(parent, structure_node);
                            
                            node->declaration.type = structure_node->structure.type;
                            
                            parent->temporary_declarations.state = parent->temporary_declarations.State_Check_Continue;
                            parent = structure_node;
                            
                            continue;
                        }
                        
                        node->declaration.type = parse_type(allocator, &reporter, &types, true);
                    }
                    
                    // falltrough
                    
                    case parent->temporary_declarations.State_Check_Continue: {
                        if (try_skip(&it, S("="))) {
                            parent->temporary_declarations.state = parent->temporary_declarations.State_Parse_Expressions;
                            continue;
                        }
                        
                        if (try_skip(&it, S(",")))
                            continue;
                        
                        // if previous type was a structure_node we don't need to skip a ';'
                        auto structure_node = try_kind_of(parent->children.tail, structure);
                        if (structure_node) {
                            do_cleanup = true;
                            break;
                        }
                        else if (try_skip(&it, S(";"))) {
                            do_cleanup = true;
                            break;
                        }
                        
                        expected_tokens(&reporter);
                    } break;
                    
                    case parent->temporary_declarations.State_Parse_Expressions: {
                        // search unparse , seperated expressions
                        auto node = parent->children.head;
                        
                        while (node) {
                            node->unparsed_expression = next_expression(allocator, &reporter, node->next == null ? S(";") : S(","));
                            node = node->next;
                        }
                        
                        do_cleanup = true;
                    } break;
                    
                    CASES_COMPLETE;
                }
                
                if (do_cleanup) {
                    
                    auto declarations = parent;
                    parent = parent->parent;
                    detach(declarations);
                    
                    while (declarations->children.head) {
                        // NOTE: anonymous structs dont have a parent, so... yeah
                        if (is_kind(*declarations->children.head, structure))
                            detach(declarations->children.head);
                        else
                            move(parent, declarations->children.head);
                    }
                    
                    free(allocator, declarations);
                    
                    continue;
                }
                
                UNREACHABLE_CODE;
            }
            
            // test for comment node
            
            if (try_skip(&it, S("def"))) {
                auto definition_node = ALLOCATE(allocator, Ast);
                *definition_node = make_kind(Ast, definition);
                attach(parent, definition_node);
                
                auto identifier = get_identifier(&reporter);
                definition_node->definition.identifier = identifier;
                
                bool is_coroutine = try_skip(&it, S("coroutine"));
                if (is_coroutine || try_skip(&it, S("function"))) {
                    
                    auto function_node = ALLOCATE(allocator, Ast);
                    *function_node = make_kind(Ast, function);
                    function_node->function = { top_function_node, is_coroutine };
                    top_function_node = function_node;
                    
                    attach(definition_node, function_node);
                    
                    skip(&reporter, S("("));
                    
                    if (!try_skip(&it, S(")"))) {
                        LOOP {
                            auto declaration_node = ALLOCATE(allocator, Ast);
                            *declaration_node = make_kind(Ast, declaration);
                            attach(function_node, declaration_node);
                            *grow(allocator, &function_node->function.arguments) = declaration_node;
                            
                            declaration_node->declaration.identifier = get_identifier(&reporter);
                            skip(&reporter, S(":"));
                            
                            declaration_node->declaration.type = parse_type(allocator, &reporter, &types, true);
                            
                            if (try_skip(&it, S(",")))
                                continue;
                            
                            if (try_skip(&it, S(")")))
                                break;
                            
                            expected_tokens(&reporter);
                        }
                    }
                    
                    if (try_skip(&it, S("->"))) {
                        
                        skip(&reporter, S("("));
                        
                        LOOP {
                            Type *type = parse_type(allocator, &reporter, &types, true);
                            
                            *grow(allocator, &function_node->function.return_types) = type;
                            
                            if (try_skip(&it, S(",")))
                                continue;
                            
                            if (try_skip(&it, S(")")))
                                break;
                            
                            expected_tokens(&reporter);
                        }
                    }
                    
                    parent = function_node;
                    continue;
                }
                
#if 0                
                if (try_skip(&it, S("list"))) {
                    
                    Type *list_type = find_or_create_base_type(allocator, identifier.text, &types);
                    EXPECT(&reporter, is_kind(*list_type, undeclared_type), S(
                        "% is allready declared here:\n"), f(*list_type));
                    
                    *list_type = make_kind(Type, list, identifier);
                    
                    skip(&reporter, S("("));
                    
                    list_type->list.entry_data.identifier = get_identifier(&reporter);
                    
                    skip(&reporter, S(":"));
                    
                    list_type->list.entry_data.type = parse_type(allocator, &reporter, &types, true);
                    
                    skip(&reporter, S(","));
                    
                    auto test = begin(&it);
                    bool ok;
                    list_type->list.with_tail = try_parse_bool(&it.text, &ok);
                    EXPECT(&reporter, ok, S("expected boolean ('true' or 'false')"));
                    end(test);
                    
                    skip(&reporter, S(","));
                    
                    test = begin(&it);
                    list_type->list.with_double_links = try_parse_bool(&it.text, &ok);
                    EXPECT(&reporter, ok, S("expected boolean ('true' or 'false')"));
                    end(test);
                    
                    if (try_skip(&it, S(","))) {
                        list_type->list.count_type = parse_type(allocator, &reporter, &types);
                        EXPECT(&reporter, list_type->list.count_type && is_kind(*list_type->list.count_type, base_type), S("expected a basic type as the list count_type"));
                    }
                    else {
                        list_type->list.count_type = null;
                    }
                    
                    skip(&reporter, S(")"));
                    skip(&reporter, S(";"));
                    
                    continue;
                }
#endif
                
                {
                    auto structure_node = parse_struct(allocator, &reporter, identifier, &types);
                    if (structure_node) {
                        attach(definition_node, structure_node);
                        parent = structure_node;
                        continue;
                    }
                }
                
                expected_tokens(&reporter);
            } // "def"
            
            {
                Ast *declarations_node = null;
                if (is_kind(*parent, structure) && try_skip(&it, S("kind"))) {
                    declarations_node = ALLOCATE(allocator, Ast);
                    *declarations_node = make_kind(Ast, temporary_declarations, true);
                    attach(parent, declarations_node);
                    
                    parent = declarations_node;
                    continue;
                }
                
                if (!declarations_node && try_skip(&it, S("var"))) {
                    declarations_node = ALLOCATE(allocator, Ast);
                    *declarations_node = make_kind(Ast, temporary_declarations, false);
                    attach(parent, declarations_node);
                    
                    parent = declarations_node;
                    continue;
                }
            }
            
            {
                Ast *conditional_node = null;
                
                if (try_skip(&it, S("if"))) {
                    conditional_node = ALLOCATE(allocator, Ast);
                    *conditional_node = make_kind(Ast, conditional_branch);
                    attach(parent, conditional_node);
                }
                
                if (!conditional_node && try_skip(&it, S("while"))) {
                    conditional_node = ALLOCATE(allocator, Ast);
                    *conditional_node = make_kind(Ast, conditional_loop, top_loop_node);
                    top_loop_node = conditional_node;
                    attach(parent, conditional_node);
                }
                
                if (conditional_node) {
                    auto unparsed_expression_node = ALLOCATE(allocator, Ast);
                    *unparsed_expression_node = make_kind(Ast, unparsed_expression);
                    attach(conditional_node, unparsed_expression_node);
                    
                    unparsed_expression_node->unparsed_expression = next_expression(allocator, &reporter, S("do"));
                    
                    parent = conditional_node;
                }
                
                if (!conditional_node && try_skip(&it, S("loop"))) {
                    conditional_node = ALLOCATE(allocator, Ast);
                    *conditional_node = make_kind(Ast, loop, top_loop_node);
                    top_loop_node = conditional_node;
                    attach(parent, conditional_node);
                }
                
                if (conditional_node || try_skip(&it, S("{"))) {
                    auto scope_node = ALLOCATE(allocator, Ast);
                    *scope_node = make_kind(Ast, scope, top_scope_node);
                    top_scope_node = scope_node;
                    attach(parent, scope_node);
                    
                    parent = scope_node;
                    
                    continue;
                }
            }
            
            if (is_kind(*parent, structure)) {
                if (try_skip(&it, S("kind"))) {
                    
                    auto kind_node = ALLOCATE(allocator, Ast);
                    
                    auto identifier = get_identifier(&reporter);
                    *kind_node = make_kind(Ast, structure_kind, identifier);
                    attach(parent, kind_node);
                    
                    continue;
                }
                
                expected_tokens(&reporter);
            }
            
            // not structure
            
            if (top_function_node) {
                Ast *return_or_yield_node = null;
                if (try_skip(&it, S("return"))) {
                    return_or_yield_node = ALLOCATE(allocator, Ast);
                    *return_or_yield_node = make_kind(Ast, return_statement);
                }
                
                if (top_function_node->function.is_coroutine && !return_or_yield_node && try_skip(&it, S("yield")))
                {
                    return_or_yield_node = ALLOCATE(allocator, Ast);
                    *return_or_yield_node = make_kind(Ast, yield_statement);
                }
                
                if (return_or_yield_node) {
                    attach(parent, return_or_yield_node);
                    
                    for (u32 i = 0; i < top_function_node->function.return_types.count; i++) {
                        
                        auto unparsed_expression_node = ALLOCATE(allocator, Ast);
                        *unparsed_expression_node = make_kind(Ast, unparsed_expression);
                        attach(return_or_yield_node, unparsed_expression_node);
                        
                        unparsed_expression_node->unparsed_expression =next_expression(allocator, &reporter, (i == top_function_node->function.return_types.count - 1) ? S(";") : S(","));
                    }
                    
                    continue;
                }
            }
            
            if (top_scope_node) {
                if (try_skip(&it, S("break"))) {
                    skip(&reporter, S(";"));
                    
                    auto break_node = ALLOCATE(allocator, Ast);
                    *break_node = make_kind(Ast, break_statement);
                    attach(parent, break_node);
                    
                    continue;
                }
            }
            
            if (top_loop_node) {
                if (try_skip(&it, S("continue"))) {
                    skip(&reporter, S(";"));
                    
                    auto continue_node = ALLOCATE(allocator, Ast);
                    *continue_node = make_kind(Ast, continue_statement);
                    attach(parent, continue_node);
                    
                    continue;
                }
            }
            
#if 0            
            
            if (try_skip(&it, S("type"))) {
                Type *type = parse_type(allocator, &reporter, &types, true);
                // its a new type
                EXPECT(&reporter, is_kind(*type, base_type), S("type allready declared"));
                
                auto test = begin(&it);
                bool ok;
                type->base_type.byte_count = TRY_PARSE_UNSIGNED_INTEGER(&it.text, &ok, 32);
                end(test);
                EXPECT(&reporter, ok, S("expecting byte_count as an u32"));
                
                skip(&reporter, S(";"));
                
                continue;
            }
#endif
            
            // if all else fails, get en unparsed expression!!!
            
            {
                auto expression_node = ALLOCATE(allocator, Ast);
                *expression_node = make_kind(Ast, unparsed_expression);
                expression_node->unparsed_expression = next_expression(allocator, &reporter, S(";"));
                attach(parent, expression_node);
            }
        }
    }
    
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
        
        Types temp_dependencies = {};
        defer { free_array(allocator, &temp_dependencies); };
        
        Types *dependencies;
        switch ((*self)->kind) {
            case_kind(Type, base_type) {
                continue;
            } break;
            
            case_kind(Type, structure) {
                dependencies = &(*self)->structure.dependencies;
            } break;
            
            case_kind(Type, list) {
                auto list = kind_of(*self, list);
                if (!is_kind(*list->entry_data.type, indirection))
                    *grow(allocator, &temp_dependencies) = list->entry_data.type;
                
                if (list->count_type)
                    *grow(allocator, &temp_dependencies) = list->count_type;
                
                dependencies = &temp_dependencies;
            } break;
            
            CASES_COMPLETE;
        }
        
        write_line_out(allocator);
        write_line_out(allocator, S("% depends on:"), f(**self));
        
        for_array_item(dependency, *dependencies) {
            write_line_out(allocator, S("  %"), f(**dependency));
            
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
    
#if 0    
    
    String_Buffer output = { allocator };
    
#if defined _WIN64
    write(&output, S(
        "#if !defined _WIN64\r\n"
        "#error this file was generated with x64 compiler on windows, offsets and sizes would be wrong on 32bit compiles\r\n"
        "#endif\r\n\r\n"
        ));
#else
    write(&output, S(
        "#if defined _WIN64\r\n"
        "#error this file was generated with x86 compiler on windows, offsets and sizes would be wrong on 64bit compiles\r\n"
        "#endif\r\n\r\n"
        ));
#endif
    
    write(&output, S(
        "#include <stdio.h>" "\r\n"
        "#include \"basic.h\"" "\r\n"
        "#include \"mo_string.h\"" "\r\n"
        "#include \"memory_growing_stack.h\"" "\r\n"
        "#include \"win32_platform.h\"" "\r\n"
        "#include \"memory_c_allocator.h\"" "\r\n"
        ));
    
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
            
            {
                auto list = try_kind_of(type, list);
                if (list) {
                    write(&output, S(
                        "#define Template_List_Name      %" "\r\n"
                        "#define Template_List_Data_Type %"  "\r\n"
                        "#define Template_List_Data_Name % // optional"  "\r\n"
                        ),
                          f(list->identifier), f(*list->entry_data.type), f(list->entry_data.identifier));
                    
                    // sizeof(head)
                    list->byte_count = sizeof(u8*);
                    list->byte_alignment = alignof(u8*);
                    
                    if (list->with_tail) {
                        write(&output, S("#define Template_List_With_Tail // optional"  "\r\n"));
                        // sizeof(tail)
                        list->byte_count += sizeof(u8*);
                    }
                    
                    if (list->with_double_links)
                        write(&output, S("#define Template_List_With_Double_Links // optional" "\r\n"));
                    
                    if (list->count_type) {
                        u32 byte_alignment;
                        u32 byte_count = byte_count_and_alignment_of(&byte_alignment, *list->count_type);
                        
                        align_to(&list->byte_count, byte_alignment);
                        list->byte_count += byte_count;
                        list->byte_alignment = MAX(list->byte_alignment, byte_alignment);
                        
                        write(&output, S("#define Template_List_Count_Type % // optional" "\r\n"), f(*list->count_type));
                    }
                    
                    write(&output, S("#include \"template_list.h\"" "\r\n\r\n"));
                    
                    continue;
                }
            }
            
            if (!is_kind(*type, structure_node))
                continue;
            
            auto structure_node = kind_of(type, structure_node);
            
            assert(is_kind(structure_node->structure, def_structure));
            
            {
                String_Buffer head_buffer = { allocator };
                defer{ free(&head_buffer); };
                
                String_Buffer body_buffer = { allocator };
                defer{ free(&body_buffer); };
                
                String_Buffer tail_buffer = { allocator };
                defer{ free(&tail_buffer); };
                
                // skip root
                auto current = structure_node;
                bool did_enter = true, did_leave = current && !current->children.head;
                
                bool parent_is_kind = false;
                
                while (current) {
                    defer { advance(&current, &did_enter, &did_leave); };
                    
                    if (is_kind(current->structure, var_structure))
                        parent_is_kind = current->is_kind;
                    
                    bool is_kind = current->is_kind || parent_is_kind;
                    
                    switch (current->kind) {
                        case_kind(Structure, def_structure) {
                            if (did_enter) {
                                write(&head_buffer, S("struct % {\r\n"), f(current->def_structure.identifier));
                                
                                if (current->def_structure.kind_count) {
                                    write(&head_buffer, S("enum Kind {\r\nKind_null,\r\n"));
                                    
                                    write(&tail_buffer, S("union {\r\n"));
                                }
                                
                                assert(!current->def_structure.byte_count);
                                assert(!current->def_structure.byte_alignment);
                            }
                            
                            if (did_leave) {
                                if (current->def_structure.kind_count) {
                                    u32 kind_count = current->def_structure.kind_count + 2;
                                    u32 bit_count = bit_count_of(kind_count);
                                    if (bit_count <= 8)
                                        bit_count = 8;
                                    else if (bit_count <= 16)
                                        bit_count = 16;
                                    else if (bit_count <= 32)
                                        bit_count = 32;
                                    else
                                        UNREACHABLE_CODE;
                                    
                                    write(&head_buffer, S("Kind_Count,\r\n};\r\n\r\nu% kind;\r\n"), f(bit_count));
                                    
                                    update_byte_count_and_alignment(current, bit_count/8, bit_count/8);
                                    update_byte_count_and_alignment(current, current->def_structure.max_kind_byte_count, current->def_structure.max_kind_byte_alignment);
                                    
                                    write(&tail_buffer, S("};\r\n"));
                                }
                                
                                // struct size must be multiple of alignment
                                align_to(&current->def_structure.byte_count, current->def_structure.byte_alignment);
                                
                                write(&tail_buffer, S("\r\nstatic const usize Byte_Count = %;\r\n"), f(current->def_structure.byte_count));
                                write(&tail_buffer, S("static const usize Byte_Alignment = %;\r\n"), f(current->def_structure.byte_alignment));
                                
                                write(&output, S("%\r\n%\r\n%};\r\n\r\n"), f(head_buffer.buffer), f(body_buffer.buffer), f(tail_buffer.buffer));
                                
                                free(&head_buffer);
                                free(&tail_buffer);
                                free(&body_buffer);
                            }
                        } break;
                        
                        case_kind(Structure, var_structure) {
                            if (did_enter) {
                                if (current->is_kind) {
                                    write(&head_buffer, S("Kind_%,\r\n"), f(current->structure.var_structure.identifier));
                                }
                                
                                if (is_kind) {
                                    write(&tail_buffer, S("struct {\r\n"));
                                }
                                else
                                    write(&body_buffer, S("struct {\r\n"));
                                
                                assert(!current->def_structure.byte_count);
                                assert(!current->def_structure.byte_alignment);
                            }
                            
                            if (did_leave) {
                                if (is_kind) {
                                    write(&tail_buffer, S("\r\nstatic const usize Byte_Count = %;\r\nstatic const usize Byte_Alignment = %;\r\n"), f(current->var_structure.byte_count), f(current->var_structure.byte_alignment));
                                    write(&tail_buffer, S("} %;\r\n\r\n"), f(current->var_structure.identifier));
                                }
                                else {
                                    write(&body_buffer, S("\r\nstatic const usize Byte_Count = %;\r\nstatic const usize Byte_Alignment = %;\r\n"), f(current->var_structure.byte_count), f(current->var_structure.byte_alignment));
                                    write(&body_buffer, S("} %;\r\n\r\n"), f(current->var_structure.byte_count), f(current->var_structure.identifier));
                                }
                                
                                // struct size must be multiple of alignment
                                align_to(&current->var_structure.byte_count, current->var_structure.byte_alignment);
                                
                                if (current->parent) {
                                    if (!current->is_kind) {
                                        update_byte_count_and_alignment(current->parent, current->structure.var_structure.byte_count, current->var_structure.byte_alignment);
                                    }
                                    else {
                                        current->parent->var_structure.max_kind_byte_alignment = MAX(current->parent->var_structure.max_kind_byte_alignment, current->var_structure.byte_alignment);
                                        current->parent->var_structure.max_kind_byte_count = MAX(current->parent->var_structure.max_kind_byte_alignment, current->var_structure.byte_count);
                                    }
                                }
                            }
                        } break;
                        
                        case_kind(Structure, field) {
                            assert(did_enter && did_leave);
                            
                            if (current->is_kind) {
                                write(&head_buffer, S("Kind_%,\r\n"), f(current->field.identifier));
                            }
                            
                            if (is_kind) {
                                write(&tail_buffer, S("% %;\r\n"), f(*current->field.type), f(current->field.identifier));
                            }
                            else
                                write(&body_buffer, S("% %;\r\n"), f(*current->field.type), f(current->field.identifier));
                            
                            if (current->parent) {
                                u32 byte_alignment;
                                u32 byte_count = byte_count_and_alignment_of(&byte_alignment, *current->field.type);
                                
                                if (!current->is_kind) {
                                    update_byte_count_and_alignment(current->parent, byte_count, byte_alignment);
                                }
                                else {
                                    current->parent->var_structure.max_kind_byte_alignment = MAX(current->parent->var_structure.max_kind_byte_alignment, byte_alignment);
                                    current->parent->var_structure.max_kind_byte_count = MAX(current->parent->var_structure.max_kind_byte_alignment, byte_count);
                                }
                            }
                        } break;
                        
                        CASES_COMPLETE;
                    }
                }
            }
        }
    }
    
    write(&output, S("\r\n"));
    
    // also here we calculate the local variable sizes
    if (functions.count) {
        write(&output, S("#include \"preprocessor_coroutines.h\"\r\n\r\n"));
        
        for_array_item(function, functions) {
            write_signature(&output, **function);
            write(&output, S(";\r\n"));
            
            auto current = &(*function)->local_variables;
            bool did_enter = true, did_leave = false;
            
            while (current) {
                defer { advance(&current, &did_enter, &did_leave); };
                
                auto def_structure = try_kind_of(&current->structure, def_structure);
                if (def_structure) {
                    if (did_enter) {
                        def_structure->max_kind_byte_count = 0;
                        def_structure->byte_count = 0;
                        def_structure->byte_alignment = 0;
                    }
                    continue;
                }
                
                auto var_structure = try_kind_of(&current->structure, var_structure);
                if (var_structure) {
                    if (did_enter) {
                        var_structure->max_kind_byte_count = 0;
                        var_structure->byte_count = 0;
                        var_structure->byte_alignment = 0;
                    }
                    
                    if (did_leave) {
                        current->parent->var_structure.max_kind_byte_count = MAX(current->parent->var_structure.max_kind_byte_count, var_structure->max_kind_byte_count);
                        current->parent->var_structure.byte_count -= var_structure->byte_count;
                        
                        write_line_out(allocator, S("subscope has byte count of %"), f(var_structure->max_kind_byte_count));
                        
                    }
                    
                    continue;
                }
                
                auto field = kind_of(&current->structure, field);
                u32 byte_alignment;
                u32 byte_count = byte_count_and_alignment_of(&byte_alignment, *field->type);
                
                current->parent->var_structure.byte_count += byte_count;
                current->parent->var_structure.max_kind_byte_count = MAX(current->parent->var_structure.max_kind_byte_count, current->parent->var_structure.byte_count);
            }
            
            write_line_out(allocator, S("function % has local byte count of %"), f((*function)->identifier), f((*function)->local_variables.def_structure.max_kind_byte_count));
            
            write(&output, S("\r\n"));
        }
    }
    
    {
        Text_Iterator it = { source, 1 };
        
        string file_path = to_forward_slashed_path(allocator, source_file_path);
        //write(&output, S("#line 1 \"%\"\r\n\r\n"), f(file_path));
        
        while (it.text.count) {
            skip_space(&it, &output);
            
            if (try_skip(&it, S("def"), &output)) {
                skip_space(&it, &output);
                
                string identifier = get_identifier(&it.text, S("_"));
                assert(identifier.count);
                skip_space(&it, &output);
                
                if (try_skip(&it, S("list"), &output)) {
                    skip_until_first_in_set(&it, S(";"), &output, true);
                    
                    continue;
                }
                
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
                    
                    auto switch_table = new_write_buffer(allocator, S("switch (CO_HEADER.current_label_index) {\r\n"));
                    defer { free(&switch_table); };
                    
                    String_Buffer body = { allocator };
                    defer { free(&body); };
                    
                    defer { 
                        if (function->is_coroutine) 
                            write(&output, S("%default:\r\nassert(0);\r\n}\r\n\r\n%"), f(switch_table.buffer), f(body.buffer)); 
                        else
                            write(&output, S("%\r\n\r\n"), f(body.buffer));
                    };
                    
                    for_array_item(argument, function->arguments) {
                        if (function->is_coroutine)
                            write(&body, S("// argument: def %: %;\r\n"), f(argument->identifier), f(*argument->type));
                    }
                    
                    u32 label_count = 0;
                    
                    if (function->is_coroutine)
                        add_label(&switch_table, &body, &label_count);
                    
                    skip_until_first_in_set(&it, S("{"), &output, true);
                    
                    auto current = &function->local_variables;
                    while (current) {
                        skip_space(&it, &body);
                        
                        while (try_skip(&it, S(";"), &body))
                            skip_space(&it, &body);
                        
                        if (try_skip(&it, S("}"), &body)) {
                            current = current->parent;
                            
                            if (function->is_coroutine) {
                                if (!current) {
                                    //TODO if no return values this, else error state
                                    write(&body, S("CO_HEADER.current_label_index = %;\r\n"), f(label_count));
                                    write(&body, S("return Coroutine_Continue;\r\n"));
                                }
                            }
                            
                            write(&body, S("}\r\n\r\n"));
                            continue;
                        }
                        
                        if (try_skip(&it, S("{"), &body)) {
                            current = current->children.head;
                            
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
                                    
                                    string expression_prime = replace_expression(allocator, expression, functions, function, current);
                                    defer { free_array(allocator, &expression_prime); };
                                    
                                    write(&body, S("*_out% = %;\r\n"), f(i), f(expression_prime));
                                }
                                
                                string first_expression_prime = replace_expression(allocator, first_expression, functions, function, current);
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
                                
                                string expression_prime = replace_expression(allocator, expression, functions, function, current);
                                defer { free_array(allocator, &expression_prime); };
                                
                                write(&body, S("CO_RESULT(%, %) = %;\r\n"), f(*function->return_types[i]), f(return_value_byte_offset_of(function, i)), f(expression_prime));
                            }
                            
                            if (is_return) {
                                write(&body, S("CO_HEADER.current_label_index = u32_max;\r\n"));
                                write(&body, S("call_stack->current_byte_index = CO_HEADER.previous_byte_index;\r\n"));
                                write(&body, S("return Coroutine_Continue;\r\n}\r\n\r\n"));
                            }
                            else {
                                
                                write(&body, S("CO_HEADER.current_label_index = %;\r\n"), f(label_count));
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
                                
                                // can't go up, since we need declared variables to be visible by replace_expression
                                if (current->next)
                                    current = current->next;
                                
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
                                        string allocator_expression_prime = replace_expression(allocator, allocator_expression, functions, function, current);
                                        defer { free_array(allocator, &allocator_expression_prime); };
                                        
                                        write(&body, S("Memory_Growing_Stack *_allocator = %;\r\n"), f(allocator_expression_prime));
                                        write(&body, S("Coroutine_Stack _call_stack = { _allocator };\r\n"));
                                    }
                                    
                                    // CO_ macros work on call_stack pointer
                                    write(&body, S("auto call_stack = &_call_stack;\r\n"));
                                    write(&body, S("*grow_item(_call_stack.allocator, &_call_stack.buffer, usize) = 0; // push terminal stack entry\r\n"));
                                    write(&body, S("auto it = co_push_coroutine(call_stack, %, %);\r\n"), f(subroutine->identifier), f(sizeof(Coroutine_Header) + byte_count_of(*subroutine)));
                                    
                                    for (u32 i = 0; i < arguments.count; i++) {
                                        string argument_prime = replace_expression(allocator, arguments[i], functions, function, current);
                                        defer { free_array(allocator, &argument_prime); };
                                        
                                        write(&body, S("*next_item(&it, %) = %;\r\n"), f(*subroutine->arguments[i].type), f(argument_prime));
                                    }
                                    
                                    write(&body, S(
                                        "\r\n"
                                        "if (!run_without_yielding(call_stack)) {\r\n"
                                        "assert(0, \"todo: handle Coroutine_Abort\");\r\n"
                                        "}\r\n\r\n"
                                        ));
                                    
                                    write_read_coroutine_results(allocator, &body, subroutine, left_expressions, functions, function, current);
                                    
                                    write(&body, S("free_array(_allocator, &call_stack->buffer);\r\n"));
                                    
                                    write(&body, S("}\r\n"));
                                    
                                    // check scope later ...
                                    
                                    continue;
                                }
                                
                                parse_call_subroutine_or_expression(allocator, &switch_table, &body, &it, left_expressions, &label_count, functions, function, current);
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
                                
                                auto expression_prime = replace_expression(allocator, expression, functions, function, current);
                                defer { free_array(allocator, &expression_prime); };
                                
                                write(&body, S("% (%) "), f(directive), f(expression_prime));
                                continue;
                            }
                        }
                        
                        {
                            parse_call_subroutine_or_expression(allocator, &switch_table, &body, &it, {}, &label_count, functions, function, current);
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
            
            UNREACHABLE_CODE;
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
        mooselib_path = new_write(allocator, S("%\\\\..\\\\..\\\\"), f(preprocesor_path));
    }
    else {
        mooselib_path = new_write(allocator, S("%\\\\%\\\\..\\\\..\\\\"), f(working_directory), f(preprocesor_path));
    }
    
    string command;
    if (do_compile)
        command = new_write(allocator, S("cl -Fe\"%.exe\" \"%\" kernel32.lib user32.lib shell32.lib gdi32.lib opengl32.lib /I \"%code\" /I \"%3rdparty\" /Zi /nologo /EHsc /link /INCREMENTAL:NO"), f(source_file_name), f(out_file_path), f(mooselib_path), f(mooselib_path));
    else
        command = new_write(allocator, S("cl /c \"%\"  /I \"%\" /Zi /nologo /EHsc /link /INCREMENTAL:NO"), f(out_file_path), f(mooselib_path));
    
    u32 exit_code;
    bool ok;
    string message = platform_api->run_command(&exit_code, &ok, platform_api, command, allocator);
    
    printf("\n\n%.*s\n", FORMAT_S(&message));
    
#endif
#endif
    
    return 1;
}