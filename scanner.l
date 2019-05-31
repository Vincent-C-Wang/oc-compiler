%{

#include "lyutils.h"

#define YY_USER_ACTION { lexer::advance(); }

#define RETURN(SYM) { return yylval_token(SYM); }

%}

%option 8bit
%option debug
%option nodefault
%option nounput
%option noyywrap
%option verbose
%option warn

LETTER          [A-Za-z_]
DIGIT           [0-9]

INTEGER         ({DIGIT}+)
CHAR            ('([^\\'\n]|\\[\\'"0nt])')
STRING          (\"([^\\"\n]|\\[\\'"0nt])*\")
IDENT           ({LETTER}({LETTER}|{DIGIT})*)

BAD_IDENT       ({DIGIT}+{LETTER}({LETTER}|{DIGIT})*)
BAD_CHAR        ('([^\\'\n]|\\[\\'"0nt]?)[^']?)
BAD_STRING      (\"([^\\"\n]|\\[\\'"0nt])*[^\"]?)

%%

"#".*           { lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }

"char"          { RETURN (TOK_CHAR);   }
"else"          { RETURN (TOK_ELSE);   }
"if"            { RETURN (TOK_IF);     }
"int"           { RETURN (TOK_INT);    }
"new"           { RETURN (TOK_NEW);    }
"null"          { RETURN (TOK_NULL);   }
"return"        { RETURN (TOK_RETURN); }
"string"        { RETURN (TOK_STRING); }
"struct"        { RETURN (TOK_STRUCT); }
"void"          { RETURN (TOK_VOID);   }
"while"         { RETURN (TOK_WHILE);  }

{IDENT}         { RETURN (TOK_IDENT);     }
{INTEGER}       { RETURN (TOK_INTCON);    }
{CHAR}          { RETURN (TOK_CHARCON);   }
{STRING}        { RETURN (TOK_STRINGCON); }

"!="            { RETURN (TOK_NE); }
"<="            { RETURN (TOK_LE); }
"=="            { RETURN (TOK_EQ); }
">="            { RETURN (TOK_GE); }

"!"             { RETURN ('!');    }
"%"             { RETURN ('%');    } 
"("             { RETURN ('(');    }
")"             { RETURN (')');    }
"*"             { RETURN ('*');    }
"+"             { RETURN ('+');    }
","             { RETURN (',');    }
"-"             { RETURN ('-');    }
"."             { RETURN ('.');    }
"/"             { RETURN ('/');    }
";"             { RETURN (';');    }
"<"             { RETURN ('<');    }
"="             { RETURN ('=');    }
">"             { RETURN ('>');    }
"["             { RETURN ('[');    }
"]"             { RETURN (']');    }
"{"             { RETURN ('{');    }
"}"             { RETURN ('}');    }
"[]"            { RETURN (TOK_ARRAY); }


{BAD_IDENT}     { lexer::badtoken(yytext); }
{BAD_CHAR}      { lexer::badtoken(yytext); }
{BAD_STRING}    { lexer::badtoken(yytext); }
.               { lexer::badchar(*yytext); }

%%
