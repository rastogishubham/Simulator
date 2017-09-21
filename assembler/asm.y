%{
/* language definition file */

/* includes */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "parser.h"
#include "semantic.h"

/* line number in file */
extern int lineno;
/* which compiler pass */
extern int pass;
/* found label */
static int label_arg;
%}

%union {
  char  *string;
  int   number;
  int   token_val;
}

%token <token_val> kADD kADDI kADDIU kADDU kAND kANDI kBEQ kBNE kBLEZ kBGTZ
                   kJ kJAL kJR kLBU kLHU kLUI kLW kNOR kOR kORI kSLT
                   kSLTI kSLTIU kSLTU kSLL kSLLV kSRL kSRLV kSB kSH kSW kSUB
                   kSUBU kTSL kLL kSC kXOR kXORI kHALT kNOP kPUSH kPOP
                   kORG kCHW kCFW 
                   kNEWLINE COMMA LPAREN RPAREN

%token <number> INTEGER
%token <number> REG_NUM
%token <string> LABEL_DEC
%token <string> LABEL
%token <string> STRING

%type <string> program
%type <string> statement_list
%type <string> statement
%type <string> label
%type <number> immediate
%type <token_val> wordlist
%type <token_val> word
%type <token_val> halfwordlist
%type <token_val> halfword
%type <token_val> rtype
%type <token_val> rstype
%type <token_val> itype
%type <token_val> istype
%type <token_val> ihtype
%type <token_val> jtype


%%

program:
   {start ();} statement_list {finish ();}
   ;

statement_list:
     statement
   | statement_list statement
   ;

statement:  label rtype REG_NUM COMMA REG_NUM COMMA REG_NUM kNEWLINE
    {
      genRType ($2, $3, $5, $7);
    }
  | label rtype kNEWLINE
    {
      genRType ($2, 0, 0, 0);
    }
  | label rtype REG_NUM kNEWLINE
    {
      genRType ($2, 0, $3, 0);
    }
  | label rstype REG_NUM COMMA REG_NUM COMMA immediate kNEWLINE
    {
      genRType ($2, $3, $5, $7);
    }

  | label itype REG_NUM COMMA REG_NUM COMMA immediate kNEWLINE
    {
      genIType ($2, $3, $5, $7);
    }
  | label itype REG_NUM COMMA immediate LPAREN REG_NUM RPAREN kNEWLINE
    {
      genIType ($2, $3, $7, $5);
    }
  | label istype REG_NUM COMMA immediate kNEWLINE
    {
      genIType ($2, $3, 0, $5);
    }
  | label ihtype kNEWLINE
    {
      genIType ($2, 0, 0, 0);
    }

  | label jtype LABEL kNEWLINE
    {
      genJType ($2, resolveLabel ($3));
    }

  | label kPUSH REG_NUM kNEWLINE
    {
      genPush ($3);
    }
  | label kPOP REG_NUM kNEWLINE
    {
      genPop ($3);
    }

  | kORG INTEGER kNEWLINE
    {
      processOrg ($2);
    }
  | label kCHW halfwordlist kNEWLINE
    {
      $$ = NULL;
      checkAlignment ();
    }
  | label kCFW wordlist kNEWLINE
    {
      $$ = NULL;
    }
  | label kNEWLINE
  ;

rtype: kADD {$$ = kADD} | kADDU {$$ = kADDU} | kAND {$$ = kAND} | kJR {$$ = kJR}
     | kNOR {$$ = kNOR} | kOR {$$ = kOR} | kSLT {$$ = kSLT} | kSLTU {$$ = kSLTU}
     | kSUB {$$ = kSUB} | kSUBU {$$ = kSUBU} | kXOR {$$ = kXOR} | kNOP {$$ = kNOP}
     | kSLLV {$$ = kSLLV} | kSRLV {$$ = kSRLV}
     ;

rstype: kSLL {$$ = kSLL} | kSRL {$$ = kSRL}
      ;

itype: kADDI {$$ = kADDI} | kADDIU {$$ = kADDIU} | kANDI {$$ = kANDI} | kBEQ {$$ = kBEQ}
     | kBNE {$$ = kBNE} | kLBU {$$ = kLBU} | kLHU {$$ = kLHU} | kLL {$$ = kLL} | kSC {$$ = kSC}
     | kLW {$$ = kLW} | kTSL {$$ = kTSL} | kORI {$$ = kORI} | kSLTI {$$ = kSLTI} | kSLTIU {$$ = kSLTIU}
     | kSB {$$ = kSB} | kSH {$$ = kSH} | kSW {$$ = kSW} | kXORI {$$ = kXORI}
     ;

istype: kLUI {$$ = kLUI} | kBLEZ {$$ = kBLEZ} | kBGTZ {$$ = kBGTZ}
      ;

ihtype: kHALT {$$ = kHALT}
      ;

jtype: kJ {$$ = kJ} | kJAL {$$ = kJAL}
     ;

label:    LABEL_DEC { if (pass == SYMBOL_PASS) processLabel ($1); }
  | /* nothing */ {$$ = NULL}
  ;

immediate:  INTEGER {label_arg = 0;}
  | LABEL {$$ = resolveLabel ($1);label_arg = 1;}
  ;

wordlist: word
  | wordlist COMMA word
  ;

word: INTEGER
  {
    processWord ($1);
  }
  | LABEL
  {
    processWord (resolveLabel ($1));
  }
    ;

halfwordlist: halfword
  | halfwordlist COMMA halfword
  ;

halfword: INTEGER
      { processHalfWord ($1); }
    | STRING
      { char *ptr = $1;
        while (*ptr) {
          processHalfWord (*ptr);
          ptr++;
        }
      }
    ;




%%



int yywrap(void)
{
  return (1);
}

void yyerror(char *errmsg)
{
  extern char *yytext;
   printf ("Line %d: %s '%s'\n", lineno, errmsg, yytext);
   exit (ERROR);
}

