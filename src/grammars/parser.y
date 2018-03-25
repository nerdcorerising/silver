%no-lines


%{
    #include <stdio.h>
    #include <string>
    #include <memory>
    #include "ast.h"
    #include "parser.hpp"

    using namespace Cocoa;
    using namespace std;

    Assembly *assembly;

    extern int yylineno;
    extern int yylex();
    void yyerror(const char *s) { printf("ERROR: %s\n", s); }

    template<typename T>
    void saveLine(T t)
    {
        t->lineNumber == yylineno;
    }

%}

/* Represents the many different ways we can access our data */
%union {
    Cocoa::Assembly *prgm;
    Cocoa::Block *blck;
    Cocoa::Class *classdecl;
    Cocoa::Expression *expr;
    Cocoa::ExpressionList *elist;
    Cocoa::Import *import;
    Cocoa::Method *mdecl;
    Cocoa::MethodSignature *mtype;
    Cocoa::ArgumentList *alist;
    Cocoa::Argument *arg;
    Cocoa::Assignment *assign;
    Cocoa::MethodCall *mcall;
    Cocoa::YieldStatement *yld;
    Cocoa::ImportList *importvec;
    Cocoa::ClassList *classvec;
    Cocoa::MethodList *methvec;
    Cocoa::Identifier *ident;
    std::string *str;
    int token;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <str> TIDENTIFIER TINTEGER TDOUBLE TSTRINGLIT
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA TDOT TSEMI
%token <token> TPLUS TMINUS TMUL TDIV TCLASS TDECLARE
%token <token> TIMPORT TMETHOD TTRUE TFALSE TRETURNS TYIELD TYIELDS

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <prgm> program
%type <classvec> classlist
%type <methvec> methodlist
%type <importvec> importlist
%type <mdecl> method
%type <mtype> methodsig
%type <alist> arglist
%type <alist> args
%type <arg> argument
%type <classdecl> class
%type <import> import
%type <blck> block
%type <expr> expression
%type <expr> term
%type <expr> factor
%type <yld> yield
%type <mcall> methodcall
%type <expr> number
%type <ident> identifier
%type <assign> assignment
%type <elist> expressionlist
%type <elist> methodCallList
%type <elist> callList

/* Operator precedence for mathematical operators */
%left TPLUS TMINUS
%left TMUL TDIV

%start program

%%

program 
        : importlist classlist methodlist                                           { $$ = new Assembly($1, $2, $3); assembly = $$; saveLine($$); }
        ;

importlist
        :                                                                           { $$ = new ImportList(); saveLine($$); }
        | import importlist                                                         { $$ = $2; $$->add($1); saveLine($$); }
        ;
        
import
        : TIMPORT                                                                   { $$ = new Import(); saveLine($$); } // TODO
        ;
        
classlist 
        :                                                                           { $$ = new ClassList(); saveLine($$); }
        | class classlist                                                           { $$ = $2; $$->add($1); saveLine($$); }
        ;

class
        : TCLASS                                                                    { $$ = new Class(); saveLine($$); }
        ;

                
methodlist 
        :                                                                           { $$ = new MethodList(); saveLine($$); }
        | method                                                                    { $$ = new MethodList(); $$->add($1); saveLine($$); }
        | method methodlist                                                         { $$ = $2; $$->add($1); saveLine($$); }
        ;
        
method
        : methodsig arglist block                                                   { $$ = new Method($1, $2, $3); saveLine($$); }
        ;

methodsig
        : TYIELDS TIDENTIFIER                                                       { $$ = new MethodSignature($2, true); saveLine($$); }
        | TIDENTIFIER                                                               { $$ = new MethodSignature($1, false); saveLine($$); }
        ;

arglist
        : TLPAREN args TRPAREN                                                      { $$ = $2; saveLine($$); }
        ;

args
        :                                                                           { $$ = new ArgumentList(); saveLine($$); }
        | argument TCOMMA args                                                      { $$ = $3; $$->add($1); saveLine($$); } 
        | argument                                                                  { $$ = new ArgumentList(); $$->add($1); saveLine($$); }
        ;

argument
        : TIDENTIFIER                                                               { $$ = new Argument($1); saveLine($$); }
        ;

expressionlist
        :                                                                           { $$ = new ExpressionList(); saveLine($$); }
        | expressionlist expression TSEMI                                           { $$ = $1; $$->add($2); saveLine($$); }
        | expression TSEMI                                                          { $$ = new ExpressionList(); $$->add($1); saveLine($$); }
        ;

expression
        : expression TPLUS expression                                               { $$ = new AddExpression(Plus, $1, $3); saveLine($$); }
        | expression TMINUS expression                                              { $$ = new SubtractExpression(Minus, $1, $3); saveLine($$); }
        | term                                                                      { $$ = $1; saveLine($$); }
        | TTRUE                                                                     { $$ = new BooleanExpression($1); saveLine($$); }
        | TFALSE                                                                    { $$ = new BooleanExpression($1); saveLine($$); }
        | TSTRINGLIT                                                                { $$ = new StringLiteral($1); saveLine($$); }
        | assignment                                                                { $$ = $1; saveLine($$); }
        | methodcall                                                                { $$ = $1; saveLine($$); }
        | yield                                                                     { $$ = $1; saveLine($$); }
        ;

yield
        : TYIELD expression                                                         { $$ = new YieldStatement($2); saveLine($$); }
        ;

methodcall
        : identifier methodCallList                                                 { $$ = new MethodCall($1, $2); saveLine($$); }
        ;

methodCallList
        : TLPAREN callList TRPAREN                                                  { $$ = $2; saveLine($$); }
        ;

callList
        :                                                                           { $$ = new ExpressionList(); saveLine($$); }
        | expression TCOMMA callList                                                { $$ = $3; $$->add($1); saveLine($$); } 
        | expression                                                                { $$ = new ExpressionList(); $$->add($1); saveLine($$); }
        ;

assignment
        : TDECLARE identifier TEQUAL expression                                     { $$ = new Assignment($2, $4, true); saveLine($$); }
        | identifier TEQUAL expression                                              { $$ = new Assignment($1, $3, false); saveLine($$); }
        ;

term
        : term TMUL factor                                                          { $$ = new MultiplyExpression(Multiply, $1, $3); saveLine($$); }
        | term TDIV factor                                                          { $$ = new DivideExpression(Divide, $1, $3); saveLine($$); }
        | factor                                                                    { $$ = $1; saveLine($$); }
        ;

factor
        : number                                                                    { $$ = $1; saveLine($$); }
        | identifier                                                                { $$ = $1; saveLine($$); }
        | TLPAREN expression TRPAREN                                                { $$ = $2; saveLine($$); }
        ;

identifier
        : TIDENTIFIER                                                               { $$ = new Identifier($1); saveLine($$); }
        ;

number
        : TINTEGER                                                                  { $$ = new IntegerLiteral($1); saveLine($$); }
        | TDOUBLE                                                                   { $$ = new DoubleLiteral($1); saveLine($$); }
        ;

block
        : TLBRACE expressionlist TRBRACE                                            { $$ = new Block($2); saveLine($$); }
        ;
%%
