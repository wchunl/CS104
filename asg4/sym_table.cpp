#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <bitset>

#include "sym_table.h"
#include "astree.h"
#include "lyutils.h"

using namespace std;


// Symbol Tables
symbol_table struct_table;
symbol_table struct_field_table;
symbol_table ident_table_global;
symbol_table ident_table_local;


void traverse(FILE* outfile, astree* root, int depth) {
   if (root == NULL) return;
   // doNode(outfile, root); // Main node checker

   // symbol* test = new symbol();
   // test->attributes.set(unsigned(attr::VADDR));
   // std::cout << test->attributes << "\n";


   fprintf (outfile, "%-20s", parser::get_tname(root->symbol));

   switch (root->symbol) {
      case TOK_STRUCT    :{printf("struct unimplemented\n"); break;} // case (4b)
      case TOK_FUNCTION  :{traverse_function(root, new symbol()); break;} // case (4h)
      case TOK_VARDECL   :{printf("vardecl unimplemented\n"); break;} // case (4k)
      case TOK_IDENT     :{printf("ident unimplemented\n"); break;} // case (4f)
      default            : {
      for (astree* child: root->children) traverse(outfile, child, depth + 1);
      break;}
   }
}

void traverse_function(astree* root, symbol* func_sym) {
   int seq_num = 0;
   // int param_num = 0;
   for (astree* child: root->children) {
      switch (child->symbol) {
         case TOK_PARAM    :  {fn_read_param(child, func_sym, seq_num); break;}
         case TOK_BLOCK    :  {printf("block found\n"); break;} // case (4h)
         case TOK_VARDECL  :  {seq_num++; break;}
         default           :  break;
      }
   }
}

// Reads all parameters of a function
void fn_read_param(astree* root, symbol* func_sym, size_t block_nr) {
   for (astree* child: root->children) {
      if (child->symbol == TOK_TYPE_ID) { // Just in case, dont need this line
         if (func_sym->parameters == nullptr)
            func_sym->parameters = new vector<symbol *>;
         symbol* param_sym = new symbol();
         set_attr(param_sym, attr::VARIABLE);
         set_attr(param_sym, attr::LVAL);
         set_attr(param_sym, attr::PARAM);
         param_sym->lloc = child->lloc;
         param_sym->block_nr = block_nr;
         print_ident(param_sym, child->children[0], child->children[1]);
         func_sym->parameters->push_back(param_sym);
      }
   }
}

// Printing to stdout for now
void print_ident(symbol* sym, astree* type, astree* name) {
   printf("%s (lloc) {block_nr} %s\n", name->lexinfo->c_str(), type->lexinfo->c_str());
   // Need to print out attribute strings
}


void set_attr(symbol* sym, attr a1) {
   sym->attributes.set(unsigned(a1));
}


// void traverse_vardecl(astree* root, symbol* sym) {
//    switch (root->symbol) {
//       case TOK_INT     :{sym->attributes.set(attr::INT); break;} // case (4h)
//       case TOK_TYPE_ID     :{printf("ident found\n"); break;} // case (4f)
//       default              : {
//       for (astree* child: root->children) traverse(outfile, child, depth + 1);
//       break;}
//    }
// }