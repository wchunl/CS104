#ifndef __EMIT_H__
#define __EMIT_H__

#include "auxlib.h"
#include "astree.h"
#include <stdio.h>


void emit(FILE* outfile, astree* root, int depth = 0);
void emit_struct(FILE* outfile,astree* root);
void emit_function(FILE* outfile,astree*root);
void emit_param(FILE* outfile,astree* root);

#endif
