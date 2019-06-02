// $Id: astree.cpp,v 1.17 2019-03-15 14:32:40-07 - - $
// Wai Chun Leung
// wleung11
// Shineng Tang
// stang38

#include <iostream>
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "sym_table.h"
#include "string_set.h"
#include "lyutils.h"


attr attr_references[] = { 
   attr::VOID, attr::INT, attr::NULLPTR_T, attr::STRING, attr::STRUCT,
   attr::ARRAY, attr::FUNCTION, attr::VARIABLE, attr::FIELD, 
   attr::TYPEID, attr::PARAM, attr::LOCAL, attr::LVAL, attr::CONST, 
   attr::VREG, attr::VADDR, attr::BITSET_SIZE};

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

astree::astree (int symbol_, const location& lloc_, const char* info) {
   symbol = symbol_;
   lloc = lloc_;
   lexinfo = string_set::intern (info);
   struct_id = nullptr;
   // vector defaults to empty -- no children
}

astree::~astree() {
   while (not children.empty()) {
      astree* child = children.back();
      children.pop_back();
      delete child;
   }
   if (yydebug) {
      fprintf (stderr, "Deleting astree (");
      astree::dump (stderr, this);
      fprintf (stderr, ")\n");
   }
}

astree* astree::adopt (astree* child1, astree* child2, astree* child3) {
   if (child1 != nullptr) children.push_back (child1);
   if (child2 != nullptr) children.push_back (child2);
   if (child3 != nullptr) children.push_back (child3);
   return this;
}

astree* astree::adopt_sym (astree* child, int symbol_) {
   symbol = symbol_;
   return adopt (child);
}


astree* astree::change_sym(int symbol_) {
   symbol=symbol_;
   return this;
}


void astree::dump_node (FILE* outfile) {
   fprintf (outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
            static_cast<const void*> (this),
            parser::get_tname (symbol),
            lloc.filenr, lloc.linenr, lloc.offset,
            lexinfo->c_str());
   for (size_t child = 0; child < children.size(); ++child) {
      fprintf (outfile, " %p",
               static_cast<const void*> (children.at(child)));
   }
}

void astree::dump_tree (FILE* outfile, int depth) {
   fprintf (outfile, "%*s", depth * 3, "");
   dump_node (outfile);
   fprintf (outfile, "\n");
   for (astree* child: children) child->dump_tree (outfile, depth + 1);
   fflush (nullptr);
}

void astree::dump (FILE* outfile, astree* tree) {
   if (tree == nullptr) fprintf (outfile, "nullptr");
                   else tree->dump_node (outfile);
}

void astree::print (FILE* outfile, astree* tree, int depth) {
   for (int i = 0; i < depth; i++) fprintf(outfile, "|   ");
   fprintf (outfile, "%s \"%s\" (%zd.%zd.%zd)",
            parser::get_tname (tree->symbol), tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset);
   // Added this
   set_attribute(tree);

   // If tree is an expression...
   if(is_expr(tree)) {
      if (tree->type != NULL) fprintf(outfile, " %s", tree->type);
   }

   // Print the type, if any
   if(strcmp(parser::get_tname(tree->symbol), "TOK_IDENT") == 0) {
      if (get_type(tree->lexinfo) != attr::NULLPTR_T) {
         fprintf(outfile, " %s", to_string(
            get_type(tree->lexinfo)).c_str());
      }
   }

   // Print the attributes
   for (long unsigned int i = 0; i < tree->attributes.size(); i++) {
      if (tree->attributes[i] == 1) {
         fprintf(outfile," %s", to_string(attr_references[i]).c_str());
      }
   }

   // Print the declaration location
   if(strcmp(parser::get_tname(tree->symbol), "TOK_IDENT") == 0) {
      if (get_lloc(tree->lexinfo).filenr != static_cast<size_t>(-1)) {
            fprintf(outfile, " (%zd.%zd.%zd)", 
            get_lloc(tree->lexinfo).filenr,
            get_lloc(tree->lexinfo).linenr,
            get_lloc(tree->lexinfo).offset);
      }
   }

   fprintf(outfile,"\n");

   // Recurse on all children
   for (astree* child: tree->children) {
      astree::print (outfile, child, depth + 1);
   }
}

void destroy (astree* tree1, astree* tree2,astree* tree3,
              astree* tree4,astree* tree5) {
   if (tree1 != nullptr) delete tree1;
   if (tree2 != nullptr) delete tree2;
   if (tree2 != nullptr) delete tree3;
   if (tree2 != nullptr) delete tree4;
   if (tree2 != nullptr) delete tree5;
}

void errllocprintf (const location& lloc, const char* format,
                    const char* arg) {
   static char buffer[0x1000];
   assert (sizeof buffer > strlen (format) + strlen (arg));
   snprintf (buffer, sizeof buffer, format, arg);
   errprintf ("%s:%zd.%zd: %s", 
              lexer::filename (lloc.filenr), lloc.linenr, lloc.offset,
              buffer);
}

bool is_expr(astree* root) {
   switch(root->symbol) {
      case TOK_WHILE     : // fallthrough 
      case TOK_IF        : // fallthrough
      case '='           : // fallthrough 
      case '+'           : // fallthrough 
      case '-'           : // fallthrough 
      case '/'           : // fallthrough 
      case '*'           : // fallthrough 
      case '%'           : // fallthrough 
      case '<'           : // fallthrough 
      case TOK_LE        : // fallthrough 
      case '>'           : // fallthrough 
      case TOK_GE        : // fallthrough
      case TOK_EQ        : // fallthrough
      case TOK_NE        : // fallthrough
      case TOK_NOT       : return true;
      default            : return false;
   }
}

//set attribute of a given root
void set_attribute(astree* root){
   int sym = root->symbol;
   switch(sym){
       case TOK_VOID:
         root->attributes.set(unsigned(attr::VOID));
         break;
       case TOK_INT:
         root->attributes.set(unsigned(attr::INT));
         break;
       case TOK_STRING:
         root->attributes.set(unsigned(attr::STRING));
         break;
       case TOK_ARRAY:
           root->attributes.set(unsigned(attr::ARRAY));
           break;
       case TOK_FUNCTION:
         root->attributes.set(unsigned(attr::FUNCTION));
         break;
       // case TOK_FIELD:
       //   root->attributes.set(unsigned(attr::FIELD));
       //   break;
       case TOK_TYPE_ID:
         root->attributes.set(unsigned(attr::TYPEID));
         break;
       case TOK_VARDECL:
         root->attributes.set(unsigned(attr::VARIABLE));
         root->attributes.set(unsigned(attr::LVAL));
         break;
       case TOK_PARAM:
         root->attributes.set(unsigned(attr::PARAM));
         break;
       case TOK_IDENT:
         root->attributes.set(unsigned(attr::VARIABLE));
         break;
       case '+': case '-': case '*': case '/': case '%': case '=': 
         root->attributes.set(unsigned(attr::INT));
         root->attributes.set(unsigned(attr::VREG));
         break;
       case TOK_INTCON:
         root->attributes.set(unsigned(attr::INT));
         root->attributes.set(unsigned(attr::CONST));
         break;
      case TOK_CHARCON:
         root->attributes.set(unsigned(attr::INT));
         root->attributes.set(unsigned(attr::CONST));
         break;
       case TOK_STRINGCON:
         root->attributes.set(unsigned(attr::STRING));
         root->attributes.set(unsigned(attr::CONST));
         break;
       case TOK_NULLPTR:
         root->attributes.set(unsigned(attr::NULLPTR_T));
         root->attributes.set(unsigned(attr::CONST));
         break;
       case TOK_GE: case TOK_LT: case TOK_GT: case TOK_LE:case TOK_EQ:
         root->attributes.set(unsigned(attr::VREG));
         break;
       case TOK_INDEX:
         root->attributes.set(unsigned(attr::LVAL));
         root->attributes.set(unsigned(attr::VADDR));
         break;
       case TOK_ARROW:
         root->attributes.set(unsigned(attr::LVAL));
         root->attributes.set(unsigned(attr::VADDR));
         break;
  }
}
