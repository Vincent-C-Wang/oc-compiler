
#ifndef __UTILS_H__
#define __UTILS_H__

// Lex and Yacc interface utility.

#include <string>
#include <vector>
using namespace std;

#include <stdio.h>

#include "astree.h"
#include "auxlib.h"

#define YYEOF 0

extern FILE* yyin;
extern char* yytext;
extern int yy_flex_debug;
extern int yydebug;
extern size_t yyleng;

extern astree* yyparse_astree;
extern vector<astree*> string_queue;

int yylex();
int yylex_destroy();
int yyparse();
void yyerror (const char* message);
const char * get_yytname (int symbol);
int yylval_token(int symbol);
astree* new_parseroot (void);

struct lexer {
   static bool interactive;
   static location lloc;
   static size_t last_yyleng;
   static vector<string> filenames;
   static const string* filename (int filenr);
   static void newfilename (const string& filename);
   static void advance();
   static void newline();
   static void badchar (unsigned char bad);
   static void badtoken (char* lexeme);
   static void include();
};

struct parser {
   static astree* root;
   static const char* get_tname (int symbol);
};

#define YYSTYPE astree*
#include "yyparse.h"

#endif
