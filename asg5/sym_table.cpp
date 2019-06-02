#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <bitset>
#include <unordered_map>
#include <cstring>
#include <string>

#include "sym_table.h"
#include "lyutils.h"
#include "string_set.h"

using namespace std;

 attr attr_reference[] = { 
    attr::VOID, attr::INT, attr::NULLPTR_T, attr::STRING, attr::STRUCT,
    attr::ARRAY, attr::FUNCTION, attr::VARIABLE, attr::FIELD, 
    attr::TYPEID, attr::PARAM, attr::LOCAL, attr::LVAL, attr::CONST, 
    attr::VREG, attr::VADDR, attr::BITSET_SIZE};

// Symbol Tables
symbol_table *fn_table = new symbol_table();
symbol_table *struct_table = new symbol_table();
symbol_table *ident_table_global = new symbol_table();
symbol_table *ident_table_local = new symbol_table();

int global_block_count = 0;
int next_block = 0;

FILE* sym_file;

// Convert string to attr for printing
attr to_attr (const string* s) {
   static const unordered_map<string,attr> hash {
      {"void"       , attr::VOID        },
      {"int"        , attr::INT         },
      {"null"       , attr::NULLPTR_T   },
      {"string"     , attr::STRING      },
      {"struct"     , attr::STRUCT      },
      {"array"      , attr::ARRAY       },
      {"function"   , attr::FUNCTION    },
      {"variable"   , attr::VARIABLE    },
      {"field"      , attr::FIELD       },
      {"typeid"     , attr::TYPEID      },
      {"param"      , attr::PARAM       },
      {"local"      , attr::LOCAL       },
      {"lval"       , attr::LVAL        },
      {"const"      , attr::CONST       },
      {"vreg"       , attr::VREG        },
      {"vaddr"      , attr::VADDR       },
      {"bitset_size", attr::BITSET_SIZE },
   };
   auto str = hash.find(*s);
   if (str == hash.end()) {
      // printf("error in to_attr, ignored\n");
      return attr::NULLPTR_T;
      // exit(-1);
   }
   return str->second;
}


//////////////////////////////////
////        TRAVERSALS        ////
//////////////////////////////////

// Main traverse function
void traverse(FILE* outfile, astree* root, int depth) {
   sym_file = outfile;
   if (root == NULL) return;
   switch (root->symbol) {
      case TOK_STRUCT    :{traverse_struct(root,new symbol()); 
                           break;}
      case TOK_FUNCTION  :{traverse_function(root, new symbol()); 
                           break;}
      case TOK_VARDECL   :{process_id(root); 
                           break;}
      case TOK_IDENT     :{printf("We don't need TOK_IDENT"); 
                           break;}
      default            :{for (astree* child: root->children) 
                              traverse(outfile, child, depth + 1);
                           break;}
   }
   // fprintf (outfile, "%-20s", parser::get_tname(root->symbol));
}

void traverse_struct(astree* root, symbol* struct_sym) {
   size_t seq_num = 0;
   set_attr(struct_sym,attr::STRUCT);//remember to print struct name
   struct_sym->lloc = root->lloc;
   struct_sym->block_nr = 0;
   struct_sym->fields = new symbol_table();
   insert_table_node(root->children[0], struct_sym, struct_table);
   print_struct(struct_sym,root->children[0]);
   for(astree* child: root->children) {
      if(child->symbol == TOK_IDENT){
            child->struct_id = child->lexinfo;//not neccessary
      }else{
         symbol* field_sym = new symbol();
         field_sym->sequence = seq_num;
         field_sym->lloc = child->lloc;
         set_attr(field_sym,attr::FIELD);//print type_id before this
         struct_sym->fields->insert(
            {const_cast<string *>(child->lexinfo),field_sym});   
         print_field(field_sym, child, child->children[0]);
         seq_num++;

      }
   }
   insert_table_node(root->children[0], struct_sym, struct_table);
}
   
void traverse_function(astree* root, symbol* func_sym) {
   next_block++; // case (3.2b)
   int cur_block = 1; // for traversing blocks
   func_sym->lloc = root->lloc;
   func_sym->block_nr = global_block_count;
   set_attr(func_sym, attr::FUNCTION);
   for (astree* child: root->children) {
      switch (child->symbol) {
         case TOK_TYPE_ID  :  {print_func(func_sym, child->children[0],
                                                child->children[1]);
                              break;}
         case TOK_PARAM    :  {fn_read_param(child, func_sym, 
                                                   cur_block);
                              break;}
         case TOK_BLOCK    :  {traverse_block(child, cur_block); 
                              break;}
         default           :  break;
      }
   }
   global_block_count++; // case (4g)
   insert_table_node(root->children[0]->children[1], 
                     func_sym, fn_table);
}

void traverse_block(astree* root, int cur_block) {
   if (root == NULL) return;
   // Entering block scope, create new symbol table (3.2c)
   //symbol_table *ident_table_local = new symbol_table();
   int seq_num = 0;
   for (astree* child: root->children) {
      switch (child->symbol) {
         case TOK_VARDECL     : {fn_read_vardecl(child, cur_block, 
                                    seq_num++, ident_table_local);
                                 break;} // print out
         case TOK_BLOCK       : traverse_block(child, cur_block + 1);
                                break;
         case TOK_CALL        : break;
         case TOK_RETURN      : break;
         default              : {
            if (typecheck(child)) {
               // printf("TYPECHECK = TRUE\n");
            } else {
               // printf("TYPECHECK = FALSE\n");
            };
            break;
         }
      }
   }
   // Leaving block scope, destroy symbol table
   //delete ident_table_local;

}

// Reads local variable declarations inside of a function
void fn_read_vardecl(astree* root, size_t block_nr, int seq_num, 
                                 symbol_table* id_table_local) {
   // Create and set symbol data
   symbol* var_sym = new symbol();
   var_sym->lloc = root->lloc;
   var_sym->block_nr = block_nr;
   var_sym->sequence = seq_num;

   switch (root->children[0]->symbol) {
      case TOK_PTR     : {var_sym->type = attr::STRUCT; break;}
      case TOK_ARRAY   : {var_sym->type = to_attr(
                           root->children[0]->children[0]->lexinfo); 
                           break;}
      default: var_sym->type = to_attr(root->children[0]->lexinfo);
   }
   set_attr(var_sym, attr::VARIABLE);
   set_attr(var_sym, attr::LVAL);
   set_attr(var_sym, attr::LOCAL);
   // Insert into local symbol table
   insert_table_node(root->children[1], var_sym, id_table_local);
   print_local_ident(var_sym, root->children[0], root->children[1]);

   // for typechecking part: 
   // type: root->children[0]
   // type to set: root->children[2]
}

// Reads all parameters of a function
void fn_read_param(astree* root, symbol* func_sym, size_t block_nr) {
   int param_num = 0;
   for (astree* child: root->children) { // does nothing if no param
      if (child->symbol == TOK_TYPE_ID) { // Just in case
         if (func_sym->parameters == nullptr)
            func_sym->parameters = new vector<symbol *>;
         // Create and set symbol data
         symbol* param_sym = new symbol();
         param_sym->lloc = child->lloc;
         param_sym->block_nr = block_nr;
         param_sym->sequence = param_num;
         param_sym->type = to_attr(child->children[0]->lexinfo);
         set_attr(param_sym, attr::VARIABLE);
         set_attr(param_sym, attr::LVAL);
         set_attr(param_sym, attr::PARAM);
         print_local_ident(param_sym, child->children[0], 
                                       child->children[1]);
         func_sym->parameters->push_back(param_sym);
         // temporary next line
         insert_table_node(child->children[1],param_sym, 
                                       ident_table_local);
         param_num++;
      }
   }
}

/////////////////////////////////
////      TYPE CHECKING      ////
/////////////////////////////////

bool typecheck(astree* root) {
   // The passed root is one of the following:
   switch (root->symbol) {
      case TOK_WHILE     : // fallthrough
      case TOK_IFELSE    : // fallthrough
      case TOK_IF        : {
         if (root->children[2] != nullptr) {
            return (typecheck(root->children[0]) 
               && typecheck(root->children[2]));
         }
      };
      case '='           : break;
      case '+'           : // fallthrough
      case '-'           : // fallthrough
      case '/'           : // fallthrough
      case '*'           : // fallthrough
      case '%'           : // fallthrough
      case '<'           : // fallthrough
      case TOK_LE        : // fallthrough
      case '>'           : // fallthrough
      case TOK_GE        : return tc_bin_op(root);
      case TOK_EQ        : // fallthrough;
      case TOK_NE        : return tc_bool_op(root);
      case TOK_NOT       : {if(find_type(root->children[0])==attr::INT)
                              return true;
                           else 
                              return false;}
      case TOK_ALLOC     : break;
      case TOK_CALL      : break;
      case TOK_RETURN    : break;
      case ';'           : return true;
      default            : return false;
   }
   return false;
}

// Binary operators
bool tc_bin_op(astree* root) {
   if (root->children.size() < 2) return false;
   astree* left = root->children[0];
   astree* right = root->children[1];

   // printf(">>type check: %s  {%s}  %s\n", 
      // left->lexinfo->c_str(), root->lexinfo->c_str(), 
      // right->lexinfo->c_str());

   if(find_type(left) == find_type(right)) {
      if (find_type(left) == attr::INT) {
         root->type = to_string(find_type(left)).c_str();
         // printf("%s   | finaltype: %s\n", 
         //    root->lexinfo->c_str(), root->type);
         return true;
      }
   }

   // printf("\nerror in tc_bin_op\n");
   return false;
   // exit(-1);
}

bool tc_bool_op(astree* root) {
   if (root->children.size() < 2) return false;
   astree* left = root->children[0];
   astree* right = root->children[1];

   // printf(">>type check: %s  {%s}  %s\n", 
   //    left->lexinfo->c_str(), root->lexinfo->c_str(), 
   //    right->lexinfo->c_str());

   // Check if types are same
   if(find_type(left) == find_type(right)) {
      root->type = to_string(find_type(left)).c_str();
      // printf("%s   | finaltype: %s\n", 
      //    root->lexinfo->c_str(), root->type);
      return true;
   }

   //Case (2.4a), assume the type
   if(find_type(left) == attr::NULLPTR_T 
      && find_type(right) != attr::NULLPTR_T) {
         root->type = to_string(find_type(right)).c_str();
         // printf("%s   | finaltype: %s\n", 
         //    root->lexinfo->c_str(), root->type);
         return true;
   } else if(find_type(right) == attr::NULLPTR_T 
      && find_type(left) != attr::NULLPTR_T) {
         root->type = to_string(find_type(left)).c_str();
         // printf("%s   | finaltype: %s\n", 
         //    root->lexinfo->c_str(), root->type);
         return true;
   }

   // printf("\nerror in tc_bool_op\n");
   return false;
   // exit(-1);
}

attr find_type(astree* root) {
      switch(root->symbol) {
         case TOK_IDENT       : {return get_type(root->lexinfo);} 
         case TOK_INTCON      : return attr::INT;
         case TOK_CHARCON     : return attr::INT;
         case TOK_STRINGCON   : return attr::STRING;
         case TOK_NULLPTR     : return attr::NULLPTR_T;
         case TOK_ARROW       : return find_type(root->children[1]);
         case TOK_INDEX       : return find_type(root->children[0]);
         default              : typecheck(root);
   }


   if (root->type != NULL) return to_attr(
      string_set::intern (root->type));

   // return to_attr(string(root->type));
   // printf("error in find_type");
   // exit(-1);
   return attr::NULLPTR_T;
}


attr get_type(const string* key) {
   // Seach local table first
   auto i = ident_table_local->find(const_cast<string *>(key));
   if (i != ident_table_local->end()) {
      return i->second->type;
   }

   // Search global table next
   auto j = ident_table_global->find(const_cast<string *>(key));
   if (j != ident_table_global->end()) {
      return j->second->type;
   }

   // printf("error in get_type");
   return attr::NULLPTR_T;
   // exit(-1);
}

location get_lloc(const string* key) {
   // Seach local table first
   auto i = ident_table_local->find(const_cast<string *>(key));
   if (i != ident_table_local->end()) {
      return i->second->lloc;
   }

   // Search global table next
   auto j = ident_table_global->find(const_cast<string *>(key));
   if (j != ident_table_global->end()) {
      return j->second->lloc;
   }

   // printf("error in get_type");
   location* null_lloc = new location();
   null_lloc->filenr = null_lloc->linenr = null_lloc->offset = -1;
   return *null_lloc;
   // exit(-1);
}


//////////////////////////////////
////         PRINTING         ////
//////////////////////////////////

// Print function header
void print_func(symbol* sym, astree* type, astree* name) {
   string ptr = "ptr";
   if (global_block_count != 0) fprintf(sym_file, "\n");
   if(strcmp(type->lexinfo->c_str(),ptr.c_str())==0){
      fprintf(sym_file, "%s (%zd.%zd.%zd) {%zd} %s <struct %s>", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str(),
         type->children[0]->lexinfo->c_str());
   }else{
      fprintf(sym_file, "%s (%zd.%zd.%zd) {%zd} %s", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str());
   }
   for (long unsigned int i = 0; i < sym->attributes.size(); i++) {
      if (sym->attributes[i] == 1) {
         fprintf(sym_file, " %s",to_string(attr_reference[i]).c_str());
      }
   }
   fprintf(sym_file, "\n");
}

// Print local identifier 3 spaces indented
void print_local_ident(symbol* sym, astree* type, astree* name) {
   string ptr = "ptr";
   string array = "array";
   if(strcmp(type->lexinfo->c_str(),ptr.c_str())==0){
      fprintf(sym_file, "   %s (%zd.%zd.%zd) {%zd} %s <struct %s>", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str(),
         type->children[0]->lexinfo->c_str());
   }else if(strcmp(type->lexinfo->c_str(),array.c_str())==0){
      fprintf(sym_file, "   %s (%zd.%zd.%zd) {%zd} %s <%s>", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str(),
         type->children[0]->lexinfo->c_str());
   }else{
      fprintf(sym_file, "   %s (%zd.%zd.%zd) {%zd} %s", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         sym->block_nr, type->lexinfo->c_str());
   }
   for (long unsigned int i = 0; i < sym->attributes.size(); i++) {
      if (sym->attributes[i] == 1) {
         fprintf(sym_file, " %s",to_string(attr_reference[i]).c_str());
      }
   }
   fprintf(sym_file, " %zd\n", sym->sequence);
}

void print_struct(symbol* sym, astree* name){
   fprintf(sym_file, "\n%s (%zd.%zd.%zd) {%zd} struct %s\n",
            name->lexinfo->c_str(),
            sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
            sym->block_nr, name->lexinfo->c_str());
}

void print_field(symbol* sym, astree* type, astree* name){
   string ptr = "ptr";
   string array = "array";
   if(strcmp(type->lexinfo->c_str(),ptr.c_str())==0){
       fprintf(sym_file, "   %s (%zd.%zd.%zd) %s <struct %s>", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         type->lexinfo->c_str(),type->children[0]->lexinfo->c_str());
   }else if(strcmp(type->lexinfo->c_str(),array.c_str())==0){
      fprintf(sym_file, "   %s (%zd.%zd.%zd) %s <%s>", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         type->lexinfo->c_str(),type->children[0]->lexinfo->c_str());
         
   }else{
      fprintf(sym_file, "   %s (%zd.%zd.%zd)  %s", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         type->lexinfo->c_str());
   }
   for (long unsigned int i = 0; i < sym->attributes.size(); i++) {
      if (sym->attributes[i] == 1) {
         fprintf(sym_file, " %s",to_string(attr_reference[i]).c_str());
      }
   }
   fprintf(sym_file, " %zd\n", sym->sequence);
}

void print_globalid(symbol* sym, astree* type, astree* name) {
   string ptr = "ptr";
   string array = "array";
   if(strcmp(type->lexinfo->c_str(),ptr.c_str())==0){
      fprintf(sym_file, "\n%s (%zd.%zd.%zd)  %s <struct %s>", 
         name->lexinfo->c_str(),
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         type->lexinfo->c_str(),type->children[0]->lexinfo->c_str());
   }else if(strcmp(type->lexinfo->c_str(),array.c_str())==0){
      fprintf(sym_file, "\n%s (%zd.%zd.%zd) %s <%s>", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset, 
         type->lexinfo->c_str(),type->children[0]->lexinfo->c_str());
         
   }else{
      fprintf(sym_file, "\n%s (%zd.%zd.%zd)  %s", 
         name->lexinfo->c_str(), 
         sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset,
         type->lexinfo->c_str());
   }
   for (long unsigned int i = 0; i < sym->attributes.size(); i++) {
      if (sym->attributes[i] == 1) {
         fprintf(sym_file, " %s",to_string(attr_reference[i]).c_str());
      }
   }
   fprintf(sym_file, " %zd\n", sym->sequence);
}

/////////////////////////////////
////          OTHER          ////
/////////////////////////////////

// Sets attribute of given symbol
void set_attr(symbol* sym, attr a1) {
   sym->attributes.set(unsigned(a1));
}

// Inserts the node containing the name and symbol into st
void insert_table_node(astree* name, symbol* sym, symbol_table* st) {
   if (name == nullptr) printf("No name found!");
   else st->insert({const_cast<string *>(name->lexinfo),sym});
}

void process_id(astree* root){
   size_t seq_num = 0;
   symbol* id_sym = new symbol();
   id_sym->lloc = root->children[0]->lloc;
   id_sym->block_nr = 0;
   set_attr(id_sym, attr::VARIABLE);//print the type_id
   set_attr(id_sym, attr::LVAL);
   ident_table_global->insert(
      {const_cast<string *>(root->children[0]->lexinfo),id_sym});
   print_globalid(id_sym,root->children[0], root->children[1]);
   seq_num++;
}

void dump_tables() {
   printf("\n\n---- Printing struct_table ----\n\n");
   for (auto const& pair: (*struct_table)) {
      printf("name: %-10s | lloc: (%zd.%zd.%zd)", pair.first->c_str(),
         pair.second->lloc.filenr, pair.second->lloc.linenr, 
         pair.second->lloc.offset);
      for (long unsigned int i=0; i<pair.second->attributes.size();i++)
         if (pair.second->attributes[i] == 1) 
            printf(" %s", to_string(attr_reference[i]).c_str());
      printf("\n");
   }
   printf("\n\n---- Printing fn_table ----\n\n");
   for (auto const& pair: (*fn_table)) {
      printf("name: %-10s | lloc: (%zd.%zd.%zd)", pair.first->c_str(),
         pair.second->lloc.filenr, pair.second->lloc.linenr, 
         pair.second->lloc.offset);
      for (long unsigned int i=0; i<pair.second->attributes.size();i++)
         if (pair.second->attributes[i] == 1) 
            printf(" %s", to_string(attr_reference[i]).c_str());
      printf("\n");
   }
   printf("\n\n---- Printing ident_table_global ----\n\n");
   for (auto const& pair: (*ident_table_global)) {
      printf("name: %-10s | lloc: (%zd.%zd.%zd)", pair.first->c_str(),
         pair.second->lloc.filenr, pair.second->lloc.linenr, 
         pair.second->lloc.offset);
      for (long unsigned int i=0; i<pair.second->attributes.size();i++)
         if (pair.second->attributes[i] == 1) 
            printf(" %s", to_string(attr_reference[i]).c_str());
      printf("\n");
   }   printf("\n\n---- Printing ident_table_local ----\n\n");
   for (auto const& pair: (*ident_table_local)) {
      printf("name: %-10s | lloc: (%zd.%zd.%zd)", pair.first->c_str(),
         pair.second->lloc.filenr, pair.second->lloc.linenr, 
         pair.second->lloc.offset);
      for (long unsigned int i=0; i<pair.second->attributes.size();i++)
         if (pair.second->attributes[i] == 1) 
            printf(" %s", to_string(attr_reference[i]).c_str());
      printf(" | type: %s", to_string(pair.second->type).c_str());
      printf("\n");
   }
}
