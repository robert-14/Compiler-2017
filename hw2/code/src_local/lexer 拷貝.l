%option noyywrap 
%{
#include <stdio.h>
#include "header.h" 
int linenumber;
symtab * lookup();
symtab * ptr;
void     insertID();
void 	 printSymTab();
/* You need to define for all tokens in C--, here are some examples */
#define INT		10
#define FLOAT 		11 
#define OP_ASSIGN 	12 
#define OP_OR 		13 
#define MK_LPAREN 	14 
#define MK_RPAREN 	15 
#define ERROR 		100 

%}

letter   [A-Za-z]
digit    [0-9]
ID	 {letter}({letter}|{digit}|"_")*
WS	 [ \t]+
Int_constant [-+]?{digit}+

/* You need to define the following RE's 
Float_constant
String_constant
comment
*/

Float_constant   [0-9]+\.[0-9]*[eE][+-]?[0-9]+|[0-9]*\.[0-9]+[eE][+-]?[0-9]+
String_constant   \"[^"]*\"
comment  \/\*([^*]|(\*+([^*/])))*\*+\/   



/* modified above */
RS_return  "return"
RS_typedef  "typedef"
RS_if       "if"
RS_else     "else"
RS_int      "int"
RS_float    "float"
RS_for      "for"
RS_void     "void"
RS_while    "while"
OP_assign        "="
OP_or   "||"
OP_and  "&&"
OP_not  "!"

/* Other operators appear here */ 
OP_add      "+"
OP_minus    "-"
OP_divide   "/"
OP_multi    "*"
OP_smaller  "<"
OP_greater  ">"
OP_smlequ   "<="
OP_grtequ   ">="
OP_nequ     "!="


newline  "\n"

DL_lbracket "["
DL_rbracket "]"
DL_lparen "("
DL_rparen ")"
DL_lbrace "{"
DL_rbrace "}"
DL_comma  ","
DL_semicol ";"
DL_dot  "."

/* Other separators appear here */

error    .

%%
{RS_return}  {}
{RS_typedef}  {}
{RS_if}       {}
{RS_else}     {}
{RS_int}      {}
{RS_float}    {}
{RS_for}      {}
{RS_void}     {}
{RS_while}    {}
{WS}            {}
{Int_constant}         {/* return INT; */ }
{Float_constant}    {}
{String_constant}  {}
{OP_and}      {}
{OP_not}      {}  
{OP_add}      {}
{OP_minus}    {}
{OP_divide}   {}
{OP_multi}    {}
{OP_smaller}  {}
{OP_greater}  {}
{OP_smlequ}   {}
{OP_grtequ}   {}
{OP_nequ}     {}
{DL_lbracket} {}
{DL_rbracket} {}
{ID}            { 
			ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);	
			else 
			     ptr->counter++;
		}

{OP_assign}     /* return OP_ASSIGN; */ 
{OP_or}         /* return OP_OR; */
{DL_lparen}     /* return MK_LPAREN; */
{DL_rparen}     /* return MK_RPAREN; */
{DL_lbrace}     /* return MK_LBRACE; */
{DL_rbrace}     /* return MK_RBRACE; */
{DL_comma}      {}
{DL_semicol}    {}
{DL_dot}         {}
{newline}       linenumber += 1;
{error}         {
                  printf("ERR %s\n",yytext);/* return ERROR; */  
                  return 0;
                }


{comment}       printf("%s\n",yytext); /* 2017.10.13 */


%%

int main(int argc, char **argv)
{
 argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   printSymTab();
   return 0;
}

