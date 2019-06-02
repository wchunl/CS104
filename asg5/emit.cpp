#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "emit.h"
#include "string_set.h"
#include "lyutils.h"



using namespace std;

FILE* emit_file;

void emit(FILE* outfile, astree* root, int depth){
    emit_file = outfile;
    if (root== NULL) return;
    switch(root->symbol){
        case TOK_STRUCT: 
          //  printf("I'm TOK_STRUCT \n");
            emit_struct(emit_file,root);
            break;
        case TOK_FUNCTION: 
         //   printf("I'm TOK_FUNC \n" );
            emit_function(emit_file,root);
            break;
        case TOK_VARDECL:
            printf("I'm TOK_VARDECL \n");
            break;
        default : 
            for (astree* child: root->children) 
                emit(outfile, child, depth + 1);
            break;
    }
}

// still need one case for type struct IDENT
void emit_struct(FILE* outfile,astree* root){
    for(auto *field: root->children){
        switch(field->symbol){
            case TOK_IDENT:
                printf(".struct: %s\n",field->lexinfo->c_str());
                break;
            case TOK_INT:
                printf(".field: int %s\n",field->children[0]->lexinfo->c_str());
                break;
            default:
                printf(".field: ptr %s\n",field->children[0]->lexinfo->c_str()); 
                break;     
        }
    }
    printf(".end\n");
}

void emit_function(FILE* outfile,astree* root){
    for(auto *child: root->children){
        switch(child->symbol){
            case TOK_BLOCK:break;
            case TOK_PARAM:
                emit_param(outfile,child);
                break;
            case TOK_TYPE_ID:
                printf("%s:%15s %s\n",child->children[1]->lexinfo->c_str(),
                    ".funcion", child->children[0]->lexinfo->c_str());
                break;
        }
    }
}

//not done because of struct IDENT
void emit_param(FILE* outfile,astree* root){
    for(auto *child: root->children){
        if(child->children[0]->symbol == TOK_INT){
            printf("%15s %s %s\n",".param",child->children[0]->lexinfo->c_str()
                ,child->children[1]->lexinfo->c_str());
        }else{
             printf("%15s %s %s\n",".param","ptr"
                ,child->children[1]->lexinfo->c_str());
        }
    }
}