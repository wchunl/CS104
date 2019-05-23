#include <stdio.h>
#include <stdlib.h>
#include "sym_table.h"
#include "astree.h"
#include "lyutils.h"

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
    if(root == NULL){
        return;
    }
   for (astree* child: root->children) {
       traverse(child);
   }
   printf("|   ");
   printf ( "%s \"%s\" (%zd.%zd.%zd)\n",
            parser::get_tname (root->symbol), root->lexinfo->c_str(),
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);

}

