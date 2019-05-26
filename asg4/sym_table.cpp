#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <bitset>
#include <unordered_map>

#include "sym_table.h"
#include "astree.h"
#include "lyutils.h"

using namespace std;

attr attr_reference[] = { 
   attr::VOID, attr::INT, attr::NULLPTR_T, attr::STRING, attr::STRUCT,
   attr::ARRAY, attr::FUNCTION, attr::VARIABLE, attr::FIELD, 
   attr::TYPEID, attr::PARAM, attr::LOCAL, attr::LVAL, attr::CONST, 
   attr::VREG, attr::VADDR, attr::BITSET_SIZE};

// Symbol Tables
symbol_table *fn_table;
symbol_table *struct_table;
symbol_table *struct_field_table;
symbol_table *ident_table_global;
symbol_table *ident_table_local;

int global_block_count = 0;
int next_block = 0;

// Convert attr to string for printing
const string to_string (attr attribute) {
   static const unordered_map<attr,string> hash {
      {attr::VOID       , "void"       },
      {attr::INT        , "int"        },
      {attr::NULLPTR_T  , "null"       },
      {attr::STRING     , "string"     },
      {attr::STRUCT     , "struct"     },
      {attr::ARRAY      , "array"      },
      {attr::FUNCTION   , "function"   },
      {attr::VARIABLE   , "variable"   },
      {attr::FIELD      , "field"      },
      {attr::TYPEID     , "typeid"     },
      {attr::PARAM      , "param"      },
      {attr::LOCAL      , "local"      },
      {attr::LVAL       , "lval"       },
      {attr::CONST      , "const"      },
      {attr::VREG       , "vreg"       },
      {attr::VADDR      , "vaddr"      },
      {attr::BITSET_SIZE, "bitset_size"},
   };
   auto str = hash.find (attribute);
   if (str == hash.end()) {
      throw invalid_argument (string (__PRETTY_FUNCTION__) + ": "
                              + to_string (unsigned (attribute)));
   }
   return str->second;
}


//////////////////////////////////
////        TRAVERSALS        ////
//////////////////////////////////

// Main traverse function
void traverse(FILE* outfile, astree* root, int depth) {
   if (root == NULL) return;
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

void traverse_struct(astree* root, symbol* struct_sym) {
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
   if (struct_name == nullptr) {
        printf("no struct name!!!\n");
   }else {
      struct_table->insert({struct_name, struct_sym});
   }
}
   
void traverse_function(astree* root, symbol* func_sym) {
   next_block++; // case (3.2b)
   int cur_block = 1;
   // >>create new symbol table for storing vars (3.2c)<<
   func_sym->lloc = root->lloc;
   func_sym->block_nr = global_block_count;
   set_attr(func_sym, attr::FUNCTION);

   for (astree* child: root->children) {
      switch (child->symbol) {
         case TOK_TYPE_ID  :  {print_func(func_sym, child->children[0], child->children[1]); break;}
         case TOK_PARAM    :  {fn_read_param(child, func_sym, cur_block); break;}
         case TOK_BLOCK    :  {traverse_block(child, cur_block); break;} // case (4h)
         default           :  break;
      }
   }
   global_block_count++; // case (4g)
   // >>leaving function so delete the created sym table<<
   // >>need to insert func_sym into function symbol table<<
}

void traverse_block(astree* root, int cur_block) {
   if (root == NULL) return;
   int seq_num = 0;
   for (astree* child: root->children) {
      switch (child->symbol) {
         case TOK_VARDECL     : {fn_read_vardecl(child, cur_block, seq_num++); break;} // print out
         case TOK_BLOCK       : traverse_block(child, cur_block + 1);
         default              : break;
      }
   }
}

void fn_read_vardecl(astree* root, size_t block_nr, int seq_num) {
   symbol* var_sym = new symbol();
   var_sym->lloc = root->lloc;
   var_sym->block_nr = block_nr;
   var_sym->sequence = seq_num;
   set_attr(var_sym, attr::VARIABLE);
   set_attr(var_sym, attr::LVAL);
   set_attr(var_sym, attr::LOCAL);
   print_local_var(var_sym, root->children[0], root->children[1]);
   // type: root->children[0]
   // name: root->children[1] 
   // thing to set to: root->children[2]
   
   // >>need to insert var_sym into local symbol table<<
}

// Reads all parameters of a function
void fn_read_param(astree* root, symbol* func_sym, size_t block_nr) {
   int param_num = 0;
   for (astree* child: root->children) { // does nothing if no param
      if (child->symbol == TOK_TYPE_ID) { // Just in case, dont need this line
         if (func_sym->parameters == nullptr)
            func_sym->parameters = new vector<symbol *>;
         symbol* param_sym = new symbol();
         set_attr(param_sym, attr::VARIABLE);
         set_attr(param_sym, attr::LVAL);
         set_attr(param_sym, attr::PARAM);
         param_sym->lloc = child->lloc;
         param_sym->block_nr = block_nr;
         param_sym->sequence = param_num;
         print_param(param_sym, child->children[0], child->children[1]);
         func_sym->parameters->push_back(param_sym);
         param_num++;
      }
   }
}


//////////////////////////////////
////         PRINTING         ////
//////////////////////////////////

void print_local_var(symbol* sym, astree* type, astree* name) {
   printf("   %s (%zd.%zd.%zd) {%zd} %s", name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str());
   for (long unsigned int i = 0; i < sym->attributes.size(); i++) {
      if (sym->attributes[i] == 1) {
         printf(" %s", to_string(attr_reference[i]).c_str());
      }
   }
   printf("% zd\n", sym->sequence);
}

void print_param(symbol* sym, astree* type, astree* name) {
   printf("   %s (%zd.%zd.%zd) {%zd} %s", name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str());
   for (long unsigned int i = 0; i < sym->attributes.size(); i++) {
      if (sym->attributes[i] == 1) {
         printf(" %s", to_string(attr_reference[i]).c_str());
      }
   }
   printf(" %zd\n", sym->sequence);
}

void print_func(symbol* sym, astree* type, astree* name) {
   printf("\n%s (%zd.%zd.%zd) {%zd} %s", name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str());
   for (long unsigned int i = 0; i < sym->attributes.size(); i++) {
      if (sym->attributes[i] == 1) {
         printf(" %s", to_string(attr_reference[i]).c_str());
      }
   }
   printf("\n");
}



/////////////////////////////////
////          OTHER          ////
/////////////////////////////////

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


