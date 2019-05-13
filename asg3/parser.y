%{
    // Wai Chun Leung
    // wleung11
    // Shineng Tang
    // stang38
    
    // $Id: parser.y,v 1.22 2019-04-23 14:07:52-07 - - $
    // Dummy parser for scanner project.
    
#include <cassert>
#include "lyutils.h"
#include "astree.h"
%}
    
%debug
%defines
%error-verbose
%token-table
%verbose

%destructor { destroy ($$); } <>
%printer { astree::dump (yyoutput, $$); } <>

%initial-action {
    parser::root = new astree (ROOT, {0, 0, 0}, "");
}

%token ROOT TOK_TYPE_ID TOK_FIELD
%token TOK_VOID TOK_INT TOK_STRING TOK_INDEX
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULLPTR TOK_ARRAY TOK_ARROW TOK_ALLOC TOK_PTR
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE TOK_NOT
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON
%token TOK_ROOT TOK_BLOCK TOK_CALL TOK_INITDECL
%token TOK_VARDECL TOK_RETURNVOID TOK_IFELSE TOK_PARAM
%token TOK_FUNCTION POS NEG TOK_PROTOTYPE

%right  TOK_IF TOK_ELSE
%right  '='
%left   TOK_EQ TOK_NE TOK_GE TOK_LE '<' '>'
%left   '+' '-'
%left   '*' '/' '%'
%right  POS NEG TOK_NOT
%left   '[' TOK_ARROW TOK_CALL TOK_ALLOC
%nonassoc   '('

%start start

%%
start       : program { $$ = $1 = nullptr; }

program     : program structdef     { $$ = $1->adopt ($2);}
            | program function      { $$ = $1->adopt ($2);}
            | program stmt          { $$ = $1->adopt ($2); }
            | program error '}'     { destroy ($3); $$ = $1;}
            | program error ';'     { destroy ($3); $$ = $1;}
            |                       { $$ = parser::root; }

structdef   : args '}' ';'                          {
                 $$ = $1; destroy($2,$3); } 
            | TOK_STRUCT TOK_IDENT '{' '}' ';'      {
                $$ = $1->adopt($2); destroy($3,$4,$5); }   

args        : args identdecl ';'                    { 
                $$ = $1->adopt($2); destroy($3); }
            | TOK_STRUCT TOK_IDENT '{' identdecl';' { 
                $$ = $1->adopt($2,$4); destroy($3,$5); } 

identdecl   : type TOK_IDENT   {
                $2->change_sym(TOK_TYPE_ID);
                $$ = $1->adopt($2); }

stmt        : vardecl    { $$ = $1; }
            | block      { $$ = $1; }
            | while      { $$ = $1; }
            | ifelse     { $$ = $1; }
            | return     { $$ = $1; }
            | ';'        { $$ = $1; }
            | expr ';'   { $$ = $1; destroy($2); }

vardecl     : identdecl '=' expr ';' {
                destroy($4); 
                $$ = $2->adopt_sym($1,TOK_VARDECL);
                $$ = $2->adopt($3);}

return      : TOK_RETURN expr ';' {destroy($3); $$ = $1->adopt($2);}
            | TOK_RETURN ';'      {
                destroy($2); 
                $$ = $1->change_sym(TOK_RETURNVOID);}

while       : TOK_WHILE '(' expr ')' stmt {destroy($2,$4); 
                                    $$ = $1->adopt($3,$5);}

ifelse      : TOK_IF '(' expr ')'stmt TOK_ELSE stmt {
                $$ = $1->adopt($3,$5,$7);$7->change_sym(TOK_IFELSE); 
                destroy($2,$4,$6); }
            | TOK_IF '(' expr ')' stmt %prec TOK_ELSE {
                $$ = $1->adopt($3,$5); destroy($2,$4); }

block       : '{' block_rec '}' {
                $$=$1->adopt_sym($2,TOK_BLOCK); destroy($3); }
            | '{''}' {
                $$ = $1->adopt_sym(nullptr,TOK_BLOCK); destroy($2); }

block_rec   : stmt { astree* t = new astree(TOK_FUNCTION, $1->lloc,"");
                          $$ = t->adopt($1);}
            | block_rec stmt { $$ = $1->adopt($2);}

expr        : expr '=' expr         { $$ = $2->adopt($1, $3); }
            | expr '+' expr         { $$ = $2->adopt($1, $3); }
            | expr '-' expr         { $$ = $2->adopt($1, $3); }
            | expr '*' expr         { $$ = $2->adopt($1, $3); }
            | expr '/' expr         { $$ = $2->adopt($1, $3); }
            | expr '%' expr         { $$ = $2->adopt($1, $3); }
            | expr TOK_EQ expr      { $$ = $2->adopt($1, $3); }
            | expr TOK_NE expr      { $$ = $2->adopt($1, $3); }
            | expr TOK_GE expr      { $$ = $2->adopt($1, $3); }
            | expr TOK_LE expr      { $$ = $2->adopt($1, $3); }
            | TOK_NOT expr          { $$ = $1->adopt($2); }
            | expr '<' expr         { $$ = $2->adopt($1, $3);}
            | expr '>' expr         { $$ = $2->adopt($1, $3);}
            | '-' expr %prec NEG    { $$ = $1->adopt($2); }
            | '+' expr %prec POS    { $$ = $1->adopt($2); }
            | allocator             { $$ = $1; }
            | call                  { $$ = $1; }
            | '(' expr ')'          {destroy($1,$3);$$ = $2;}
            | variable              { $$ = $1; }
            | constant              { $$ = $1; }

exprs       : exprs ',' expr        { $$ = $1->adopt($3); destroy($2);}
            | expr                  { $$ = $1;}

allocator   : TOK_ALLOC '<' TOK_STRING '>' '(' expr ')' {
                destroy ($2,$4,$5,$7); $$ = $1->adopt($3,$6); }
            | TOK_ALLOC '<' TOK_STRUCT TOK_IDENT '>' '(' ')'  {
                destroy($2,$3,$5,$6,$7);
                $4->change_sym(TOK_TYPE_ID);
                $$ = $1->adopt($4); }
            | TOK_ALLOC '<' TOK_ARRAY'<' plaintype'>''>''(' expr')'  {
                destroy($2,$4,$6);
                destroy($7,$8,$10);
                $$ = $1->adopt($3);
                $5->change_sym(TOK_TYPE_ID);
                $$ =$1->adopt($5,$9); }

plaintype   : TOK_VOID                              { $$ = $1; }
            | TOK_INT                               { $$ = $1; }
            | TOK_STRING                            { $$ = $1; }
            | TOK_PTR '<' TOK_STRUCT TOK_IDENT'>'   {
                destroy($2,$3,$5);
                $4->change_sym(TOK_TYPE_ID);$$ = $1->adopt($4); }

type        : plaintype      { $$ = $1; }
            | TOK_ARRAY '<' plaintype '>' {destroy($2, $4);
                $3->change_sym(TOK_TYPE_ID);
                $$ = $1->adopt($3);}

function    : type TOK_IDENT params ')' block { 
                destroy($4);
                astree* func = new astree(TOK_FUNCTION, $1->lloc,"");
                astree* id = new astree(TOK_TYPE_ID, $1->lloc,"");
                id->adopt($1,$2);
                $$ = func->adopt(id,$3,$5); }
            | type TOK_IDENT '(' ')' block { 
                destroy($4);
                $3->change_sym(TOK_PARAM);
                astree* func = new astree(TOK_FUNCTION, $1->lloc,"");
                astree* id = new astree(TOK_TYPE_ID, $1->lloc,"");
                id->adopt($1,$2);
                $$ = func->adopt (id,$3,$5); }
            | type TOK_IDENT params ')' ';' { 
                destroy($4,$5);
                astree* func = new astree(TOK_FUNCTION, $1->lloc,"");
                astree* id = new astree(TOK_TYPE_ID, $1->lloc,"");
                id->adopt($1,$2);
                $$ = func->adopt(id,$3); }
            | type TOK_IDENT '(' ')' ';' { 
                destroy($4,$5);
                $3->change_sym(TOK_PARAM);
                astree* func = new astree(TOK_FUNCTION, $1->lloc,"");
                astree* id = new astree(TOK_TYPE_ID, $1->lloc,"");
                id->adopt($1,$2);
                $$ = func->adopt (id,$3); }

params      : '(' type TOK_IDENT {
                astree* id = new astree(TOK_TYPE_ID, $2->lloc,"");
                id->adopt($2,$3);
                $$ = $1->adopt_sym(id,TOK_PARAM); }
            | params ',' type TOK_IDENT {
                destroy($2); 
                astree* id = new astree(TOK_TYPE_ID, $3->lloc,"");
                id->adopt($3,$4);
                $$ = $1->adopt(id); }


call        : TOK_IDENT '(' exprs ')'   { 
                destroy($4); $2->change_sym(TOK_CALL);
                $$ = $2->adopt($1,$3); }
            | TOK_IDENT '(' ')' { 
                destroy($3); 
                $2->change_sym(TOK_CALL);$$ = $2->adopt($1); }

variable    : TOK_IDENT        { $$ = $1; }
            | expr '[' expr ']'{ 
                destroy($4); 
                $2->change_sym(TOK_INDEX);
                $$ = $2->adopt($1,$3); }
            | expr  TOK_ARROW  TOK_IDENT   { 
                $$ = $2->adopt($1,$3); }

constant    : TOK_INTCON          { $$ = $1; }
            | TOK_CHARCON         { $$ = $1; }
            | TOK_STRINGCON       { $$ = $1; }
            | TOK_NULLPTR         { $$ = $1; }
            ;
%%


const char *parser::get_tname (int symbol) {
    return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
    return YYTRANSLATE (symbol) > YYUNDEFTOK;
}
