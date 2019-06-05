#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string.h>
#include <unordered_map>
#include <iostream>

#include "emit.h"
#include "string_set.h"
#include "lyutils.h"
#include "sym_table.h"


using namespace std;

// unordered_map<const char*, int> regis;

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
            emit_struct(root);
            break;
        case TOK_FUNCTION: 
         //   printf("I'm TOK_FUNC \n" );
            emit_function(root);
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
void emit_struct(astree* root){
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
    printf("\t  %s\n",".end");
}
//not done with struct IDENT
void emit_function(astree* root){
    for(auto* child: root->children){
        switch(child->symbol){
            case TOK_TYPE_ID:
                if(child->children[0]->symbol == TOK_STRING){
                    printf("%s:\t  .function ptr\n",
                        const_cast<char*>(
                        child->children[1]->lexinfo->c_str()));
                }else{
                    printf("%s:\t  .function int\n",
                        const_cast<char*>(
                        child->children[1]->lexinfo->c_str()));
                }
                break;
            case TOK_PARAM:
                emit_param(child);
                break;
            case TOK_BLOCK:
                emit_local_vars(child);
                emit_block(child);
                break;
        }
    }
     printf("\t  %s\n",".end");
}

// Need to emit the var_decl inside the block node in function first,
// then we can parse the actual expression to see if it is simple or not
void emit_local_vars(astree* root) {
    for(auto* child: root->children) {
        if (child->symbol == TOK_VARDECL) {
            // Print identifier and type
            if(child->children[0]->symbol == TOK_INT){
                printf("\t  .local int %s\n", child->children[1]->lexinfo->c_str());
            }else{
                printf("\t  .local ptr %s\n", child->children[1]->lexinfo->c_str());  
            }
        }
    }
}

//not done because of struct IDENT
void emit_param(astree* root){
    for(auto* child: root->children){
        if(child->children[0]->symbol == TOK_INT){
            printf("\t  .param int %s\n",
                child->children[1]->lexinfo->c_str());
        }else{
             printf("\t  .param ptr %s\n",
                child->children[1]->lexinfo->c_str());
        }
    }
}

// TOK_VARDECL and expressions checked in emit_expr()
void emit_block(astree* root){
    for(auto* child: root->children){
        switch(child->symbol){
            case TOK_RETURN : 
                printf("\t  return %s\n",child->children[0]->lexinfo->c_str());
                printf("\t  return \n");
                break;
            case TOK_CALL   : emit_call(child); break;
            case TOK_IF     : //fallthrough
            case TOK_IFELSE : emit_if(child); break;
            case TOK_WHILE  : 
                emit_while( child);
                while_nr++;
                break;
            default: emit_expr(child);
        }
    }
}

void emit_call(astree* root) {
    // If call has args
    if (root->children.size() > 1) {
        // If call args are not simple
        if(root->children[1]->children.size() > 0) {
            emit_expr(root->children[1]);
            printf("\t  %s($t%d:i)\n", 
                root->children[0]->lexinfo->c_str(),
                register_nr++);
        } else { // If is simple
            printf("\t  %s(%s)\n", 
                root->children[0]->lexinfo->c_str(),
                root->children[1]->lexinfo->c_str());
        }
    } else { // If no args provided
        printf("\t  %s()\n", 
            root->children[0]->lexinfo->c_str());
    }
}

void emit_if(astree* root) {
    cout << "unimpl if with lexinfo: " << root->lexinfo << endl;
}

void emit_while(astree* root){
    //while statement
    if(root->children[0]->children[0]->symbol == TOK_INDEX){
        // when there's array elments in while using alloc???
        // printf("%s%d:%14s%d:%s = %s %s %s\n",".wh",while_nr,"$t",
        // register_nr,root->children[0]->children[0]->lexinfo->c_str(),
        // root->children[0]->children[0]->lexinfo->c_str(),
        // root->children[0]->lexinfo->c_str(),
        // root->children[0]->children[1]->lexinfo->c_str());
    }else{
        printf(".wh%d:", while_nr);
        emit_expr(root->children[0]);
    }
    printf("\t  goto .od%d if not $t%d:i\n",
        while_nr, register_nr);

    //create do, call emit_block
    if(root->children[1]->symbol == TOK_BLOCK){
        printf("%s%d:",".do",while_nr);
        emit_block(root->children[1]);
    }

    printf(".od%d:", while_nr);
}


///////////////////////////////////
////        EXPRESSIONS        ////
///////////////////////////////////

// Read a node if it is an expression
void emit_expr(astree* root){
    switch(root->symbol) {
        case '+'            : // fallthrough
        case '-'            : // fallthrough
        case '/'            : // fallthrough
        case '*'            : // fallthrough
        case '%'            : // fallthrough
        case '<'            : // fallthrough
        case TOK_LE         : // fallthrough
        case '>'            : // fallthrough
        case TOK_GE         : emit_bin_expr(root);    break;
        case TOK_NOT        : emit_unary_expr(root);  break;
        case TOK_VARDECL    : emit_asg_expr(root, 1); break;
        case '='            : emit_asg_expr(root, 0); break;
        case TOK_IDENT      : // fallthrough
        case TOK_INTCON     : // fallthrough
        case TOK_STRINGCON  : // fallthrough
        case TOK_NULLPTR    :
            printf("%s", root->lexinfo->c_str()); 
            break;
        case TOK_ARROW      : break; //unimpl, refer to 6.2
        case TOK_CALL       : break; //unimpl, refer to 6.3 2nd paragraph
    }
}

// Generate code for an assignment expression ("=" or vardecl)
// start is the number of the node to start from
// start for vardecl is 1 and for "=" it is 0
void emit_asg_expr(astree*root, int start) {
    // If node is not nested
    if(root->children[start+1]->children.size() == 0) {
        printf("\t  %s = %s\n",
            root->children[start]->lexinfo->c_str(),
            root->children[start+1]->lexinfo->c_str());
    } else { // If node is nested, traverse it
        emit_expr(root->children[start+1]);
        printf("\t  %s = $t%d:i\n", 
            root->children[start]->lexinfo->c_str(), 
            register_nr++);
        // Increment register number
    }
}

// Generate binary expression code
void emit_bin_expr(astree* root) {

    // If reached deepest node
    if(root->children[0]->children.size() == 0) {
        printf("\t  $t%d:i = ", register_nr);
        printf("%s %s %s\n", 
            root->children[0]->lexinfo->c_str(),
            root->lexinfo->c_str(),
            root->children[1]->lexinfo->c_str());
    } else { // Else keep traversing
        emit_expr(root->children[0]);
    }

}

// Generate unary expression code
void emit_unary_expr(astree* root) {
    cout << "unimpl emit_unary_expr(), symbol lex: " << root->lexinfo->c_str() << endl;
}