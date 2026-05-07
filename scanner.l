%{
#include "tokens.h"
#include <string.h>
#include <stdlib.h>

extern int line_num;
%}

%option noyywrap

%%
"int"       { return INT; }
"char"      { return CHAR; }
"return"    { return RETURN; }
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"for"       { return FOR; }
"printf"    { return PRINTF; }

[0-9]+      { return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { return IDENTIFIER; }
\"[^\"\n]*\" { return STRING; }
">"|"<"|">="|"<="|"=="|"!=" { return REL_OP; }
"="         { return ASSIGN_OP; }
"+"|"-"|"*"|"/" { return ARITH_OP; }
";"|"("|")"|"{"|"}"|"," { return yytext[0]; }

[ \t\r]+    { /* ignore whitespace */ }
\n          { line_num++; }
.           { printf("[SCANNER ERROR] Line %d: Unknown character '%s'\n", line_num, yytext); }
%%