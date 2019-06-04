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

unordered_map<const char*, int> regis;

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
            case TOK_BLOCK:
                emit_block(child);
                break;
            case TOK_PARAM:
                emit_param(child);
                break;
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
        }
    }
     printf("\t  %s\n",".end");
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

void emit_block(astree* root){
    for(auto* child: root->children){
        switch(child->symbol){
            case TOK_VARDECL:
                // Print identifier and type
                if(child->children[0]->symbol == TOK_INT){
                    printf("\t  .local int %s\n", child->children[1]->lexinfo->c_str());
                }else{
                   printf("\t  .local ptr %s\n", child->children[1]->lexinfo->c_str());  
                }

                // Insert varaible into register map
                regis.insert({child->children[1]->lexinfo->c_str(), register_nr});
                register_nr++;

                // if (child->children[2]->symbol == '+') {
                //     emit_asg(child);
                // }
                // debug_regis();
                break;
            case TOK_RETURN : 
                printf("\t  return %s\n",child->children[0]->lexinfo->c_str());
                printf("\t  return \n");
                break;
            // case '=' : 
            case TOK_CALL   : break;
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

void emit_if(astree* root) {
    //unimpl
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
        printf("%s%d:\t  %s%d:i = ",".wh",while_nr,"$t",
            get_reg_nr(root->children[0]->children[0]->lexinfo->c_str()));
        emit_bin_expr(root->children[0]);
        printf("\n");
            // root->children[0]->children[0]->lexinfo->c_str(),
            // root->children[0]->children[0]->lexinfo->c_str(),
            // root->children[0]->lexinfo->c_str(),
            // root->children[0]->children[1]->lexinfo->c_str());
    }
    printf("\t  goto .od%d if not $t%d:i\n",while_nr,
        get_reg_nr(root->children[0]->children[0]->lexinfo->c_str()));
        // root->children[0]->children[0]->lexinfo->c_str());
    // register_nr++;

    //create do, call emit_block
    if(root->children[1]->symbol == TOK_BLOCK){
        printf("%s%d:",".do",while_nr);
        emit_block(root->children[1]);
    }

    printf(".od%d:", while_nr);
}

// Read a node if it is an expression
void emit_expr(astree* root){
    switch(root->symbol) {
        case '=' : {//assignment
            int regnr =  get_reg_nr(root->children[0]->lexinfo->c_str());
            if (regnr != -1) printf("\t  %s%s%d:i = ", "", "$t", regnr);
            // else printf("ERROR: could not find symbol, ignoring \n");

            emit_bin_expr(root->children[1]);
            printf("\n");
            break;}
        case '+'        : // fallthrough
        case '-'        : // fallthrough
        case '/'        : // fallthrough
        case '*'        : // fallthrough
        case '<'        : // fallthrough
        case '%'        : // fallthrough
        case TOK_LE     : // fallthrough
        case '>'        : // fallthrough
        case TOK_GE     : emit_bin_expr(root);
        default: break;
    }
}

// Generate binary expression code
void emit_bin_expr(astree*root) {
    if (root->children.size() == 2) {
        // Inorder traversal
        emit_bin_expr(root->children[0]);
        printf("%s ", root->lexinfo->c_str());
        emit_bin_expr(root->children[1]);
    } else {
        printf("%s ", root->lexinfo->c_str());
    }
}

int get_reg_nr(const char* key) {
    auto i = regis.find(key);
    if (i != regis.end()) {
        return i->second;
    } else {
        return -1;
    }
}

void debug_regis() {
    cout << "\n------ debugging regis -------\n";
    for (auto const& pair: regis) {
        printf("name: %-10s | regis_nr: %d\n", pair.first,pair.second);
    }
}