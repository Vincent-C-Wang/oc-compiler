
#include <iostream>
#include <string>
using namespace std;

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "auxlib.h"
#include "lyutils.h"
#include "astree.h"
#include "stringset.h"
#include "symtable.h"
#include "emit.h"

extern int   yy_flex_debug, yydebug;
FILE * tok_file; //pointer the tokenizer uses
FILE * sym_file;
char * file_name;
string commands = "cpp ";
extern astree *yyparse_astree;



void read_pipe( const char * commands) {
    yyin = popen(commands, "r");
    yyparse();
    pclose(yyin);
}

void write_ast(string filename) {
    FILE * output = fopen(filename.c_str(), "w");
    astree::print(output, yyparse_astree, 0);
    fclose(output);
}

void write_str( string filename ) {
    FILE * output = fopen(filename.c_str(), "w");
    stringset::dump_stringset(output);
    fclose(output);
}

void write_sym(string filename){
    sym_file = fopen(filename.c_str(), "w");
    typecheck(yyparse_astree);
    fclose(sym_file);
}
void write_oil(string filename){
  FILE* output =fopen(filename.c_str(), "w");
  emit(output, yyparse_astree);
}
int main( int argc, char** argv ) {

    char execname[] = "oc";
    set_execname(execname);

        int x;
        while ( (x = getopt( argc, argv, "D@ly" )) != -1 ) {
            switch (x) {
                case 'l':
                    yy_flex_debug = 1;
                    break;

                case 'y':
                    yydebug = 1;
                    break;

                case '@':
                    set_debugflags(optarg);
                    break;

                case 'D':
                    commands.append("-D ");
                    commands.append(optarg );
                    commands.append(" ");
                    break;
            }
        }
    file_name = argv[optind];
    commands.append(file_name);
    string name = string(file_name);
    name = name.substr(0, name.find_last_of("."));
    string strResult = name + ".str";
    string tokResult = name + ".tok";
    string astResult = name + ".ast";
    string symResult = name + ".sym";
    string oilResult = name + ".oil";


    tok_file = fopen(tokResult.c_str(), "w");
    read_pipe(commands.c_str());
    write_str(strResult);
    fclose(tok_file);
    write_ast(astResult);
    write_sym(symResult);
    write_oil(oilResult);
    return get_exitstatus();
}
