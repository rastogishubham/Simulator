#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "common.h"
#include "parser.h"
#include "semantic.h"

#define DEFAULT_OUT "meminit.hex"

extern int yydebug;
extern int lineno;

int version_major = 2;
int version_minor = 0;

int debug;
int pass;
/* maybe this should be extern? */
u_int32 addr_inc_amount;
int output_type;
int print_sym_table;

char output_type_names[][15] = {"None","Ram File","Intel Hex","Altera Mif"};
char output_file_names[][15] = {"None","meminit.ram","meminit.hex","meminit.mif"};

FILE *output_file;

void usage (char *);
void printISA (void);

int main (int argc, char **argv)
{
  char* outfile;
  char* usrfile;
  char* infile;
  int c;
  extern FILE *yyin;

#if YYDEBUG
   yydebug = 1;
#endif

  /* setup parameters */
  debug = FALSE;
  outfile = DEFAULT_OUT;
  usrfile = output_file_names[0];
  addr_inc_amount = WORD_SIZE;
  /* change me to intel hex */
  output_type = OUTPUT_INTELHEX;
  print_sym_table = FALSE;

  if (argc < 2)
  {
    usage (argv[0]);
    exit (ERROR);
  }

  while ((c = getopt (argc,argv,"hxrvilb:a:o:")) != EOF)
  {
    switch (c)
    {
      case 'h':
        usage (argv[0]);
        exit (0);

      case 'i':
        printISA ();
        exit (0);

      case 'x':
        output_type = OUTPUT_INTELHEX;
        outfile = output_file_names[OUTPUT_INTELHEX];
        break;

      case 'r':
        output_type = OUTPUT_RAM_FILE;
        outfile = output_file_names[OUTPUT_RAM_FILE];
        break;

      case 'v':
        debug = TRUE;
        break;

      case 'l':
        print_sym_table = TRUE;
        break;

      case 'b':
        if (!isdigit ((int)optarg[0]))
        {
          fprintf (stderr,"ERROR: -b must be followed by a number\n");
          usage (argv[0]);
          exit (ERROR);
        }
        addr_inc_amount = atoi(optarg);
        break;

      case 'o':
        usrfile = strdup(optarg);
        break;

      default:
        fprintf (stderr,"unrecognized option: -%c\n",optopt);
        exit (ERROR);
    }
  }
  /* user changed output filename */
  if (strcmp(usrfile, output_file_names[0]))
  {
    outfile = usrfile;
  }

  /* still some options left on compand line */
  if (optind < argc)
  {
    infile = argv[optind];
  }
  else
  {
    fprintf (stderr,"Input filename not specified\n");
    exit (ERROR);
  }

  /* hack to add new line to end of file */
  if (access (infile, F_OK) == 0)
  {
    /* open file */
    yyin = fopen (infile,"a+");
    if (yyin)
    {
      char c;
      fseek (yyin, -1, SEEK_END);
      c = fgetc (yyin);
      /* check for new line */
      if (c != '\n')
      {
        fseek (yyin, 0, SEEK_END);
        fputc ('\n', yyin);
      }
      /* close file */
      fclose (yyin);
    }
  }

  /* open file for reading */
  yyin = fopen (infile, "r");

  if (yyin == NULL)
  {
    fprintf (stderr, "Error: Cannot open %s\n", infile);
    exit (1);
  }

  /* open output file for writing */
  output_file = fopen (outfile, "w");

  if (output_file == NULL)
  {
    fprintf (stderr, "Error: Cannot open %s\n", outfile);
    fclose (yyin);
    exit (1);
  }

  /* set line number and first pass */
  lineno = 1;
  pass = SYMBOL_PASS;
  yyparse ();

  /* second pass */
  rewind (yyin);
  lineno = 1;
  pass = MAIN_PASS;
  yyparse ();

  /* close files */
  fclose (yyin);
  fclose (output_file);

  exit(0);
}


void usage (char *command)
{
  fprintf (stderr, "\nUsage: %s [options] input_file\n", command);
  fprintf (stderr, "Ver: %d.%d  Written by: Eric Villasenor "
      "(Kurt Otte & Rick Kennell)\n\n",
      version_major, version_minor);

  fprintf (stderr, "   -h           Help\n"
                   "   -i           Print Instruction Set Arch\n"
                   "   -r           Output type set to a ram file(legacy)\n"
                   "   -x           Output type set to intel hex file(default)\n"
                   "   -v           Turn debuging info on in hex file\n"
                   "   -l           Print labels\n"
                   "   -b #         Ram depth (num of bytes per address)  \n"
                   "   -o filename  Specify output filename (default meminit.hex)\n"
                   "\n");

  fprintf (stderr, "Defaults\n");
  fprintf (stderr, "   Default Output File: %s\n", DEFAULT_OUT);
  fprintf (stderr, "   Default Output Type: %s\n", output_type_names[output_type]);
  if (addr_inc_amount == 1)
  {
    fprintf (stderr, "   Ram Depth: 16 bits (word) per address\n\n");
  }
  else if (addr_inc_amount == 2)
  {
    fprintf (stderr, "   Ram Depth: 8 bits (byte) per address\n\n");
  }
  else
  {
    fprintf (stderr, "   Ram Depth: One word per %d byte addresses\n\n",
    addr_inc_amount);
  }
}

void printISA (void)
{
  printf ("\nMIPS Instruction Set Architecture\n\n"
          "---------------------<reserved registers>------------------------\n"
          "$0                   zero\n"
          "$1                   assembler temporary\n"
          "$29                  stack pointer\n"
          "$31                  return address\n"
          "---------------------<R-type Instructions>-----------------------\n"
          "ADDU   $rd,$rs,$rt   R[rd] <= R[rs] + R[rt] (unchecked overflow)\n"
          "ADD    $rd,$rs,$rt   R[rd] <= R[rs] + R[rt]\n"
          "AND    $rd,$rs,$rt   R[rd] <= R[rs] AND R[rt]\n"
          "JR     $rs           PC <= R[rs]\n"
          "NOR    $rd,$rs,$rt   R[rd] <= ~(R[rs] OR R[rt])\n"
          "OR     $rd,$rs,$rt   R[rd] <= R[rs] OR R[rt]\n"
          "SLT    $rd,$rs,$rt   R[rd] <= (R[rs] < R[rt]) ? 1 : 0\n"
          "SLTU   $rd,$rs,$rt   R[rd] <= (R[rs] < R[rt]) ? 1 : 0\n"
          //"SLL    $rd,$rs,shamt R[rd] <= R[rs] << shamt\n"
          "SLLV   $rd,$rs,$rt   R[rd] <= R[rt] << [0:4] R[rs]\n"
          //"SRL    $rd,$rs,shamt R[rd] <= R[rs] >> shamt\n"
          "SRLV   $rd,$rs,$rt   R[rd] <= R[rt] >> [0:4] R[rs]\n"
          "SUBU   $rd,$rs,$rt   R[rd] <= R[rs] - R[rt] (unchecked overflow)\n"
          "SUB    $rd,$rs,$rt   R[rd] <= R[rs] - R[rt]\n"
          "XOR    $rd,$rs,$rt   R[rd] <= R[rs] XOR R[rt]\n"
          "---------------------<I-type Instructions>-----------------------\n"
          "ADDIU  $rt,$rs,imm   R[rt] <= R[rs] + SignExtImm (unchecked overflow)\n"
          "ADDI   $rt,$rs,imm   R[rt] <= R[rs] + SignExtImm\n"
          "ANDI   $rt,$rs,imm   R[rt] <= R[rs] & ZeroExtImm\n"
          "BEQ    $rs,$rt,label PC <= (R[rs] == R[rt]) ? npc+BranchAddr : npc\n"
          "BNE    $rs,$rt,label PC <= (R[rs] != R[rt]) ? npc+BranchAddr : npc\n"
          "LUI    $rt,imm       R[rt] <= {imm,16b'0}\n"
          "LW     $rt,imm($rs)  R[rt] <= M[R[rs] + SignExtImm]\n"
          "ORI    $rt,$rs,imm   R[rt] <= R[rs] OR ZeroExtImm\n"
          "SLTI   $rt,$rs,imm   R[rt] <= (R[rs] < SignExtImm) ? 1 : 0\n"
          "SLTIU  $rt,$rs,imm   R[rt] <= (R[rs] < SignExtImm) ? 1 : 0\n"
          "SW     $rt,imm($rs)  M[R[rs] + SignExtImm] <= R[rt]\n"
          //"TSL    $rt,imm($rs)  R[rt] <= M[R[rs] + SignExtImm] <= 0xffffffff\n"
          "LL     $rt,imm($rs)  R[rt] <= M[R[rs] + SignExtImm]; rmwstate <= addr\n"
          "SC     $rt,imm($rs)  if (rmw) M[R[rs] + SignExtImm] <= R[rt], R[rt] <= 1 else R[rt] <= 0\n"
          "XORI   $rt,$rs,imm   R[rt] <= R[rs] XOR ZeroExtImm\n"
          "---------------------<J-type Instructions>-----------------------\n"
          "J      label         PC <= JumpAddr\n"
          "JAL    label         R[31] <= npc; PC <= JumpAddr\n"
          "---------------------<Other Instructions>------------------------\n"
          "HALT\n"
          "---------------------<Pseudo Instructions>-----------------------\n"
          "PUSH   $rs           $29 <= $29 - 4; Mem[$29+0] <= R[rs] (sub+sw)\n"
          "POP    $rt           R[rt] <= Mem[$29+0]; $29 <= $29 + 4 (add+lw)\n"
          "NOP                  Nop\n"
          "-----------------------------------------------------------------\n"
          "-----------------------------------------------------------------\n"
          "org  Addr         Set the base address for the code to follow \n"
          "chw  #            Assign value to half word memory\n"
          "cfw  #            Assign value to word of memory\n"
          );
}

