%option verbose
%option never-interactive
%option nounistd
%option yylineno

%{
    #include <string>
    #include "ast.h"
    #include "parser.hpp"
    #define saveToken() yylval.str = new std::string(yytext, yyleng)
    #define TOKEN(t) (yylval.token = t)
    extern "C" int yywrap() { return 1; }
%}



%%

[ \t\n\r]                                   ;
[#]+.*                                      ;
true                                    return TOKEN(TTRUE);
false                                   return TOKEN(TFALSE);
class                                   return TOKEN(TCLASS);
import                                  return TOKEN(TIMPORT);
var                                     return TOKEN(TDECLARE);
yield                                   return TOKEN(TYIELD);
yields                                  return TOKEN(TYIELDS);
\"(\\.|[^\\"])*\"                       saveToken(); return (int)TSTRINGLIT;
[a-zA-Z][a-zA-Z0-9_]*                   saveToken(); return (int)TIDENTIFIER;
[0-9]+                                  saveToken(); return (int)TINTEGER;
[0-9]+\.[0-9]*                          saveToken(); return (int)TDOUBLE;
"="                                     return TOKEN(TEQUAL);
"=="                                    return TOKEN(TCEQ);
"!="                                    return TOKEN(TCNE);
"<"                                     return TOKEN(TCLT);
"<="                                    return TOKEN(TCLE);
">"                                     return TOKEN(TCGT);
">="                                    return TOKEN(TCGE);
"("                                     return TOKEN(TLPAREN);
")"                                     return TOKEN(TRPAREN);
"{"                                     return TOKEN(TLBRACE);
"}"                                     return TOKEN(TRBRACE);
"."                                     return TOKEN(TDOT);
","                                     return TOKEN(TCOMMA);
";"                                     return TOKEN(TSEMI);
"+"                                     return TOKEN(TPLUS);
"-"                                     return TOKEN(TMINUS);
"*"                                     return TOKEN(TMUL);
"/"                                     return TOKEN(TDIV);
.                                       printf("Unknown token!\n"); yyterminate();

%%
