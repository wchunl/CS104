#include <stdio.h>
#include <stdlib.h>
#include "sym_table.h"
#include "astree.h"
#include "lyutils.h"



// Symbol Tables
symbol_table struct_table;
symbol_table ident_table_global;
symbol_table ident_table_local;


void traverse(FILE* outfile, astree* root, int depth) {
   if (root == NULL) return;
   for (astree* child: root->children) {
      traverse(outfile, child, depth + 1);
   }
   // doNode(outfile, root); // Main node checker


   fprintf (outfile, "%-20s ", parser::get_tname(root->symbol));

   switch (root->symbol) {
      case TOK_STRUCT    :{printf("struct found\n"); break;} // case (4b)
      case TOK_FUNCTION  :{printf("function found\n"); break;} // case (4h)
      case TOK_VARDECL   :{printf("vardecl found\n"); break;} // case (4k)
      case TOK_BLOCK     :{printf("block found\n"); break;} // case (4h)
      case TOK_IDENT     :{printf("ident found\n"); break;} // case (4f)
      default              : break;
   }



   // // Debug printing
   // for (int i = 0; i < depth; i++) fprintf(outfile, "|   ");
   // fprintf (outfile, "%s \"%s\" (%zd.%zd.%zd)\n",
   //          parser::get_tname (root->symbol), root->lexinfo->c_str(),
   //          root->lloc.filenr, root->lloc.linenr, root->lloc.offset);

}

