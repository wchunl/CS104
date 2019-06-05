#ifndef __EMIT_H__
#define __EMIT_H__

#include "auxlib.h"
#include "astree.h"
#include <stdio.h>

void emit(FILE* outfile, astree* root, int depth = 0);
void emit_struct(astree* root);
void emit_block(astree* root);
void emit_while(astree* root);
void emit_if(astree* root);

// Functions
void emit_function(astree*root);
void emit_param(astree* root);
void emit_local_vars(astree* root);
void emit_global_vars(astree* root);

// Expressions
void emit_expr(astree* root);
void emit_bin_expr(astree*root);
void emit_unary_expr(astree*root);

// // Other
// int get_reg_nr(const char* key);
// void debug_regis();

#endif
