#ifndef PARSER_H
#define PARSER_H

#define SYMBOL_PASS 1
#define MAIN_PASS 2

int   yyparse   (void);
int   yylex     (void);
int   yywrap    (void);
void  yyerror   (char *errmsg);

#endif /* PARSER_H */
