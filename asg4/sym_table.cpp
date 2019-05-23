
#include "sym_table.h"

// symbol_table

// symbol::symbol (astree* n) {

// }



/* void symbol::print (astree* tree) {
   for (int i = 0; i < depth; i++) fprintf(outfile, "|   ");
   printf ( "%s \"%s\" (%zd.%zd.%zd)\n",
            parser::get_tname (tree->symbol), tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset);
   for (astree* child: tree->children) {
      astree::print (outfile, child, depth + 1);
   }
}
 */

void traverse(astree* root) {
   // printf()
}
