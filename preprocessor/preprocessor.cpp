
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

#include "template_defines.h"

struct Best_Token_Iterator {
    string *text_iterator;
    string best_token;
    string best_end;
    u32 best_index;
    u32 count;
    bool best_skip_end;
};

Best_Token_Iterator begin_best_token(string *text_iterator) {
    return { text_iterator, {}, {}, u32_max };
}

void test_token(Best_Token_Iterator *iterator, string end, bool skip_end = true) {
    bool found = false;
    
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
    if (found && ((iterator->best_index == u32_max) || (a.count < iterator->best_token.count))) {
        iterator->best_token = a;
        iterator->best_end = end;
        iterator->best_index = iterator->count;
        iterator->best_skip_end = skip_end;
    }
    
    iterator->count++;
}

bool end(Best_Token_Iterator *iterator) {
    advance(iterator->text_iterator, iterator->best_token.count);
    
    if (iterator->best_skip_end)
        advance(iterator->text_iterator, iterator->best_end.count);
    
    return (iterator->best_token.count > 0);
}

struct Token {
    string text;
    u32 line_count;
    u32 column_count;
};

#define Template_Array_Name      Tokens
#define Template_Array_Data_Type Token
#include "template_array.h"

struct Tokenizer {
    Memory_Allocator *allocator;
    string_array expected_tokens;
    Token skipped_token;
    
    string text;
    string line_start;
    u32 line_count;
    u32 column_count;
};

auto White_Space      = S(" \t\r\0");
auto White_Space_Ex   = S(" \t\r\n\0");
auto Operator_Symbols = S("+-*/%|&~<>=²³!?^");

struct Tokenizer_Test {
    Tokenizer *it;
    string text;
    u32 line_count;
    u32 column_count;
};

Tokenizer_Test begin(Tokenizer *it) {
    return { it, it->text, it->line_count, it->column_count };
}

#if 0
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
#endif

Token end(Tokenizer_Test test) {
    free_array(test.it->allocator, &test.it->expected_tokens);
    
    auto result = Token{ sub_string(test.text, test.it->text), test.line_count, test.column_count };
    
    skip_set(&test.it->text, White_Space_Ex);
    
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
    
    return result;
}

void revert(Tokenizer_Test test) {
    test.it->text         = test.text;
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
    
    auto a = begin(&result);
    skip_set(&result.text, White_Space_Ex);
    end(a);
    
    return result;
}

struct Report_Context {
    Memory_Allocator *allocator;
    string file_path;
    string file_text;
    Tokenizer *tokenizer;
};

struct Format_Info_File_Position {
    String_Write_Function write;
    string file_path;
    u32 line_count;
    u32 column_count;
};

STRING_WRITE_DEC(write_file_position) {
    auto format_info = &va_arg(*va_args, Format_Info_File_Position);
    write(allocator, output, S("%(%:%)"), f(format_info->file_path), f(format_info->line_count), f(format_info->column_count));
}

Format_Info_File_Position f(string file_path, u32 line_count, u32 column_count) {
    return { write_file_position, file_path, line_count, column_count };
}

struct Format_Info_Marked_Line {
    String_Write_Function write;
    string line;
    u32 line_count;
    u32 column_count;
    u32 indent_count;
    bool with_file_position;
    string file_path;
};

STRING_WRITE_DEC(write_marked_line) {
    auto format_info = &va_arg(*va_args, Format_Info_Marked_Line);
    
    string pretty_line = {};
    free_array(allocator, &pretty_line);
    
    u32 pretty_column_count = 0;
    
    assert(format_info->column_count);
    u32 column_count = format_info->column_count - 1;
    
    while (format_info->line.count) {
        u32 byte_count;
        u32 head = utf8_head(format_info->line, &byte_count);
        
        if (head == '\t') {
            write(allocator, &pretty_line, S("  "));
            
            if (column_count)
                pretty_column_count += 2;
        }
        else {
            write(allocator, &pretty_line, S("%"), f(sub(format_info->line, 0, byte_count)));
            
            if (column_count)
                pretty_column_count++;
        }
        
        advance(&format_info->line, byte_count);
        if (column_count)
            column_count--;
    }
    
    if (format_info->with_file_position)
        write(allocator, output, S("%:\n\n"), f(format_info->file_path, format_info->line_count, format_info->column_count));
    
    write(allocator, output, S("%: %%\n"), f(format_info->line_count), f_indent(format_info->indent_count), f(pretty_line));
    
    if (format_info->indent_count + pretty_column_count > 0)
        write(allocator, output, S("%: %~\n"), f(format_info->line_count), f_indent(format_info->indent_count + pretty_column_count));
    else
        write(allocator, output, S("%: ~\n"), f(format_info->line_count));
}

Format_Info_Marked_Line f(string line, u32 line_count, u32 column_count, u32 indent_count) {
    return { write_marked_line, line, line_count, column_count, indent_count };
}

Format_Info_Marked_Line f(string whole_text, Token token, u32 indent_count, bool with_file_position = false, string file_path = {}) {
    string line = token.text;
    
    u32 max_byte_count = index_of(whole_text, token.text.data);
    for (u32 i = 0; i < token.column_count - 1; i++) {
        assert(max_byte_count);
        u32 byte_count = utf8_unsafe_previous(line.data, max_byte_count);
        line.data  -= byte_count;
        line.count += byte_count;
        max_byte_count -= byte_count;
    }
    
    max_byte_count = whole_text.data + whole_text.count - line.data;
    line.count = 0;
    LOOP {
        u32 byte_count;
        u32 head = utf8_unsafe_next(&byte_count, line.data + line.count, max_byte_count);
        if (head == '\n')
            break;
        
        line.count += byte_count;
        max_byte_count -= byte_count;
    }
    
    return { write_marked_line, line, token.line_count, token.column_count, indent_count, with_file_position, file_path };
}

void report(Report_Context *context, string format, ...) {
    va_list va_args;
    va_start(va_args, format);
    write_out_va(context->allocator, format, va_args);
    va_end(va_args);
    
    write_line_out(context->allocator);
    
    UNREACHABLE_CODE; // easier to debug
    exit(0);
}

void expect(Report_Context *context, Token token, string format, ...) {
    write_out(context->allocator, S("% error: "), f(context->file_path, token.line_count, token.column_count));
    
    va_list va_args;
    va_start(va_args, format);
    write_out_va(context->allocator, format, va_args);
    va_end(va_args);
    
    write_out(context->allocator, S("\n\n"));
    write_out(context->allocator, S("%"), f(context->file_text, token, 2));
    
    UNREACHABLE_CODE; // easier to debug
    exit(0);
}

#define EXPECT(context, condition, format, ...) \
if (!(condition)) { \
    expect(context, { (context)->tokenizer->text, (context)->tokenizer->line_count, (context)->tokenizer->column_count }, format, __VA_ARGS__); \
}

#define EXPECT_TOKEN(context, condition, token, format, ...) \
if (!(condition)) { \
    expect(context, token, format, __VA_ARGS__); \
}

#define EXPECT_FORMATED(context, condition, format, ...) \
if (!(condition)) { \
    report(context, format, __VA_ARGS__); \
}

bool try_skip(Tokenizer *it, string pattern) {
    auto test = begin(it);
    
    if (try_skip(&it->text, pattern)) {
        it->skipped_token = end(test);
        
        return true;
    }
    
    revert(test);
    *grow(it->allocator, &it->expected_tokens) = pattern;
    
    return false;
}

void skip(Report_Context *context, string pattern) {
    bool ok = try_skip(context->tokenizer, pattern);
    
    if (!ok) {
        EXPECT(context, ok, S("expected '%'"), f(pattern));
    }
}

Token try_skip_identifier(Tokenizer *tokenizer) {
    auto test = begin(tokenizer);
    string identifier = get_identifier(&tokenizer->text, S("_"));
    if (identifier.count) {
        return end(test);
    }
    else {
        revert(test);
        return {};
    }
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
    
    EXPECT(context, 0, S("%"), f(output));
}

struct Type;

#define Template_Array_Name      Types
#define Template_Array_Data_Type Type*
#include "template_array.h"

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
        Kind_function,
        
        Kind_Count,
    } kind;
    
    union {
        struct {
            Token namespace_path;
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
            Ast *node;
            Types dependencies;
        } function;
    };
};

#define Template_Array_Name Ast_Array
#define Template_Array_Data_Type Ast*
#include "template_array.h"

struct Ast {
    enum Kind {
        Kind_null,
        
        Kind_definition,
        Kind_declaration,
        
        //Kind_enumaration,
        
        Kind_assignment,
        
        Kind_temporary_declarations,
        
        Kind_structure,
        Kind_structure_kind,
        
        Kind_function,
        
        Kind_comment,
        
        Kind_return_statement,
        
        Kind_break_statement, 
        Kind_continue_statement,
        
        Kind_conditional_branch,
        Kind_conditional_loop,
        
        Kind_scope, 
        Kind_loop,
        
        Kind_unparsed_expression,
        
        Kind_Count,
    } kind;
    
    TEMPLATE_DOUBLE_LIST_TYPE_DEC(Children, Ast);
    
    TEMPLATE_TREE_LEAF_DEC(Ast);
    
    union {
        struct {
            Token token;
            Token identifier;
            Type *type;
            Token unparsed_expression;
        } declaration, structure_kind;
        
        struct {
            Token destination_unparsed_expression;
            Token source_unparsed_expression;
        } assignment;
        
        struct {
            Token token;
            bool as_structure_kinds;
            
            enum State {
                State_Parse_Identifier,
                State_Check_Continue,
                State_Parse_Expressions,
            } state;
            
            Children list;
        } temporary_declarations;
        
        struct {
            Token token;
            Token identifier;
            Ast *value;
        } definition;
        
#if 0        
        struct {
            Tokens values;
            Type *size_type;
        } enumaration;
#endif
        
        struct {
            Token token;
            Token open_bracket, closed_bracked;
            Type type;
            Children body;
            Ast *kind_union;
            Types ordered_sub_types;
        } structure;
        
        struct {
            Token token;
            bool is_coroutine;
            Type type;
            Types return_types;
            Children arguments;
            Ast *statement;
        } function;
        
        Token comment;
        
        struct {
            Token token;
            bool is_yield;
            Children values;
        } return_statement;
        
        struct {
            Token token;
            Ast *parent_scope;
        } break_statement, continue_statement;
        
        struct {
            Token token;
            Ast *condition;
            Ast *statement;
        } conditional_branch;
        
        struct {
            Token token;
            Ast *condition;
            Ast *statement;
        } conditional_loop;
        
        struct {
            Token token;
            Ast *statement;
        } loop;
        
        struct {
            Token identifier;
            Token open_bracket, closed_bracked;
            Children body;
        } scope;
        
        Token unparsed_expression;
    };
};

#define Template_Tree_Name      Ast
#define Template_Tree_Struct_Is_Declared
#define Template_Tree_With_Leafs
#include "template_tree.h"

void align_to(u32 *byte_count, u32 byte_alignment) {
    if (byte_alignment)
        (*byte_count) += (byte_alignment - (*byte_count % byte_alignment)) % byte_alignment;
}

#if 0
void update_byte_count_and_alignment(Structure *node, u32 child_byte_count, u32 child_byte_alignment) {
    assert(!is_kind(*node, field));
    
    node->def_structure.byte_alignment = MAX(node->def_structure.byte_alignment, child_byte_alignment);
    align_to(&node->def_structure.byte_count, child_byte_alignment);
    node->def_structure.byte_count += child_byte_count;
}
#endif

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
        
        case_kind(Type, list) {
            assert(type.list.byte_count);
            assert(type.list.byte_alignment);
            *out_byte_alignment = type.list.byte_alignment;
            return type.list.byte_count;
        }
#endif
        
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
    bool full_namespace;
};

STRING_WRITE_DEC(write_type) {
    auto format_info = &va_arg(*va_args, Type_Format_Info);
    
    auto type = &format_info->type;
    auto indirection = try_kind_of(type, indirection);
    if (indirection)
        type = indirection->type;
    
    if (format_info->full_namespace && is_kind(*type, structure)) {
        auto current = type->structure.node;
        auto offset = byte_count_of(*output);
        
        bool first_skipped = false;
        
        while (current) {
            auto definition_node = try_kind_of(current, definition);
            if (definition_node) {
                if (first_skipped) {
                    auto count = definition_node->identifier.text.count + 1;
                    grow(allocator, output, count);
                    
                    copy(output->data + offset + count, output->data + offset, byte_count_of(*output) - offset - count);
                    copy(output->data + offset, definition_node->identifier.text.data, count - 1);
                    (*output)[offset + count - 1] = '.';
                }
                
                first_skipped = true;
            }
            
            current = current->parent;
        }
    }
    
    switch (type->kind) {
#if 0
        case_kind(Type, undeclared_type) {
            write(allocator, output, S("%"), f(type->undeclared_type.identifier.text));
        } break;
#endif
        
        case_kind(Type, base_type) {
            write(allocator, output, S("%"), f(type->base_type.identifier.text));
        } break;
        
        case_kind(Type, structure) {
            auto definition_node = type->structure.node->parent;
            if (definition_node && is_kind(*definition_node, definition))
                write(allocator, output, S("%"), f(definition_node->definition.identifier.text));
            else
                write(allocator, output, S("anonymous struct"));
        } break;
        
#if 0        
        case_kind(Type, list) {
            write(allocator, output, S("%"), f(type->list.identifier.text));
        } break;
#endif
        
        CASES_COMPLETE;
    }
    
    if (indirection)
        write(allocator, output, S("%"), f_indent(indirection->level, '*'));
}

Type_Format_Info f(Type type, bool full_namespace = false) {
    return { write_type, type, full_namespace };
}

#define if_kind(base, kind) \
auto kind = try_kind_of(base, kind); \
if (kind)


Ast * find_relative_type(Ast *parent, string_array relative_namespace_path) {
    auto current = parent;
    bool did_enter = true;
    auto i = 0;
    
    while (current != parent->parent) {
        switch (current->kind) {
            case_kind(Ast, definition) {
                if (did_enter) {
                    if (current->definition.identifier.text == relative_namespace_path[i]) {
                        i++;
                        
                        if (i == relative_namespace_path.count)
                            return current;
                        
                        current = current->definition.value;
                        continue; // skip normal advance
                    }
                    else {
                        break; // try next sibling
                    }
                }
                else {
                    // we entered because we matched
                    // but nothing in innerscoped mathed
                    return null;
                }
            } break;
            
            // TODO: named scopes must be matched
            case_kind(Ast, scope) {
                if (did_enter) {
                    if (current->scope.body.head) {
                        current = current->scope.body.head;
                        continue; // skip normal advance
                    }
                }
            } break;
            
            case_kind(Ast, structure) {
                if (did_enter) {
                    if (current->structure.body.head) {
                        current = current->structure.body.head;
                        continue; // skip normal advance
                    }
                }
            } break;
        }
        
        // only one way, we have to match any child!!
        //current = current->next;
        advance_up_or_right(&current, &did_enter);
    }
    
    return null;
}

void to_namespace_path(Memory_Allocator *allocator, string_array *namespace_path, string identifier_path) {
    do {
        string identifier = get_identifier(&identifier_path, S("_"));
        assert(identifier.count);
        skip_set(&identifier_path, White_Space);
        
        *grow(allocator, namespace_path) = identifier;
        
        if (!try_skip(&identifier_path, S("."))) {
            return;
        }
        
        skip_set(&identifier_path, White_Space);
    } while (identifier_path.count);
}

Ast * find_type(Ast *parent, string_array relative_namespace_path) {
    
    auto current = parent;
    
    // we don't need to namespace the parents name
    if (is_kind(*current, definition))
        current = current->definition.value;
    
    while (current) {
        defer { current = current->parent; };
        
        auto definition = try_kind_of(current, definition);
        if (definition && (definition->identifier.text == relative_namespace_path[0])) {
            if (relative_namespace_path.count > 1) {
                auto node = find_relative_type(current, sub(relative_namespace_path, 1));
                if (node)
                    return node;
            }
            else {
                return current;
            }
        }
        
        auto scope = try_kind_of(current, scope);
        if (scope) {
            auto node = find_relative_type(current, relative_namespace_path);
            if (node)
                return node;
        }
        
        auto structure = try_kind_of(current, structure);
        if (structure) {
            auto node = find_relative_type(current, relative_namespace_path);
            if (node)
                return node;
        }
    }
    
    return null;
}

void add_dependency(Memory_Allocator *allocator, Report_Context *context, Type *type, Type *dependency) {
    assert(is_kind(*type, structure));
    
    if (!is_kind(*dependency, structure) || (is_ancesotor(type->structure.node, dependency->structure.node) > 2))
        return;
    
    for_array_item(it, type->structure.dependencies) {
        if (*it == dependency)
            return;
    }
    
    *grow(allocator, &type->structure.dependencies) = dependency;
}

void add_dependencies(Memory_Allocator *allocator, Report_Context *context, Type *type, Type *child) {
    assert(is_kind(*child, structure));
    
    if (child->structure.node->parent && is_kind(*child->structure.node->parent, definition))
        add_dependency(allocator, context, type, child);
    
    for_array_item(dependency, child->structure.dependencies) {
        EXPECT(context, type != *dependency, S("cyclic dependency, % and % depend on each other"), f(*type), f(*child));
        
        add_dependency(allocator, context, type, *dependency);
    }
}

void replace_undeclared_type(Memory_Allocator *allocator, Report_Context *context, Ast *parent, Type *parent_type, Type **type_storage) {
    
    bool is_indirection = is_kind(**type_storage, indirection);
    
    if (is_indirection)
        type_storage = &(*type_storage)->indirection.type;
    
    if (!is_kind(**type_storage, undeclared_type))
        return;
    
    auto identifier = (*type_storage)->undeclared_type.namespace_path;
    
    string_array relative_path = {};
    defer { free_array(allocator, &relative_path); };
    to_namespace_path(allocator, &relative_path, identifier.text);
    
    free(allocator, *type_storage);
    
    auto node = find_type(parent, relative_path);
    assert(is_kind(*node, definition));
    
    auto type_node = node->definition.value;
    assert(is_kind(*type_node, structure));
    
    *type_storage = &type_node->structure.type;
    
    EXPECT(context, *type_storage, S("type is undefined:\n\n%\n\n%"), f(context->file_path, identifier.line_count, identifier.column_count), f(context->file_text, identifier, 2));
    
    if (!is_indirection && parent_type) {
        add_dependency(allocator, context, parent_type, *type_storage);
    }
}

Type * parse_type(Memory_Allocator *allocator, Report_Context *context, Types base_types, bool is_expected = false)
{
    auto test = begin(context->tokenizer);
    
    u32 indirection_count = 0;
    while (try_skip(&context->tokenizer->text, S("*"))) {
        skip_set(&context->tokenizer->text, White_Space_Ex);
        indirection_count++;
    }
    
    auto type_test = begin(context->tokenizer);
    string type_end = context->tokenizer->text;
    
    LOOP {
        auto identifier = get_identifier(&context->tokenizer->text, S("_"));
        if (!identifier.count) {
            EXPECT(context, !is_expected, S("expected type"));
            revert(test);
            return null;
        }
        
        type_end = context->tokenizer->text;
        
        skip_set(&context->tokenizer->text, White_Space_Ex);
        
        if (!try_skip(&context->tokenizer->text, S(".")))
            break;
        
        skip_set(&context->tokenizer->text, White_Space_Ex);
    }
    
    // TOTAL HACK: to get the proper line count of the base_type, not the whole type...
    Token type_identifier;
    
    {
        auto backup = begin(context->tokenizer);
        context->tokenizer->text = type_end;
        type_identifier = end(type_test);
        revert(backup);
    }
    
    end(test);
    
    Type *type = null;
    for_array_item(it, base_types) {
        if ((*it)->base_type.identifier.text == type_identifier.text) {
            type = *it;
            break;
        }
    }
    
    if (!type)
        type = new_kind(allocator, Type, undeclared_type, type_identifier);
    
    if (indirection_count) {
        auto indirection = new_kind(allocator, Type, indirection, type, indirection_count);
        return indirection;
    }
    
    return type;
}

#if 0
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
#endif

void add_label(String_Buffer *switch_table, String_Buffer *body, u32 *label_count) {
    write(switch_table, S("case %:\r\ngoto co_label%;\r\n\r\n"), f(*label_count), f(*label_count));
    
    write(body, S("co_label%:\r\n\r\n"), f(*label_count));
    
    (*label_count)++;
}

#if 0
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
#endif

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

Token next_expression(Memory_Allocator *allocator, Report_Context *context, string terminal = S(","), u32 expected_expression_count = 0) {
    
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
        test_token(&best, close_tokens[close_tokens.count - 1], (close_tokens.count > 1));
        test_token(&best, S(";"), false);
        test_token(&best, S("do"), false);
        test_token(&best, S("\""), false);
        end(&best);
        
        switch (best.best_index) {
            case 0: {
                *grow(allocator, &close_tokens) = S(")");
            } break;
            
            case 1: {
                *grow(allocator, &close_tokens) = S("}");
            } break;
            
            case 2: {
                shrink(allocator, &close_tokens);
            } break;
            
            case 3:
            case 4: {
                end(test);
                EXPECT(context, 0, S("unexpected '%', expected % expressions"), f(best.best_end), f(expected_expression_count));
            } break;
            
            case 5: {
                parse_quoted_string(&context->tokenizer->text);
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

#define Template_Tree_Name      Type_Node
#define Template_Tree_Data_Type Type*
#define Template_Tree_Data_Name type
#include "template_tree.h"

#define Template_Array_Name      Type_Nodes
#define Template_Array_Data_Type Type_Node
#include "template_array.h"

void order_sub_types(Memory_Allocator *allocator, Report_Context *context, Ast *structure_node) {
    if (!structure_node->structure.type.structure.dependencies.count)
        return;
    
    Type_Nodes nodes = {};
    defer { free_array(allocator, &nodes); };
    
    auto root = grow(allocator, &nodes);
    *root = {};
    root->type = &structure_node->structure.type;
    
    auto current = root;
    
    Types ordered_visited_types = {};
    
    bool did_enter = true, did_leave = false;
    while (current) {
        
        defer { advance(&current, &did_enter, &did_leave); };
        
        if (did_enter) {
            bool do_skip = false;
            for_array_item(it, ordered_visited_types) {
                if (*it == current->type) {
                    do_skip = true;
                    break;
                }
            }
            
            if (do_skip)
                continue;
            
            for_array_item(it, current->type->structure.dependencies) {
                bool is_sub_type = false;
                auto statement = structure_node->structure.body.head;
                while (statement) {
                    if (is_kind(*statement, definition)) {
                        auto structure = try_kind_of(statement->definition.value, structure);
                        if (structure && (&structure->type == *it)) {
                            is_sub_type = true;
                            break;
                        }
                    }
                    
                    statement = statement->next;
                }
                
                if (!is_sub_type)
                    continue;
                
                auto child = grow(allocator, &nodes);
                *child = {};
                child->type = *it;
                attach(current, child);
            }
            
            did_leave = (current->children.count == 0);
        }
        
        // dont add self
        if (did_leave && current->parent)
            *grow(allocator, &ordered_visited_types) = current->type;
    }
    
    auto types = ordered_visited_types;
    while (types.count) {
        
        auto type = types[types.count - 1];
        
        auto node = type->structure.node;
        
        while (node->parent != structure_node)
            node = node->parent;
        
        assert(node->parent == structure_node);
        
        // move node to front
        // we insert last types first at head
        // so we remain the original order
        detach(&structure_node->structure.body, node);
        
        insert_head(&structure_node->structure.body, node);
        node->parent = structure_node;
        
        types.count--;
    }
    
    structure_node->structure.ordered_sub_types = ordered_visited_types;
}


#if 0
Types write_tree(Memory_Allocator *allocator, Report_Context *context, Type *root) {
    assert(root);
    
    write_line_out(allocator, S("root"));
    
    if (!root->structure.dependencies.count)
        return {};
    
    Type_Path path = {};
    defer { assert(path.count == 0); };
    Types dependency_order = {};
    
    // skip root, it contains no info
    *grow(allocator, &path) = { root->structure.dependencies };
    
    bool do_add_line = false;
    
    usize depth = 0;
    bool did_enter = true;
    while (path.count) {
        defer { advance(allocator, &path, &dependency_order, &did_enter); };
        
        auto type = path[path.count - 1][0];
        
        if (!did_enter) {
            *grow(allocator, &dependency_order) = type;
            
            do_add_line |= !type->structure.dependencies.count;
            continue;
        }
        
        if (do_add_line) {
            if (path.count > 1) {
                for (u32 i = 0; i < path.count - 1; i++) {
                    // branch has a next child
                    if (path[i].count > 1)
                        write_out(allocator, S("| "));
                    else
                        write_out(allocator, S("  "));
                }
            }
            
            write_line_out(allocator, S("| "));
            
            do_add_line = false;
        }
        
        // skip visited types
        bool do_next = false;
        for_array_item(it, dependency_order) {
            if (*it == type) {
                did_enter = false;
                do_next = true;
                
                if (path.count > 1) {
                    for (u32 i = 0; i < path.count - 1; i++) {
                        // branch has a next child
                        if (path[i].count > 1)
                            write_out(allocator, S("| "));
                        else
                            write_out(allocator, S("  "));
                    }
                }
                
                write_line_out(allocator, S("|-x [%]"), f(*type));
                do_add_line = true;
                break;
            }
        }
        
        if (do_next)
            continue;
        
        // check if we enter and leave
        bool did_leave = !type->structure.dependencies.count;
        if (did_leave) {
            *grow(allocator, &dependency_order) = type;
            do_add_line = true;
        }
        
        if (path.count > 1) {
            for (u32 i = 0; i < path.count - 1; i++) {
                // branch has a next child
                if (path[i].count > 1)
                    write_out(allocator, S("| "));
                else
                    write_out(allocator, S(" "));
            }
        }
        
        write_line_out(allocator, S("|-%"), f(*type));
        
        // check new types for cyclic dependencies
        if (path.count > 1) {
            for (u32 i = 0; i < path.count - 1; i++) {
                EXPECT(context, (path[i][0] != type), S("cyclic dependencies, % and % depend on each other"), f(*path[i][0]), f(*type));
            }
        }
    }
    
    write_line_out(allocator, S("\ndependency order:\n"));
    
    for_array_item(it, dependency_order) {
        write_line_out(allocator, S("%"), f(**it));
    }
    
    return dependency_order;
}
#endif

Ast * parse_struct(Memory_Allocator *allocator, Report_Context *context, Ast *parent, Token identifier = {}, Type *parent_type = null) {
    if (!try_skip(context->tokenizer, S("struct")))
        return null;
    
    skip(context, S("{"));
    
    if (identifier.text.count) {
        string relative_path[] = {
            identifier.text
        };
        
        auto node = find_relative_type(parent, ARRAY_INFO(relative_path));
        //!node->definition.value means we are just defining this structure, so this is our parent
        EXPECT_FORMATED(context, !node || !node->definition.value, 
                        S("% error: type % allready defined\n"
                          "\n"
                          "%\n"
                          "    here: \n"
                          "\n"
                          "%:\n"
                          "\n"
                          "%"),
                        f(context->file_path, identifier.line_count, identifier.column_count), f(identifier.text),
                        f(context->file_text, identifier, 2, true, context->file_path),
                        
                        f(context->file_text, node->definition.identifier, 2, true, context->file_path))
    }
    
    auto structure_node = new_kind(allocator, Ast, structure, context->tokenizer->skipped_token);
    structure_node->structure.type = make_kind(Type, structure, structure_node);
    
    return structure_node;
}

Ast * parse_function(Memory_Allocator *allocator, Report_Context *context, Ast *parent, Types base_types, Token identifier = {}) 
{
    bool is_coroutine = try_skip(context->tokenizer, S("coroutine"));
    if (!is_coroutine && !try_skip(context->tokenizer, S("function")))
        return null;
    
    auto function_node = new_kind(allocator, Ast, function, context->tokenizer->skipped_token, is_coroutine);
    
    skip(context, S("("));
    
    if (!try_skip(context->tokenizer, S(")"))) {
        LOOP {
            auto declaration_node = ALLOCATE(allocator, Ast);
            *declaration_node = make_kind(Ast, declaration);
            insert_tail(&function_node->function.arguments, declaration_node);
            declaration_node->parent = function_node;
            
            declaration_node->declaration.identifier = get_identifier(context);
            skip(context, S(":"));
            
            declaration_node->declaration.type = parse_type(allocator, context, base_types, true);
            
            if (try_skip(context->tokenizer, S(",")))
                continue;
            
            if (try_skip(context->tokenizer, S(")")))
                break;
            
            expected_tokens(context);
        }
    }
    
    if (try_skip(context->tokenizer, S("->"))) {
        
        skip(context, S("("));
        
        LOOP {
            Type *type = parse_type(allocator, context, base_types, true);
            
            *grow(allocator, &function_node->function.return_types) = type;
            
            if (try_skip(context->tokenizer, S(",")))
                continue;
            
            if (try_skip(context->tokenizer, S(")")))
                break;
            
            expected_tokens(context);
        }
        
    }
    
    return function_node;
}

Ast::Children *children_of(Ast *parent) {
    if (!parent)
        return null;
    
    switch (parent->kind) {
        case_kind(Ast, structure)
            return &parent->structure.body;
        
        case_kind(Ast, scope)
            return &parent->scope.body;
        
        case_kind(Ast, temporary_declarations)
            return &parent->temporary_declarations.list;
        
        CASES_COMPLETE;
    }
    
    UNREACHABLE_CODE;
    return null;
}


bool attach_or_test(bool *out_is_one_child, Ast *new_parent, Ast *child, bool is_test, bool do_replace) {
    
    assert(is_test || child);
    
    Ast **destination = null;
    
    *out_is_one_child = true;
    
    switch (new_parent->kind) {
        case_kind(Ast, structure_kind);
        case_kind(Ast, declaration) {
            assert(is_test);
            return true;
        } break;
        
        case_kind(Ast, definition) {
            destination = &new_parent->definition.value;
        } break;
        
        case_kind(Ast, function) {
            destination = &new_parent->function.statement;
        } break;
        
        case_kind(Ast, conditional_branch) {
            destination = &new_parent->conditional_branch.statement;
        } break;
        
        case_kind(Ast, conditional_loop) {
            destination = &new_parent->conditional_loop.statement;
        } break;
        
        case_kind(Ast, loop) {
            destination = &new_parent->loop.statement;
        } break;
        
        default: {
            assert(!do_replace);
            *out_is_one_child = false;
            
            if (!is_test)
                attach(new_parent, children_of(new_parent), child);
            
            return false;
        }
    }
    
    if (!is_test) {
        assert(do_replace || (*destination == null));
        *destination = child;
        child->parent = new_parent;
    }
    
    return (*destination != null);
}

bool is_done(Ast *new_parent) {
    bool ignored;
    return attach_or_test(&ignored, new_parent, null, true, false);
}

void attach(Ast *new_parent, Ast *child) {
    bool ignored;
    attach_or_test(&ignored, new_parent, child, false, false);
}

bool has_only_one_child(Ast *parent) {
    bool result;
    attach_or_test(&result, parent, null, true, false);
    return result;
}

void replace_single_child(Ast *parent, Ast *child) {
    bool has_one_child;
    attach_or_test(&has_one_child, parent, child, false, true);
    assert(has_one_child);
}

void detach(Ast *parent, Ast *child) {
    assert(child->parent == parent);
    detach(children_of(parent), child);
}

Ast *children_head_of(Ast *node) {
    Ast *children_head = null;
    
    switch (node->kind) {
        case_kind(Ast, structure_kind);
        case_kind(Ast, declaration);
        case_kind(Ast, assignment);
        case_kind(Ast, comment);
        case_kind(Ast, break_statement);
        case_kind(Ast, continue_statement);
        case_kind(Ast, unparsed_expression);
        {
        } break;
        
        case_kind(Ast, temporary_declarations) {
            children_head = node->temporary_declarations.list.head;
        } break;
        
        case_kind(Ast, definition) {
            children_head = node->definition.value;
        } break;
        
        case_kind(Ast, structure) {
            children_head = node->structure.body.head;
        } break;
        
        case_kind(Ast, function) {
            children_head = node->function.statement;
        } break;
        
        case_kind(Ast, return_statement) {
            children_head = node->return_statement.values.head;
        } break;
        
        case_kind(Ast, conditional_branch) {
            children_head = node->conditional_branch.statement;
        } break;
        
        case_kind(Ast, conditional_loop) {
            children_head = node->conditional_loop.statement;
        } break;
        
        case_kind(Ast, loop) {
            children_head = node->loop.statement;
        } break;
        
        case_kind(Ast, scope) {
            children_head = node->scope.body.head;
        } break;
        
        CASES_COMPLETE;
    }
    
    return children_head;
}

void advance(Ast **iterator, bool *did_enter, bool *did_leave, usize *depth = null) {
    
    advance(iterator, children_head_of(*iterator), did_enter, depth);
    
    if (*did_enter)
        *did_leave = (children_head_of(*iterator) == null);
    else
        *did_leave = true;
}

#if 0
void move(Ast *new_parent, Ast *child) {
    move(new_parent, children_of(new_parent), child, children_of(child->parent));
}
#endif

void write(String_Buffer *buffer, u32 *indent_depth, string format, ...) {
    
    static bool do_new_line = false;
    
    va_list va_args;
    va_start(va_args, format);
    
    string text = {};
    defer { free_array(buffer->allocator, &text); };
    write_va(buffer->allocator, &text, format, va_args);
    
    va_end(va_args);
    
    auto it = text;
    
    while (it.count) {
        u32 byte_count;
        u32 head = utf8_head(it, &byte_count);
        
        if (head == '}') {
            assert(*indent_depth > 0);
            (*indent_depth)--;
        }
        
        if (do_new_line) {
            auto dest = grow(buffer->allocator, &buffer->buffer, *indent_depth * 4);
            reset(dest, *indent_depth * 4, ' ');
            do_new_line = false;
        }
        
        auto dest = grow(buffer->allocator, &buffer->buffer, byte_count);
        copy(dest, it.data, byte_count);
        
        advance(&it, byte_count);
        
        if (head == '\n') {
            do_new_line = true;
        }
        else if (head == '{')
            (*indent_depth)++;
    }
}

void write_scope_name(String_Buffer *output, u32 *indent_depth,Ast *scope_node) {
    write(output, indent_depth, S("_scope_%_%_%"), f(scope_node->scope.identifier.text), f(scope_node->scope.open_bracket.line_count), f(scope_node->scope.open_bracket.column_count));
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
    write_line_out(allocator);
    
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
    
    Type root_type = make_kind(Type, structure, null);
    
    Types base_types = {};
    
#define add_base_type(name) { \
        *grow(allocator, &base_types) = new_kind(allocator, Type, base_type, { S(# name) }, sizeof(name), alignof(name));\
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
    
    Tokenizer it = make_tokenizer(allocator, source);
    Report_Context reporter = { allocator, source_file_path, source, &it };
    
    Ast *file_scope_node = new_kind(allocator, Ast, structure);
    file_scope_node->structure.type = make_kind(Type, structure, file_scope_node);
    
    {
        
        // TODO: add filescope? or/and global scope?
        auto parent = file_scope_node;
        
        Ast *parent_namespace_node = file_scope_node;
        Ast *parent_structure_node = null;
        Ast *parent_function_node = null;
        Ast *parent_loop_node = null;
        Ast *parent_scope_node = null;
        
        while (it.text.count) {
            
            debug_break_once(it.line_count >= 45);
            
            {
                parent_function_node = null;
                parent_structure_node = null;
                parent_namespace_node = null;
                parent_loop_node = null;
                parent_scope_node = null;
                
                while (is_done(parent)) {
                    parent = parent->parent;
                }
                
                bool done = false;
                auto scope = parent;
                while (!done && scope) {
                    switch (scope->kind) {
                        case_kind(Ast, structure) {
                            if (!parent_structure_node && !parent_function_node && !parent_loop_node && !parent_scope_node) {
                                parent_structure_node = scope;
                            }
                        } break;
                        
                        case_kind(Ast, function) {
                            if (!parent_function_node && !parent_structure_node) {
                                parent_function_node = scope;
                                
                                if (parent_scope_node == scope->function.statement)
                                    parent_scope_node = null;
                            }
                        } break;
                        
                        case_kind(Ast, loop);
                        case_kind(Ast, conditional_loop) {
                            if (!parent_loop_node && !parent_structure_node) {
                                parent_loop_node = scope;
                            }
                        } break;
                        
                        case_kind(Ast, scope) {
                            if (!parent_scope_node && !parent_structure_node) {
                                parent_scope_node = scope;
                            }
                        } break;
                        
                        case_kind(Ast, definition) {
                            parent_namespace_node = scope;
                            done = true;
                        } break;
                    }
                    
                    scope = scope->parent;
                }
                
                if (!scope)
                    parent_namespace_node = file_scope_node;
            }
            
            if (is_kind(*parent, temporary_declarations)) {
                bool do_cleanup = false;
                
                switch (parent->temporary_declarations.state) {
                    case parent->temporary_declarations.State_Parse_Identifier: {
                        auto identifier = get_identifier(&reporter);
                        
                        skip(&reporter, S(":"));
                        
                        auto node = ALLOCATE(allocator, Ast);
                        if (parent->temporary_declarations.as_structure_kinds)
                            *node = make_kind(Ast, structure_kind, parent->temporary_declarations.token, identifier);
                        else
                            *node = make_kind(Ast, declaration, parent->temporary_declarations.token, identifier);
                        
                        // just temporary while processing
                        attach(parent, &parent->temporary_declarations.list, node);
                        
                        // create anonymous struct and its type
                        auto structure_node = parse_struct(allocator, &reporter, parent_namespace_node);
                        if (structure_node) {
                            node->declaration.type = &structure_node->structure.type;
                            structure_node->parent = node;
                            
                            parent->temporary_declarations.state = parent->temporary_declarations.State_Check_Continue;
                            parent = structure_node;
                            
                            continue;
                        }
                        
                        node->declaration.type = parse_type(allocator, &reporter, base_types, true);
                    }
                    
                    // falltrough
                    
                    case parent->temporary_declarations.State_Check_Continue: {
                        if (try_skip(&it, S("="))) {
                            parent->temporary_declarations.state = parent->temporary_declarations.State_Parse_Expressions;
                            continue;
                        }
                        
                        if (try_skip(&it, S(","))) {
                            parent->temporary_declarations.state = parent->temporary_declarations.State_Parse_Identifier;
                            continue;
                        }
                        
                        // if previous type was a structure_node we don't need to skip a ';'
                        auto structure_node = try_kind_of(parent->temporary_declarations.list.tail, structure);
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
                        auto node = parent->temporary_declarations.list.head;
                        auto count = parent->temporary_declarations.list.count;
                        
                        for (u32 i = 0; i < count; i++) {
                            assert(is_kind(*node, declaration));
                            
                            auto assignment_node = new_kind(allocator, Ast, assignment);
                            assignment_node->assignment.destination_unparsed_expression = node->declaration.identifier;
                            assignment_node->assignment.source_unparsed_expression = next_expression(allocator, &reporter, (i == count - 1) ? S(";") : S(","));
                            
                            attach(parent, assignment_node);
                            
                            node = node->next;
                        }
                        
                        do_cleanup = true;
                    } break;
                    
                    CASES_COMPLETE;
                }
                
                if (do_cleanup) {
                    auto declarations_node = parent;
                    parent = parent->parent;
                    
                    if (!declarations_node->temporary_declarations.as_structure_kinds) {
                        
                        bool inserted_scope = has_only_one_child(parent);
                        if (inserted_scope) {
                            // token is required to give the scope a unique name
                            // to break or continue
                            auto scope_node = new_kind(allocator, Ast, scope, {}, declarations_node->temporary_declarations.token);
                            replace_single_child(parent, scope_node);
                            parent = scope_node;
                        }
                        else {
                            detach(parent, declarations_node);
                        }
                        
                        auto declarations = kind_of(declarations_node, temporary_declarations);
                        
                        while (declarations->list.head) {
                            auto it = declarations->list.head;
                            detach(&declarations->list, it);
                            
                            attach(parent, it);
                        }
                        
                        free(allocator, declarations_node);
                        
                        if (inserted_scope)
                            parent = parent->parent;
                    }
                    
                    continue;
                }
                
                UNREACHABLE_CODE;
            }
            
            if (try_skip(&it, S("}"))) {
                
                EXPECT(&reporter, parent->parent, S("unexpected '}' without opening '{'"));
                
                switch (parent->kind) {
                    case_kind(Ast, structure) {
                        parent->structure.closed_bracked = it.skipped_token;
                        
                        // only after the whole struct is defined, we add the kinds
                        if (parent->structure.kind_union != null) {
                            parent->structure.kind_union->parent = null;
                            attach(parent, &parent->structure.body, parent->structure.kind_union);
                        }
                        
                    } break;
                    
                    case_kind(Ast, scope) {
                        parent->scope.closed_bracked = it.skipped_token;
                    } break;
                    
                    CASES_COMPLETE;
                }
                
                parent = parent->parent;
                
                continue;
            }
            
            // test for comment node
            
            // commen in structs and functions
            
            if (try_skip(&it, S("def"))) {
                auto definition_node = ALLOCATE(allocator, Ast);
                *definition_node = make_kind(Ast, definition);
                
                auto identifier = get_identifier(&reporter);
                definition_node->definition.identifier = identifier;
                attach(parent, definition_node);
                
                bool is_coroutine = try_skip(&it, S("coroutine"));
                if (is_coroutine || try_skip(&it, S("function"))) {
                    
                    auto function_node = new_kind(allocator, Ast, function, it.skipped_token, is_coroutine);
                    
                    definition_node->definition.value = function_node;
                    function_node->parent = definition_node;
                    
                    skip(&reporter, S("("));
                    
                    if (!try_skip(&it, S(")"))) {
                        LOOP {
                            auto declaration_node = ALLOCATE(allocator, Ast);
                            *declaration_node = make_kind(Ast, declaration);
                            insert_tail(&function_node->function.arguments, declaration_node);
                            declaration_node->parent = function_node;
                            
                            declaration_node->declaration.identifier = get_identifier(&reporter);
                            skip(&reporter, S(":"));
                            
                            declaration_node->declaration.type = parse_type(allocator, &reporter, base_types, true);
                            
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
                            Type *type = parse_type(allocator, &reporter, base_types, true);
                            
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
                    auto structure_node = parse_struct(allocator, &reporter, parent_namespace_node, identifier);
                    if (structure_node) {
                        definition_node->definition.value = structure_node;
                        structure_node->parent = definition_node;
                        
                        parent = structure_node;
                        continue;
                    }
                }
                
                expected_tokens(&reporter);
            } // "def"
            
            {
                Ast *declarations_node = null;
                if (is_kind(*parent, structure) && try_skip(&it, S("kind"))) {
                    
                    if (!parent->structure.kind_union) {
                        assert(!parent->structure.kind_union);
                        declarations_node = new_kind(allocator, Ast, temporary_declarations, it.skipped_token, true);
                        parent->structure.kind_union = declarations_node;
                        
                        // dont add to body
                        // only after the whole struct is defined, we add the kinds
                        // see "}" for structures
                        declarations_node->parent = parent;
                    }
                    
                    declarations_node = parent->structure.kind_union;
                    
                    parent = declarations_node;
                    continue;
                }
                
                if (!declarations_node && try_skip(&it, S("var"))) {
                    declarations_node = ALLOCATE(allocator, Ast);
                    *declarations_node = make_kind(Ast, temporary_declarations, it.skipped_token, false);
                    attach(parent, declarations_node);
                    
                    parent = declarations_node;
                    continue;
                }
            }
            
            // function only
            
            if (parent_function_node) {
                {
                    Ast *conditional_node = null;
                    bool is_loop;
                    if (try_skip(&it, S("if"))) {
                        conditional_node = new_kind(allocator, Ast, conditional_branch);
                    }
                    
                    if (!conditional_node && try_skip(&it, S("while"))) {
                        conditional_node = new_kind(allocator, Ast, conditional_loop);
                    }
                    
                    if (conditional_node) {
                        auto unparsed_expression_node = new_kind(allocator, Ast, unparsed_expression);
                        
                        unparsed_expression_node->parent = conditional_node;
                        unparsed_expression_node->unparsed_expression = next_expression(allocator, &reporter, S("do"), 1);
                        
                        if (is_kind(*conditional_node, conditional_branch))
                            conditional_node->conditional_branch.condition = unparsed_expression_node;
                        else
                            conditional_node->conditional_loop.condition = unparsed_expression_node;
                        
                        attach(parent, conditional_node);
                        parent = conditional_node;
                        continue;
                    }
                }
                
                if (try_skip(&it, S("loop"))) {
                    auto loop_node = new_kind(allocator, Ast, loop);
                    attach(parent, loop_node);
                    parent = loop_node;
                    continue;
                }
                
                Ast *return_or_yield_node = null;
                if (try_skip(&it, S("return"))) {
                    return_or_yield_node = new_kind(allocator, Ast, return_statement, false);
                }
                
                if (!return_or_yield_node && try_skip(&it, S("yield")))
                {
                    EXPECT_TOKEN(&reporter, parent_function_node->function.is_coroutine, it.skipped_token, S("'yield' is only allowed inside a coroutine body"));
                    return_or_yield_node = new_kind(allocator, Ast, return_statement, it.skipped_token, true);
                }
                
                if (return_or_yield_node) {
                    attach(parent, return_or_yield_node);
                    
                    for (u32 i = 0; i < parent_function_node->function.return_types.count; i++) {
                        
                        auto unparsed_expression_node = ALLOCATE(allocator, Ast);
                        *unparsed_expression_node = make_kind(Ast, unparsed_expression);
                        insert_tail(&return_or_yield_node->return_statement.values, unparsed_expression_node);
                        unparsed_expression_node->parent = return_or_yield_node;
                        
                        unparsed_expression_node->unparsed_expression = next_expression(allocator, &reporter, (i == parent_function_node->function.return_types.count - 1) ? S(";") : S(","), parent_function_node->function.return_types.count);
                        EXPECT_TOKEN(&reporter, unparsed_expression_node->unparsed_expression.text.count, unparsed_expression_node->unparsed_expression, S("expected expression"));
                    }
                    
                    continue;
                }
                
                {
                    Ast *break_or_continue_node = null;
                    if (try_skip(&it, S("break"))) {
                        break_or_continue_node = new_kind(allocator, Ast, break_statement, it.skipped_token);
                    }
                    
                    if (!break_or_continue_node && try_skip(&it, S("continue"))) {
                        break_or_continue_node = new_kind(allocator, Ast, continue_statement, it.skipped_token);
                    }
                    
                    if (break_or_continue_node) {
                        if (is_kind(*break_or_continue_node, break_statement)) {
                            EXPECT_TOKEN(&reporter, parent_scope_node, it.skipped_token, S("'break' is only allowed inside sub scopes of a function"));
                        }
                        else {
                            EXPECT_TOKEN(&reporter, parent_loop_node, it.skipped_token, S("'continue' is only allowed inside loops of a function"));
                        }
                        
                        Ast *parent_scope = null;
                        {
                            auto parent_scope_identifier = try_skip_identifier(&it);
                            
                            auto it = parent;
                            while (it != parent_function_node) {
                                if (is_kind(*it, scope)) {
                                    if ((parent_scope_identifier.text.count == 0) || (it->scope.identifier.text == parent_scope_identifier.text)) {
                                        parent_scope = it;
                                        break;
                                    }
                                }
                                
                                it = it->parent;
                            }
                            
                            EXPECT_TOKEN(&reporter, (parent_scope_identifier.text.count == 0) || parent_scope, parent_scope_identifier, S("unknown parent scope '%'"), f(parent_scope_identifier.text));
                        }
                        
                        skip(&reporter, S(";"));
                        
                        if (is_kind(*break_or_continue_node, break_statement))
                            break_or_continue_node->break_statement.parent_scope = parent_scope;
                        else
                            break_or_continue_node->continue_statement.parent_scope = parent_scope;
                        
                        attach(parent, break_or_continue_node);
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
                {
                    // scopes can have a leading identifer as their names
                    auto test = begin(&it);
                    string identifier = get_identifier(&it.text, S("_"));
                    skip_set(&it.text, White_Space_Ex);
                    
                    if (try_skip(&it.text, S("{"))) {
                        revert(test);
                        
                        Token name = {};
                        if (identifier.count)
                            name = get_identifier(&reporter);
                        
                        //TODO: print function location
                        if ((parent_function_node == parent) && name.text.count) {
                            EXPECT_FORMATED(&reporter, 0, 
                                            S("%: error function body must not be named\n"
                                              "\n"
                                              "%\n"
                                              "\n"
                                              "    of function:\n"
                                              "\n"
                                              "%"),
                                            f(reporter.file_path, name.line_count, name.column_count),
                                            f(reporter.file_text, name, 2),
                                            f(reporter.file_text, parent_function_node->function.token, 2, true, reporter.file_path));
                            
                        }
                        
                        skip(&reporter, S("{"));
                        
                        auto scope_node = ALLOCATE(allocator, Ast);
                        *scope_node = make_kind(Ast, scope, name, it.skipped_token);
                        
                        attach(parent, scope_node);
                        
                        parent = scope_node;
                        continue;
                    }
                    else {
                        revert(test);
                    }
                }
                
                // if all else fails, get en unparsed expression!!!
                {
                    auto unparsed_expression = next_expression(allocator, &reporter, S(";"));
                    if (unparsed_expression.text.count) {
                        auto expression_node = new_kind(allocator, Ast, unparsed_expression, unparsed_expression);
                        attach(parent, expression_node);
                    }
                    
                    continue;
                }
            } // function only stuff
            
            expected_tokens(&reporter);
        }
        
        // check if all scopes are done
        while (is_done(parent)) {
            parent = parent->parent;
        }
        
        // file is parsed but we are not at file_scope_node
        if (parent != file_scope_node) {
            switch (parent->kind) {
                case_kind(Ast, scope) {
                    EXPECT_TOKEN(&reporter, 0, parent->scope.open_bracket, S("scope not closed"));
                } break;
                
                case_kind(Ast, structure) {
                    EXPECT_TOKEN(&reporter, 0, parent->structure.open_bracket, S("structure not closed"));
                } break;
                
                CASES_COMPLETE;
            }
        }
    }
    
    // replace undeclared types
    {
        auto current = file_scope_node->structure.body.head;
        bool did_enter = true;
        
        while (current) {
            Type **type_storage = null;
            
            // find parent namespace
            Ast *parent_namespace = null;
            Type *parent_type = null;
            {
                auto it = current->parent;
                
                while (it && (!parent_type || !parent_namespace)) {
                    if (!parent_namespace && is_kind(*it, definition)) {
                        parent_namespace = it;
                    } else if (!parent_type && is_kind(*it, structure)) {
                        parent_type = &it->structure.type;
                    }
                    
                    it = it->parent;
                }
                
                assert((current == file_scope_node) || parent_type);
                
                if (!parent_namespace)
                    parent_namespace = file_scope_node;
            }
            
            Ast *next = null;
            switch (current->kind) {
                
                case_kind(Ast, structure_kind);
                case_kind(Ast, declaration) {
                    if (did_enter) {
                        if (is_kind(*current->declaration.type, structure)) {
                            next = current->declaration.type->structure.node;
                        }
                        else {
                            replace_undeclared_type(allocator, &reporter, parent_namespace, parent_type, &current->declaration.type);
                        }
                    }
                } break;
                
                case_kind(Ast, definition) {
                    if (did_enter)
                        next = current->definition.value;
                } break;
                
                case_kind(Ast, structure) {
                    if (did_enter) {
                        // empty struct, just skipp
                        if (!current->structure.body.head)
                            break;
                        
                        next = current->structure.body.head;
                    }
                    else {
                        if (parent_type)
                            add_dependencies(allocator, &reporter, parent_type, &current->structure.type);
                        else {
                            assert(current == file_scope_node);
                        }
                        
                        write_line_out(allocator, S("%:\n"), f(current->structure.type, true));
                        
                        write_line_out(allocator, S("  dependencies:"));
                        
                        for_array_item(it, current->structure.type.structure.dependencies)
                            write_line_out(allocator, S("    %"), f(**it, true));
                        
                        write_line_out(allocator);
                        
                        order_sub_types(allocator, &reporter, current);
                        write_line_out(allocator, S("  ordered sub types:"));
                        
                        for_array_item(it, current->structure.ordered_sub_types)
                            write_line_out(allocator, S("    %"), f(**it, true));
                        
                        write_line_out(allocator);
                    }
                } break;
                
                case_kind(Ast, function) {
                    if (did_enter) {
                        for_array_item(type, current->function.return_types)
                            replace_undeclared_type(allocator, &reporter, parent_namespace, parent_type, type);
                        
                        for_list_item(argument, current->function.arguments)
                            replace_undeclared_type(allocator, &reporter, parent_namespace, parent_type, &argument->declaration.type);
                        
                        next = current->function.statement;
                    }
                } break;
                
                case_kind(Ast, temporary_declarations) {
                    if (did_enter) {
                        assert(current->temporary_declarations.as_structure_kinds);
                        next = current->temporary_declarations.list.head;
                    }
                } break;
                
                case_kind(Ast, assignment);
                case_kind(Ast, unparsed_expression);
                case_kind(Ast, return_statement);
                case_kind(Ast, break_statement);
                case_kind(Ast, continue_statement) {
                } break;
                
                case_kind(Ast, conditional_branch) {
                    if (did_enter)
                        next = current->conditional_branch.statement;
                } break;
                
                case_kind(Ast, conditional_loop) {
                    if (did_enter)
                        next = current->conditional_loop.statement;
                } break;
                
                case_kind(Ast, loop) {
                    if (did_enter)
                        next = current->loop.statement;
                } break;
                
                // TODO: add scope name to namespace_path
                case_kind(Ast, scope) {
                    // empty scop, just skipp
                    if (!current->scope.body.head)
                        break;
                    
                    if (did_enter)
                        next = current->scope.body.head;
                } break;
                
                CASES_COMPLETE;
            }
            
            if (!next) {
                advance_up_or_right(&current, &did_enter);
            }
            else {
                current = next;
                did_enter = true;
            }
        }
    }
    
    { 
        write_line_out(transient_allocator, S("type dependencies\n\n"));
        
#if 0        
        for_array_item(it, file_scope_node->structure.type.structure.dependencies) {
            write_line_out(transient_allocator, S("%"), f(**it, true));
        }
#endif
        
        //write_tree(allocator, &reporter, &file_scope_node->structure.type);
    }
    
    // collect functions
    Ast_Array functions = {};
    {
        auto node = file_scope_node->structure.body.head;
        
        bool did_enter = true, did_leave = false;
        while (node) {
            defer { advance(&node, &did_enter, &did_leave); };
            
            if_kind(node, function) {
                if (did_enter) {
                    *grow(allocator, &functions) = node;
                }
            }
        }
    }
    
    String_Buffer output = { allocator };
    
    // generate c code
    {
        
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
        
        write(&output, S("\r\n"));
        
        u32 indent_depth = 0;
        
        auto node = file_scope_node->structure.body.head;
        bool did_enter = true;
        
        // forward declare sub structs
        for_array_item(it, file_scope_node->structure.ordered_sub_types) {
            write(&output, &indent_depth, S("struct %;\n"), f(**it));
        }
        
        write(&output, &indent_depth, S("\n"));
        
        while (node) {
            
            Ast *node_children_head = null;
            
            defer { advance(&node, node_children_head, &did_enter); };
            
            switch (node->kind) {
                
                case_kind(Ast, temporary_declarations) {
                    assert(node->temporary_declarations.as_structure_kinds);
                    
                    if (did_enter) {
                        write(&output, &indent_depth, S("\nunion {\n"));
                        
                        node_children_head = node->temporary_declarations.list.head;
                    }
                    else {
                        write(&output, &indent_depth, S("};\n"));
                    }
                    
                    continue;
                } break;
                
                
                case_kind(Ast, structure) {
                    if (did_enter) {
                        if (node->parent && is_kind(*node->parent, definition))
                            write(&output, &indent_depth, S("struct % {\n"), f(node->parent->definition.identifier.text));
                        else
                            write(&output, &indent_depth, S("struct {\n"));
                        
                        
                        auto structure = kind_of(node, structure);
                        
                        // forward declare sub structs
                        if (structure->ordered_sub_types.count) {
                            write(&output, &indent_depth, S("\n"));
                            
                            for_array_item(it, structure->ordered_sub_types) {
                                write(&output, &indent_depth, S("struct %;\n"), f(**it));
                            }
                            
                            write(&output, &indent_depth, S("\n"));
                        }
                        
                        if (structure->kind_union) {
                            write(&output, &indent_depth, S("enum Kind {\n"));
                            write(&output, &indent_depth, S("Kind_null,\n"));
                            
                            auto kind = structure->kind_union->temporary_declarations.list.head;
                            while (kind) {
                                write(&output, &indent_depth, S("Kind_%,\n"), f(kind->declaration.identifier.text));
                                kind = kind->next;
                            }
                            
                            write(&output, &indent_depth, S("};\n\n"));
                            
                            write(&output, &indent_depth, S("static const u32 Kind_Count = %;\n\n"), f(structure->kind_union->temporary_declarations.list.count + 1));
                            
                            // TODO: find best matching size
                            write(&output, &indent_depth, S("u32 kind;\n\n"));
                        }
                        
                        node_children_head = structure->body.head;
                    }
                    else {
                        // skip file scope
                        if (node->parent) {
                            // anonymous structs dont get closed with a ';'
                            if (is_kind(*node->parent, definition))
                                write(&output, &indent_depth, S("};\n\n"));
                            else
                                write(&output, &indent_depth, S("}"));
                        }
                    }
                    
                    continue;
                } break;
                
                case_kind(Ast, function) {
                    if (!did_enter) {
                        write(&output, &indent_depth, S("}\n\n"));
                        continue;
                    }
                    
                } break;
                
                case_kind(Ast, scope) {
                    if (!did_enter && node->parent && !is_kind(*node->parent, function)) {
                        
                        //if (node->scope.identifier.text.count)
                        write(&output, &indent_depth, S("} "));
                        write_scope_name(&output, &indent_depth, node);
                        write(&output, &indent_depth, S("_end:;\n\n"));
                        //else
                        //write(&output, &indent_depth, S("}\n\n"));
                        
                        continue;
                    }
                    else if (did_enter) {
                        write(&output, &indent_depth, S("/* "));
                        write_scope_name(&output, &indent_depth, node);
                        write(&output, &indent_depth, S(" */ {\n"));
                        
                        //if (node->scope.identifier.text.count)
                        write_scope_name(&output, &indent_depth, node);
                        write(&output, &indent_depth, S("_begin:;\n"));
                        
                        node_children_head = node->scope.body.head;
                        continue;
                    }
                    
                } break;
                
                case_kind(Ast, structure_kind);
                case_kind(Ast, declaration) {
                    if (did_enter) {
                        // its an anonymous struct we own
                        if (is_kind(*node->declaration.type, structure) && (node->declaration.type->structure.node->parent == node)) {
                            node_children_head = node->declaration.type->structure.node;
                        }
                        else {
                            write(&output, &indent_depth, S("% %;\n"), f(*node->declaration.type), f(node->declaration.identifier.text));
                        }
                    }
                    else {
                        assert(is_kind(*node->declaration.type, structure));
                        write(&output, &indent_depth, S(" %;\n"), f(node->declaration.identifier.text));
                    }
                    
                    continue;
                } break;
                
                case_kind(Ast, assignment) {
                    assert(did_enter);
                    write(&output, &indent_depth, S("% = %;\n"), f(node->assignment.destination_unparsed_expression.text), f(node->assignment.source_unparsed_expression.text));
                    continue;
                } break;
                
                case_kind(Ast, conditional_branch) {
                    if (did_enter) {
                        write(&output, &indent_depth, S("if (%) "), f(node->conditional_branch.condition->unparsed_expression.text));
                        node_children_head = node->conditional_branch.statement;
                    }
                    continue;
                    
                } break;
                
                case_kind(Ast, conditional_loop) {
                    if (did_enter) {
                        write(&output, &indent_depth, S("while (%) "), f(node->conditional_branch.condition->unparsed_expression.text));
                        node_children_head = node->conditional_loop.statement;
                    }
                    continue;
                    
                } break;
                
                case_kind(Ast, loop) {
                    if (did_enter) {
                        write(&output, &indent_depth, S("for (;;) "));
                        node_children_head = node->loop.statement;
                    }
                    continue;
                } break;
                
                case_kind(Ast, return_statement) {
                    if (did_enter) {
                        if (node->return_statement.values.count > 1) {
                            auto it = node->return_statement.values.head->next;
                            
                            u32 i = 1;
                            while (it) {
                                assert(is_kind(*it, unparsed_expression));
                                write(&output, &indent_depth, S("(*_out%) = %;\n"), f(i), f(it->unparsed_expression.text));
                                i++;
                                it = it->next;
                            }
                        }
                        
                        if (node->return_statement.values.count > 0) {
                            assert(is_kind(*node->return_statement.values.head, unparsed_expression));
                            
                            write(&output, &indent_depth, S("return %;\n"), f(node->return_statement.values.head->unparsed_expression.text));
                        }
                        else {
                            write(&output, &indent_depth, S("return "));
                        }
                    } 
                    continue;
                } break;
                
                case_kind(Ast, continue_statement) {
                    assert(did_enter);
                    auto scope = node->break_statement.parent_scope;
                    write(&output, &indent_depth, S("goto "));
                    write_scope_name(&output, &indent_depth, scope);
                    write(&output, &indent_depth, S("_begin; // continue\n"));
                    continue;
                } break;
                
                case_kind(Ast, break_statement) {
                    assert(did_enter);
                    auto scope = node->break_statement.parent_scope;
                    write(&output, &indent_depth, S("goto "));
                    write_scope_name(&output, &indent_depth, scope);
                    write(&output, &indent_depth, S("_end; // break\n"));
                    continue;
                } break;
                
                case_kind(Ast, unparsed_expression) {
                    assert(did_enter);
                    write(&output, &indent_depth, S("%;\n"), f(node->unparsed_expression.text));
                    continue;
                } break;
            }
            
            if_kind(node, definition) {
                if (did_enter) {
                    if_kind(definition->value, structure) {
                        node_children_head = definition->value;
                        continue;
                    }
                    
                    if_kind(definition->value, function) {
                        if (function->return_types.count) 
                            write(&output, &indent_depth, S("% "), f(*function->return_types[0]));
                        else
                            write(&output, &indent_depth, S("void "));
                        
                        write(&output, &indent_depth, S("%("), f(definition->identifier.text));
                        
                        u32 i = 1;
                        for (; i < function->return_types.count; i++) {
                            if (i < function->return_types.count + function->arguments.count - 1)
                                write(&output, &indent_depth, S("%* _out%, "), f(*function->return_types[i]), f(i));
                            else
                                write(&output, &indent_depth, S("%* _out%\n"), f(*function->return_types[i]), f(i));
                        }
                        
                        for_list_item(argument_node, function->arguments) {
                            auto argument = kind_of(argument_node, declaration);
                            
                            if (i < function->return_types.count + function->arguments.count - 1)
                                write(&output, &indent_depth, S("% %, "), f(*argument->type), f(argument->identifier.text));
                            else
                                write(&output, &indent_depth, S("% %"), f(*argument->type), f(argument->identifier.text));
                            
                            i++;
                        }
                        
                        write(&output, &indent_depth, S(") {\n"));
                        
                        node_children_head = function->statement;
                        
                        if (is_kind(*node_children_head, scope))
                            node_children_head = node_children_head->scope.body.head;
                        
                        continue;
                    }
                }
            }
        }
    }
    
    
#if 1
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