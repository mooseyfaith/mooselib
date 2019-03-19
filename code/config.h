#if !defined CONFIG_H
#define CONFIG_H

#include "platform.h"
#include "mo_string.h"

struct Config_Table {
    u32 variable_count;
    string *variable_names;
    string *variable_values;
};

//Config_Table make_config_table(const char *file_path, Platform_Read_File_Func read_file, Memory_Stack *memory) {

Config_Table make_config_table(const string config_text, Memory_Allocator *allocator) {
    if (!config_text.count)
        return {};
    
    string white_spaces = S(" \t");
    string line_endings = S("\r\n\0");
    string token_endings = S(" \t\r\n\0"); 
    
    string it = config_text;
    u32 variable_count = 0;
    
    while (it.count) {
        bool changed = false;
        if (skip_set(&it, white_spaces).count)
            changed = true;
        
        string line = skip_until_first_in_set(&it, line_endings);
        skip_set(&it, line_endings);
        if (line.count) {
            ++variable_count;
            changed = true;
        }
        
        if (!changed)
            break;
    }
    
    Config_Table config_table;
    config_table.variable_count = variable_count;
    config_table.variable_names = ALLOCATE_ARRAY(allocator, string, variable_count);
    config_table.variable_values = ALLOCATE_ARRAY(allocator, string, variable_count);
    
    it = config_text;
    variable_count = 0;
    
    while (it.count) {
        bool changed = false;
        
        if (skip_set(&it, white_spaces).count)
            changed = true;
        
        string line = skip_until_first_in_set(&it, line_endings);
        skip_set(&it, line_endings);
        
        if (line.count) {
            usize max_length;
            
            string token = skip_until_first_in_set(&line, token_endings);			
            config_table.variable_names[variable_count] = make_string(allocator, token);
            
            skip_set(&line, white_spaces);
            string value = skip_until_first_in_set(&line, line_endings);
            
            if (value.count)
                config_table.variable_values[variable_count] = make_string(allocator, value);
            else
                config_table.variable_values[variable_count] = EMPTY_STRING;
            
            ++variable_count;
            changed = true;
        }
        
        if (!changed)
            break;
    }
    
    return config_table;
}

Config_Table make_config_table(string file_path, Platform_Read_Entire_File_Function read_entire_file, Memory_Allocator *allocator, Memory_Allocator *temporary_allocator) {
    string text = read_entire_file(file_path, temporary_allocator);
    Config_Table table = make_config_table(text, allocator);
    free(temporary_allocator, text.data);
    
    return table;
}

string get_config_value(Config_Table *table, string variable_name) {
    for (string *it = table->variable_names, *end = it + table->variable_count;	it != end; ++it) {
        if (*it == variable_name)
            return table->variable_values[it - table->variable_names];
    }
    
    return EMPTY_STRING;
}

#endif // CONFIG_H