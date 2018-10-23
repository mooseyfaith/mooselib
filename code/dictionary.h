#if !defined(_ENGINE_DICTIONARY_H_)
#define _ENGINE_DICTIONARY_H_

#include "mo_string.h"

struct Dictionary;

struct Dictionary_Entry_Header {
    string key;
};

Dictionary_Entry_Header * get_entry(bool *optional_ok, Dictionary *dict, string key);

struct Dictionary {
    u32 count;
    Dictionary_Entry_Header **entries;
    
    Dictionary_Entry_Header * operator[](string key) {
        bool ok;
        Dictionary_Entry_Header *v = get_entry(&ok, this, key);
        assert(ok);
        return v;
    }
};

Dictionary make_dictionary(Memory_Allocator *allocator, u32 count) {
    Dictionary dict;
    dict.count = count;
    dict.entries = ALLOCATE_ARRAY(allocator, Dictionary_Entry_Header *, count);
    
    for (Dictionary_Entry_Header **it = dict.entries, **end = it + count; it != end; ++it)
        *it = NULL;
    
    return dict;
}

u32 hash_djb2(const u8 *it, const u8 *end) {
    u32 hash = 5381;
    
    while (it != end)
        hash = ((hash << 5) + hash) + *(it++); /* hash * 33 + c */
    
    return hash;
}

#define GET_DICTIONARY_ENTRY(optional_ok, dict, key, type) ((type *)get_entry(optional_ok, dict, key))

Dictionary_Entry_Header * get_entry(bool *optional_ok, Dictionary *dict, string key) {
    u32 slot = hash_djb2(key, one_past_last(key)) % dict->count;
    
    if (!dict->entries[slot])
        goto failed;
    
    if (dict->entries[slot]->key != key) {
        u32 i = slot + 1;
        while (i != slot) {
            if (!dict->entries[i])
                goto failed;
            
            if (dict->entries[i]->key == key)
                break;
            
            i = (i + 1) % dict->count;
        }
        
        if (i == slot) // searched for every entry in the book ... (pun intended)
            goto failed;
        
        slot = i;
    }
    
    if (optional_ok)
        *optional_ok = true;
    
    return dict->entries[slot];
    
    failed:
    if (optional_ok)
        *optional_ok = false;
    
    return NULL;
}

#define SET_DICTIONARY_ENTRY(dict, entry) set_entry(dict, (Dictionary_Entry_Header *)entry)

void set_entry(Dictionary *dict, Dictionary_Entry_Header *entry) {
    u32 slot = hash_djb2(entry->key, one_past_last(entry->key)) % dict->count;
    
    if (dict->entries[slot] && dict->entries[slot]->key != entry->key) {
        u32 i = slot + 1;
        while (i != slot && dict->entries[i] && dict->entries[i]->key != entry->key)
            i = (i + 1) % dict->count;
        
        assert(i != slot);
        slot = i;
    }
    
    dict->entries[slot] = entry;
}

#endif
