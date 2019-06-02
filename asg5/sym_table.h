#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

#include <string>
#include <vector>
#include <bitset>
#include <unordered_map>

#include "auxlib.h"
#include "astree.h"
using namespace std;

struct symbol;
using symbol_table = unordered_map<string*,symbol*>;
using symbol_entry = symbol_table::value_type;

// Symbol table node (3.0)
struct symbol {
    attr_bitset attributes;
    size_t sequence;
    symbol_table* fields;
    location lloc;
    size_t block_nr;
    vector<symbol*>* parameters;
    attr type;
};

const string to_string (attr attribute);
attr to_attr (const string* string);

// Traverse Functions
void traverse(FILE* outfile, astree* tree, int depth = 0);
void traverse_struct(astree* root, symbol* sym);
void traverse_function(astree* root, symbol* sym);
void traverse_block(astree* root, int cur_block);
void process_id(astree* root);

// Function helper functions
void fn_read_param(astree* root, symbol* func_sym, size_t block_nr);
void fn_read_vardecl(astree* root, size_t block_nr, int seq_num, 
                            symbol_table* id_table_local);

// Typecheck Functions
bool typecheck(astree* root);
bool tc_bin_op(astree* root);
bool tc_bool_op(astree* root);
attr get_type(const string* key);
attr find_type(astree* root);
location get_lloc(const string* key);

// Print Functions
void print_local_ident(symbol* sym, astree* type, astree* name);
void print_func(symbol* sym, astree* type, astree* name);
void print_struct(symbol* sym, astree* name);
void print_field(symbol* sym, astree* type, astree* name);
void print_globalid(symbol* sym, astree* type, astree* name);

// Other
void set_attr(symbol* sym,  attr a1);
void insert_table_node(astree* name, symbol* sym, symbol_table* st);
void dump_tables();


#endif
