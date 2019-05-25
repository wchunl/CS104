#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <bitset>

#include "sym_table.h"
#include "astree.h"
#include "lyutils.h"

using namespace std;


// Symbol Tables
symbol_table *struct_table;
//symbol_table struct_field_table;
symbol_table *ident_table_global;
symbol_table *ident_table_local;


void traverse(FILE* outfile, astree* root, int depth) {
   if (root == NULL) return;
   // doNode(outfile, root); // Main node checker

   // symbol* test = new symbol();
   // test->attributes.set(unsigned(attr::VADDR));
   // std::cout << test->attributes << "\n";



   switch (root->symbol) {
      case TOK_STRUCT    :{traverse_struct(root,new symbol()); break;} // case (4b)
      case TOK_FUNCTION  :{traverse_function(root, new symbol()); break;} // case (4h)
      case TOK_VARDECL   :{printf("vardecl unimplemented\n"); break;} // case (4k)
      case TOK_IDENT     :{process_id(root); break;} // case (4f)
      default            : {
      for (astree* child: root->children) traverse(outfile, child, depth + 1);
      break;}
   }

   fprintf (outfile, "%-20s", parser::get_tname(root->symbol));
}

void traverse_struct(astree* root, symbol* struct_sym){
   size_t seq_num = 0;
   set_attr(struct_sym,attr::STRUCT);//remember to print struct name after this
   string* struct_name = const_cast<string *>(root->children[0]->lexinfo);
   struct_sym->block_nr = 0;
   struct_sym->fields = new symbol_table();
   if (struct_name == nullptr) {
        printf("no struct name!!!\n");
   }else {
      struct_table->insert({struct_name, struct_sym});
   }   
   for(astree* child: root->children){
      if(child->symbol == TOK_IDENT){
            child->struct_id = child->lexinfo;//not neccessary
      }else{
         symbol* field_sym = new symbol();
         field_sym->sequence = seq_num;
         field_sym->lloc = child->lloc;
         set_attr(field_sym,attr::FIELD);//print type_id before this
         struct_sym->fields->insert({const_cast<string *>(child->lexinfo),field_sym});   
         seq_num++;
      }
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

void process_id(astree* root){
   symbol* id_sym = new symbol();
   id_sym->lloc = root->children[0]->lloc;
   id_sym->block_nr = 0;
   set_attr(id_sym, attr::VARIABLE);//print the type_id before printing these atrribute
   set_attr(id_sym, attr::LVAL);
   ident_table_global->insert({const_cast<string *>(root->children[0]->lexinfo),id_sym});
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


