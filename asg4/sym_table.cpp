#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <bitset>

#include "sym_table.h"
#include "astree.h"
#include "lyutils.h"



// Symbol Tables
symbol_table struct_table;
symbol_table struct_field_table;
symbol_table ident_table_global;
symbol_table ident_table_local;


void traverse(FILE* outfile, astree* root, int depth) {
   if (root == NULL) return;
   // doNode(outfile, root); // Main node checker

   symbol* test = new symbol();
   test->attributes.set(unsigned(attr::VADDR));
   std::cout << test->attributes << "\n";


   fprintf (outfile, "%-20s", parser::get_tname(root->symbol));

   switch (root->symbol) {
      case TOK_STRUCT    :{printf("struct found\n"); break;} // case (4b) set attribute to attr::STRUCT 
      case TOK_FUNCTION  :{traverse_function(root); break;} // case (4h) set attribute to attr::FUNCTION
      case TOK_VARDECL   :{traverse_vardecl(root, new symbol()); break;} // case (4k)
      case TOK_IDENT     :{printf("ident found\n"); break;} // case (4f)
      default              : {
      for (astree* child: root->children) traverse(outfile, child, depth + 1);
      break;}
   }
}

void traverse_function(astree* root) {
   int seq_num;
   int param_num;
   switch (root->symbol) {
      case TOK_PARAM   : param_num++;
      case TOK_BLOCK     :{printf("block found\n"); break;} // case (4h)
      case TOK_VARDECL : seq_num++;
   }
}

void traverse_vardecl(astree* root, symbol* sym) {
   switch (root->symbol) {
      case TOK_INT     :{sym->attributes.set(attr::INT); break;} // case (4h)
      case TOK_TYPE_ID     :{printf("ident found\n"); break;} // case (4f)
      case TOK_TYPE_ID     :{printf("ident found\n"); break;} // case (4f)
      default              : {
      for (astree* child: root->children) traverse(outfile, child, depth + 1);
      break;}
   }
}

// void tcheck_struct(astree* root) {

// }