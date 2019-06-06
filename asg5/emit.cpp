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

FILE* emit_file;
int while_nr = -1;
int if_nr = -1;
int register_nr = 0;
int string_nr = 0;

// Whether or not a label was already printed previously
bool lbl = false;

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
        //    printf("I'm TOK_VARDECL \n");
            emit_global_vars(root);
            break;
        default : 
            for (astree* child: root->children) 
                emit(outfile, child, depth + 1);
            break;
    }
}

// still need one case for type struct IDENT
void emit_struct(astree* root){
    for(auto* field: root->children){
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
                if(child->children[0]->symbol == TOK_VOID){
                    printf("%s:\t  .function\n",
                        const_cast<char*>(
                        child->children[1]->lexinfo->c_str()));
                }else if(child->children[0]->symbol == TOK_INT){
                    printf("%s:\t  .function int\n",
                        const_cast<char*>(
                        child->children[1]->lexinfo->c_str()));
                }else{
                     printf("%s:\t  .function\n",
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
                printf("\t  .local int %s\n", 
                    child->children[1]->lexinfo->c_str());
            }else{
                printf("\t  .local ptr %s\n", 
                    child->children[1]->lexinfo->c_str());  
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
    // // Need to check if the first node in this block
    // // generates a label. If it does, print newline
    // if (root->children.size() > 0) {
    //     switch(root->children[0]->symbol) {
    //         case TOK_IF     : 
    //         case TOK_WHILE  : printf("\n");
    //     }
    // }

    for(auto* child: root->children){
        switch(child->symbol){
            case TOK_RETURN : 
                printf("\t  return %s\n",
                child->children[0]->lexinfo->c_str());
                printf("\t  return \n"); lbl = false;
                break;
            case TOK_CALL   : emit_call(child); break;
            case TOK_IFELSE : // fallthrough
            case TOK_IF     : if_nr++; emit_if(child); break;
            case TOK_WHILE  : while_nr++; emit_while(child); break;
            case TOK_BLOCK  : emit_block(child); break;
            default: emit_expr(child);
        }
    }
}

void emit_call(astree* root) {
    // If call has args
    if (root->children.size() > 1) {
        // If call args are not simple
        if(root->children[1]->children.size() > 0) {
            // If multi args?
            emit_expr(root->children[1]);
            printf("\t  %s($t%d:i)\n", 
                root->children[0]->lexinfo->c_str(),
                register_nr++); lbl = false;
        } else { // If is simple
            printf("\t  %s(%s)\n", 
                root->children[0]->lexinfo->c_str(),
                root->children[1]->lexinfo->c_str()); lbl = false;
        }
    } else { // If no args provided
        printf("\t  %s()\n", 
            root->children[0]->lexinfo->c_str()); lbl = false;
    }
}

void emit_if(astree* root) {
    // Save the if_nr for this if loop
    int this_if_nr = if_nr;
    // Print if label
    plbl(); printf(".if%d:", this_if_nr);

    // Generate expression for boolean
    emit_expr(root->children[0]);

    // Print goto if not...
    printf("\t  goto .el%d if not $t%d:i\n",
        this_if_nr, register_nr++); lbl = false;

    // If there is a block, emit it
    if(root->children[1]->symbol == TOK_BLOCK){
        plbl(); printf(".th%d:", this_if_nr);
        emit_block(root->children[1]);
    }else{ // Else emit the statement
        plbl(); printf(".th%d:", this_if_nr);
        emit_expr(root->children[1]);
    }

    // If there is a third arg
    if (root->children.size() == 3) {
        // Print the else label
        plbl(); printf(".el%d:", this_if_nr);
        // Lexinfo of third arg
        const char* str = root->children[2]->lexinfo->c_str();
        // Comparing the lexinfo of TOK_IFELSE
        if (strcmp(str, "if") == 0) { // if arg is an else if
            if_nr++;
            emit_if(root->children[2]);
        } else if (strcmp(str, "(") == 0) { // if arg is a call
            emit_call(root->children[2]);
        } else if (strcmp(str, "=") == 0) { // if arg is an asg
            emit_asg_expr(root->children[2],0);
        } else if (strcmp(str, "{") == 0) { // if arg is a block
            emit_block(root->children[2]);
        }
    }

    // Print the if end
    plbl(); printf(".fi%d:", this_if_nr);

}

// Generate code for while, need to rewrite (test 23)
void emit_while(astree* root){
    // Save the while_nr for this while loop
    int this_while_nr = while_nr;

    // Print while label
    plbl(); printf(".wh%d:", this_while_nr);

    // Generate expression for boolean
    emit_expr(root->children[0]);

    // Print goto if not...
    printf("\t  goto .od%d if not $t%d:i\n",
        this_while_nr, register_nr++); lbl = false;

    // If there is a block, emit it
    if(root->children[1]->symbol == TOK_BLOCK){
        plbl(); printf(".do%d:", this_while_nr); 
        emit_block(root->children[1]);
    }else{ // Else emit the statement
        plbl(); printf(".do%d:", this_while_nr);
        emit_expr(root->children[1]);
    }

    // Print the while end
    plbl(); printf(".od%d:", this_while_nr);
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
        case TOK_NE         : // fallthrough
        case TOK_EQ         : // fallthrough
        case TOK_GE         : emit_bin_expr(root);    break;
        case TOK_NOT        : emit_unary_expr(root);  break;
        case TOK_VARDECL    : emit_asg_expr(root, 1); break;
        case '='            : emit_asg_expr(root, 0); break;
        case TOK_IDENT      : // fallthrough
        case TOK_INTCON     : // fallthrough
        case TOK_STRINGCON  : // fallthrough
        case TOK_NULLPTR    :
            printf("\t  %s\n", root->lexinfo->c_str()); lbl = false;
            break;
        case TOK_ARROW      : break; //unimpl, refer to 6.2
        //unimpl, refer to 6.3 2nd paragraph
        case TOK_CALL       : emit_call(root); break; 
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
            root->children[start+1]->lexinfo->c_str()); lbl = false;
    } else { // If node is nested, traverse it
        emit_expr(root->children[start+1]);
        printf("\t  %s = $t%d:i\n", 
            root->children[start]->lexinfo->c_str(), 
            register_nr++); lbl = false;
        // Increment register number
    }
}

// Generate binary expression code
void emit_bin_expr(astree* root) {
    if(root->children.size() == 1) {
        emit_unary_expr(root);
        return;
    }

    // If reached deepest node
    if(root->children[0]->children.size() == 0) {
        printf("\t  $t%d:i = ", register_nr);
        printf("%s %s %s\n", 
            root->children[0]->lexinfo->c_str(),
            root->lexinfo->c_str(),
            root->children[1]->lexinfo->c_str()); lbl = false;
    } else { // Else keep traversing
        emit_expr(root->children[0]);
        printf("\t  $t%d:i = $t%d:i %s %s\n",
            register_nr + 1, 
            register_nr,
            root->lexinfo->c_str(),
            root->children[1]->lexinfo->c_str()); lbl = false;
        register_nr++;
    }

}

// Generate unary expression code
void emit_unary_expr(astree* root) {
    // If reached deepest node
    if(root->children[0]->children.size() == 0) {
        printf("\t  $t%d:i = ", register_nr);
        printf("%s %s\n", 
            root->lexinfo->c_str(),
            root->children[0]->lexinfo->c_str()); lbl = false;
    } else { // Else keep traversing
        emit_expr(root->children[0]);
        printf("\t  $t%d:i = %s $t%d:i\n",
            register_nr + 1, 
            root->lexinfo->c_str(),
            register_nr); lbl = false;
        register_nr++;
    }
}

//Generate global variable code
void emit_global_vars(astree* root){
    if(root->children[0]->symbol == TOK_INT){
        printf("%s:\t  .global int %s\n",
         root->children[1]->lexinfo->c_str()
         ,root->children[2]->lexinfo->c_str()); lbl = false;
    }else if(root->children[0]->symbol == TOK_STRING){
        printf(".s%d:\t  \"%s\"\n", 
        string_nr,root->children[2]->lexinfo->c_str());
        string_nr++; lbl = false;
    }else{
        printf("%s:\t  .global ptr %s\n", 
        root->children[1]->lexinfo->c_str(),
        root->children[2]->lexinfo->c_str()); lbl = false;
    }
}

// Called everytime a label is printed.
// This function ensures that there wont be two labels
// right after one another by printing a newline between them
void plbl() {
    if (lbl) printf("\n");
    lbl = true;
}
