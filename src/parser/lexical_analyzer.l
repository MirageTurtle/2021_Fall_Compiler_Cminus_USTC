%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines = 1;
int pos_start;
int pos_end;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}


%%
 /* to do for students */
 /* two cases for you, pass_node will send flex's token to bison */
\+ 	{pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD;}

 /****请在此补全所有flex的模式与动作  end******/
if {pos_start = pos_end; pos_end += 2; pass_node(yytext); return IF;}
else {pos_start = pos_end; pos_end += 4; pass_node(yytext); return ELSE;}
int {pos_start = pos_end; pos_end += 3; pass_node(yytext); return INT;}
return {pos_start = pos_end; pos_end += 6; pass_node(yytext); return RETURN;}
void {pos_start = pos_end; pos_end += 4; pass_node(yytext); return VOID;}
while {pos_start = pos_end; pos_end += 5; pass_node(yytext); return WHILE;}
float {pos_start = pos_end; pos_end += 5; pass_node(yytext); return FLOAT;}

\- {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MINUS;}
\* {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MUL;}
\/ {pos_start = pos_end; pos_end += 1; pass_node(yytext); return DIV;}
\< {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LT;}
\<= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return LE;}
\> {pos_start = pos_end; pos_end += 1; pass_node(yytext); return BT;}
\>= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return BE;}
== {pos_start = pos_end; pos_end += 2; pass_node(yytext); return EQ;}
!= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return NEQ;}
= {pos_start = pos_end; pos_end += 1; pass_node(yytext); return ASSIGN;}
; {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SEMICOLON;}
, {pos_start = pos_end; pos_end += 1; pass_node(yytext); return COMMA;}
\( {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LPAREN;}
\) {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RPAREN;}
\[ {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACKET;}
\] {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACKET;}
\{ {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACE;}
\} {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACE;}

[a-zA-Z]+ {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return ID;}
[0-9]+ {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return INTEGER;}
([0-9]+\.|[0-9]*\.[0-9]+) {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return FLOAT;}
\/\*[^\*]*(\*+[^\*\/][^\*]*)*\**\*\/ {pos_start = pos_end; pos_end += yyleng;}
[ \t\r\n]+ {pos_start = pos_end; pos_end += yyleng;}
[\n]+ {lines += yyleng; pos_start = 0; pos_end = 0;}
. { pos_start = pos_end; pos_end++; return ERROR; }
%%
