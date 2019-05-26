#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

#include <string>
#include <vector>
#include <bitset>
#include <unordered_map>

#include "auxlib.h"
#include "astree.h"
using namespace std;

// Attributes for types and properties (2.2)
enum class attr {
    VOID, INT, NULLPTR_T, STRING, STRUCT, ARRAY, FUNCTION, VARIABLE,
    FIELD, TYPEID, PARAM, LOCAL, LVAL, CONST, VREG, VADDR, BITSET_SIZE
};
using attr_bitset = bitset<unsigned(attr::BITSET_SIZE)>;

struct astree;
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
};

const string to_string (attr attribute);

// Traverse Functions
void traverse(FILE* outfile, astree* tree, int depth = 0);
void traverse_struct(astree* root, symbol* sym);
void traverse_function(astree* root, symbol* sym);
void traverse_block(astree* root, int cur_block);
void process_id(astree* root);

// Function helper functions
void fn_read_param(astree* root, symbol* func_sym, size_t block_nr);
void fn_read_vardecl(astree* root, size_t block_nr, int seq_num);

// Print functions
void print_param(symbol* sym, astree* type, astree* name);
void print_func(symbol* sym, astree* type, astree* name);
void print_local_var(symbol* sym, astree* type, astree* name);

// Setting attribute of a symbol
void set_attr(symbol* sym,  attr a1);

#endif