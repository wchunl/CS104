#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string.h>

#include "emit.h"
#include "string_set.h"
#include "lyutils.h"



using namespace std;

FILE* emit_file;
int while_nr = 0;
int if_nr = 0;
int else_nr = 0;
int register_nr = 0;

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
                printf("%-9s%3s\n",".struct:",field->lexinfo->c_str());
                break;
            case TOK_INT:
                printf("%-9s%3s %s\n",".field:","int",
                    field->children[0]->lexinfo->c_str());
                break;
            default:
                printf("%-9s%3s %s\n",".field:","ptr",
                    field->children[0]->lexinfo->c_str()); 
                break;     
        }
    }
    printf("%15s\n",".end");
}
//not done with struct IDENT
void emit_function(FILE* outfile,astree* root){
    for(auto* child: root->children){
        switch(child->symbol){
            case TOK_BLOCK:
                emit_block(outfile,child);
                break;
            case TOK_PARAM:
                emit_param(outfile,child);
                break;
            case TOK_TYPE_ID:
                if(child->children[0]->symbol == TOK_STRING){
                    printf("%-9s%5s %s\n",strcat(const_cast<char*>
                    (child->children[1]->lexinfo->c_str()),":"),
                    ".funcion", "ptr");
                }else{
                    printf("%-9s%5s %s\n",
                    strcat(const_cast<char*>(
                    child->children[1]->lexinfo->c_str()),":"),
                    ".funcion","int");
                }
                break;
        }
    }
     printf("%15s\n",".end");
}

//not done because of struct IDENT
void emit_param(FILE* outfile,astree* root){
    for(auto* child: root->children){
        if(child->children[0]->symbol == TOK_INT){
            printf("%15s %s %s\n",".param"
                ,"int",child->children[1]->lexinfo->c_str());
        }else{
             printf("%15s %s %s\n",".param","ptr"
                ,child->children[1]->lexinfo->c_str());
        }
    }
}

void emit_block(FILE* outfile,astree* root){
    for(auto* child: root->children){
        switch(child->symbol){
            case TOK_VARDECL:
                if(child->children[0]->symbol == TOK_INT){
                    printf("%15s %s %s\n", ".local","int",child->children[1]->lexinfo->c_str());
                }else{
                   printf("%15s %s %s\n", ".local","ptr",child->children[1]->lexinfo->c_str());  
                }
                break;
            case TOK_RETURN: 
                printf("%15s %s\n", "return",child->children[0]->lexinfo->c_str());
                printf("%15s\n", "return");
                break;
            case TOK_CALL: 

                break;
            case TOK_IF: break;
            case TOK_IFELSE: break;
            case '=' : break;
            case TOK_WHILE: 
                emit_while(outfile, child);
                while_nr++;
                break;
        }
    }
}

void emit_while(FILE* outfile,astree* root){
    //while statement
    if(root->children[0]->children[0]->symbol == TOK_INDEX){
        // when there's array elments in while using alloc???
        // printf("%s%d:%14s%d:%s = %s %s %s\n",".wh",while_nr,"$t",
        // register_nr,root->children[0]->children[0]->lexinfo->c_str(),
        // root->children[0]->children[0]->lexinfo->c_str(),
        // root->children[0]->lexinfo->c_str(),
        // root->children[0]->children[1]->lexinfo->c_str());
    }else{
        printf("%s%d:%9s%d:%s = %s %s %s\n",".wh",while_nr,"$t",
            register_nr,root->children[0]->children[0]->lexinfo->c_str(),
            root->children[0]->children[0]->lexinfo->c_str(),
            root->children[0]->lexinfo->c_str(),
            root->children[0]->children[1]->lexinfo->c_str());
    }  
    printf("%15s %s%d if not $t%d:%s\n","goto",".od",while_nr,register_nr,
        root->children[0]->children[0]->lexinfo->c_str());
    register_nr++;

    //create do, call emit_block
    if(root->children[1]->symbol == TOK_BLOCK){
        printf("%s%d:\n",".do",while_nr);
        emit_block(outfile,root);
    }

}
