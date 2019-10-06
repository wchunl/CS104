%{
// Wai Chun Leung
// wleung11
// Shineng Tang
// stang38

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

%}

%option 8bit
%option debug
%option nobackup
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn
/*%option verbose*/

LETTER          [A-Za-z_]
DIGIT           [0-9]
MANTISSA        ({DIGIT}+\.?{DIGIT}*|\.{DIGIT}+)
EXPONENT        ([Ee][+-]?{DIGIT}+)
CHAR            ('([^\\'\n]|\\[\\'"0nt])')
STRING          (\"([^\\"\n]|\\[\\'"0nt])*\")
ICHAR           ('([^\\'\n]|\\[\\'"0nt])*\')
ISTRING         (\"([^\\"\n]|\\[\\'"0nt])*\)
INT             ({DIGIT}+)
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
IIDENT          ({DIGIT}({LETTER}|{DIGIT})*)


%%

"#".*           { lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }

"="             { return lexer::token ('='); }
"+"             { return lexer::token ('+'); }
"-"             { return lexer::token ('-'); }
"*"             { return lexer::token ('*'); }
"/"             { return lexer::token ('/'); }
"^"             { return lexer::token ('^'); }
"("             { return lexer::token ('('); }
")"             { return lexer::token (')'); }
";"             { return lexer::token (';'); }
"if"            { return lexer::token(TOK_IF);}
"else"          { return lexer::token(TOK_ELSE);}
"while"         { return lexer::token(TOK_WHILE);} 
"return"        { return lexer::token(TOK_RETURN);}
"alloc"         { return lexer::token(TOK_ALLOC);}
"nullptr"       { return lexer::token(TOK_NULLPTR); }
"not"           { return lexer::token(TOK_NOT); }
"int"           { return lexer::token(TOK_INT); }
"string"        { return lexer::token(TOK_STRING); }
"struct"        { return lexer::token(TOK_STRUCT); }
"array"         { return lexer::token(TOK_ARRAY); }
"ptr"           { return lexer::token(TOK_PTR); }
"void"          { return lexer::token(TOK_VOID); }
"=="            { return lexer::token(TOK_EQ); }
"!="            { return lexer::token(TOK_NE); }
"<"             { return lexer::token(TOK_LT); }
"<="            { return lexer::token(TOK_LE); }
">"             { return lexer::token(TOK_GT); }
">="            { return lexer::token(TOK_GE); }
"["             { return lexer::token('['); }
"]"             { return lexer::token(']'); }
"{"             { return lexer::token('{'); }
"}"             { return lexer::token('}'); }
","             { return lexer::token(','); }
"."             { return lexer::token('.'); }
"%"             { return lexer::token('%'); }
"!"             { return lexer::token('!'); }
{INT}           { return lexer::token(TOK_INTCON); }
{IDENT}         { return lexer::token( TOK_IDENT ); }
{STRING}        { return lexer::token(TOK_STRINGCON);}
{CHAR}          { return lexer::token(TOK_CHARCON);}
{IIDENT}        { return lexer::badtoken(*yytext);}
.               { lexer::badchar (*yytext); }

%%
