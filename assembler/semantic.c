#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "y.tab.h"
#include "common.h"
#include "semantic.h"
#include "parser.h"

#define SYM_TABLE_SIZE 2048

/* which compiler pass */
int32 pass;
/* line number of source file */
int32 lineno;
/* MIPS is 4 */
int32 addr_inc_amount;
/* what type of init file to output */
int32 output_type;
/* unknown */
int32 print_sym_table;

/* file pointer to output file */
FILE *output_file;
/* are we debugging */
//int debug;

/* common registers */
int32 zero_reg = 0;
int32 temp_reg = 1;
int32 stack_reg = 29;


/* current address */
static u_int32 current_addr = 0;
/* symbol table */
static SymEntry **symbol_table;

/* returns opcode name */
static char *getOpcodeName (u_int32 kop);
static int32 hash1 (char *string);
static int32 hash2 (char *string);

/* outputs code in hex format */
static void outputCode (u_int32 address, u_int32 instr, char *debug_str);

/* this should probably change to hex record */
#define S_RECORD_MAX_SIZE 512 /* was 100 */
u_int8 s_record[S_RECORD_MAX_SIZE];
u_int32 s_record_addr_prev;
u_int32 s_record_addr_base;
static void outputIntelHex (void);
static void outputIntelHexStart (void);
static void outputIntelHexStop (void);

/* method is called before asm program is parsed */
void start(void)
{
  /* initialize variables */
  /* set current address */
  current_addr = 0;
  /* set machine code address */
  s_record_addr_prev = 0;
  s_record_addr_base = 0;
  /* reset our machine code */
  memset (s_record, '\0', S_RECORD_MAX_SIZE);

  /* which compile pass are we on */
  if (pass == SYMBOL_PASS)
  {
    /* look for symbols */
    symTableInit ();
  }

  if (pass == MAIN_PASS)
  {
    /* output srecord */
    if (output_type == OUTPUT_INTELHEX)
    {
      outputIntelHexStart ();
    }
  }
}

/* called when asm file is through being parsed */
void finish(void)
{
  if (pass == MAIN_PASS)
  {
    /* kill symbol table this was the last pass */
    symTableDelete ();

    /* finish the machine code */
    if (output_type == OUTPUT_INTELHEX)
    {
      outputIntelHex ();
      outputIntelHexStop ();
    }
  }
}

/* process org to change addresses */
void processOrg (u_int32 addr)
{
  //ADAM - should check to see if addr is less than current_addr
  //       and throw an error
  if (addr < current_addr)
  {
    fprintf (stderr, "Line %d: Cannot ORGanize address to 0x%X overlapping address 0x%X\n",
        lineno, addr, current_addr);
    exit (ERROR);
  }
  current_addr = addr;
}

/* initialize memory word
 * to value */
void processWord (u_int32 val)
{
  /* do this for main pass */
  if (pass == MAIN_PASS)
  {
    /* put value in address and add comment */
    outputCode (current_addr, val, "CFW");
  }
  /* go to next address */
  current_addr += WORD_SIZE;
}

/* initialize memory byte to value */
/* just don't use this until you figure out
 * what is going on */
/* change fc* to chw or cfw */
void processHalfWord (u_int32 val)
{
  static int32 buffered;
  static u_int32 prev_addr;
  static u_int32 prev_val;

  /* init in main pass */
  if (pass == MAIN_PASS)
  {
    if (!buffered)
    {
      buffered = 1;
      prev_val = val;
      prev_addr = current_addr;
    }
    else
    {
      outputCode (prev_addr, (val&0xffff) << 16 | (prev_val & 0xffff), "CHW");
      prev_addr = 0;
      prev_val = 0;
      buffered = 0;
    }
  }
  current_addr += WORD_SIZE/2;
}

void checkAlignment ()
{
  if (current_addr % WORD_SIZE)
  {
    processHalfWord (0);
  }
}

/* process a label in asm file */
void processLabel (char *label)
{
  /* symbol table entry */
  SymEntry *entry;

  /* don't process new label on main pass */
  if (pass == MAIN_PASS)
  {
    return;
  }
  /* process labels on symbol pass */

  /* do we have a label? */
  if (label)
  {
    /* allocate new symbol table entry */
    entry = (SymEntry *)calloc (1, sizeof(SymEntry));

    /* give it name and address */
    entry->label = label;
    entry->address = current_addr;

    /* if we are printing symbol table
     * print out new entry */
    if (print_sym_table)
    {
      printf ("%s\t:  0x%04x\n", label, current_addr);
    }

    /* symbol table error */
    if (symTableEnter (entry) == ERROR)
    {
      fprintf (stderr, "Internal Error: Unable to enter (%s) in sym table\n",
              label);
      exit (ERROR);
    }
  }
}

/* look up a value in the symbol table */
u_int32 resolveLabel (char *label)
{
  /* symbol table entry */
  SymEntry *entry;

  /* look up the label */
  entry = symTableLookup (label);

  /* what is this for? */
  if (pass == SYMBOL_PASS)
  {
    /* ah this is for ldiw so
     * when you do ldiw $5, wee
     * and then do wee later in the
     * program there is a place holder that
     * allows ldiw to load upp and lower bytes
     * i think
     */
    return(0xFFFFFFFF); /* ensure LDIW loads upper and lower */
  }

  /* can't find symbol, they forgot to define it */
  if (entry == NULL)
  {
    fprintf (stderr, "Line %d: Label (%s) is not declared\n", lineno, label);
    exit (ERROR);
  }

  /* return address of symbol */
  return (entry->address);
}

/* not used */
/* should probably use int64 */
void checkInt (int32 num, int32 flag, int32 bits)
{
  /* check if int is bigger than number of bits */
  if ((num & ~(0xFFFFFFFF >> (32 - bits))) && (flag == UNSIGNED))
  {
    fprintf (stderr, "Line %d: Opcode param %d [0x%x] out of range (%d bits)\n",
            (lineno-1), num, num, bits);
    exit (ERROR);
  }
  /* check if int out of sign range */
  if (((num > ((0x1 << bits) - 1)) || (num < -(0x1 << bits))) && (flag == SIGNED))
  {
    fprintf (stderr, "Line %d: Opcode param %d [0x%x] out of sign range\n",
            (lineno-1), num, num);
    exit (ERROR);
  }
  /* int is ok */
}

/* generate an rtype instruction */
void genRType (u_int32 kop, u_int32 rd, u_int32 rs, u_int32 rt_shmt)
{
  /* instruction in 32 bits */
  u_int32 packed_opcode;
  /* instruction in written format */
  char debug_str[80];

  /* only do translate on main pass */
  if (pass != SYMBOL_PASS)
  {
    /* switch on op token */
    switch(kop)
    {
      case kNOP: /* i placed nop here -evillase */
        packed_opcode = formR (0, 0, 0, 0, 0, 0x00);
        break;
      case kSLL:
        packed_opcode = formR (0, rs, 0, rd, rt_shmt, 0x00);
        break;
      case kSRL:
        packed_opcode = formR (0, rs, 0, rd, rt_shmt, 0x02);
        break;
      case kSLLV:
        packed_opcode = formR(0, rs, rt_shmt, rd, 0, 0x04);
        break;
      case kSRLV:
        packed_opcode = formR(0, rs, rt_shmt, rd, 0, 0x06);
        break;
      case kJR:
        packed_opcode = formR (0, rs, 0, 0, 0, 0x08);
        break;
      case kADD:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x20);
        break;
      case kADDU:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x21);
        break;
      case kSUB:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x22);
        break;
      case kSUBU:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x23);
        break;
      case kAND:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x24);
        break;
      case kOR:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x25);
        break;
      case kXOR:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x26);
        break;
      case kNOR:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x27);
        break;
      case kSLT:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x2a);
        break;
      case kSLTU:
        packed_opcode = formR (0, rs, rt_shmt, rd, 0, 0x2b);
        break;
      default:
        printf ("Operand is %d\n", kop);
        assert ("Unknown opcode in genRType" == 0);
    }
    /* instruction in string format */
    switch (kop)
    {
      /* these are the only ones who use shamt */
      case kSLL:
      case kSRL:
      case kNOP:
        sprintf (debug_str, "%s $%d,$%d,%d",
            getOpcodeName (kop), rd, rs, rt_shmt);
        break;
      /* this is only one who uses just rs */
      case kJR:
        sprintf (debug_str, "%s $%d",
            getOpcodeName (kop), rs);
        break;
      default:
        sprintf (debug_str, "%s $%d,$%d,$%d",
            getOpcodeName (kop), rd, rs, rt_shmt);
        break;
    }
    /* output instruction */
    outputCode (current_addr, packed_opcode, debug_str);
  }
  /* increment address */
  current_addr += addr_inc_amount;
}

/* generate an itype instruction */
void genIType (u_int32 kop, u_int32 rt, u_int32 rs, u_int32 imm)
{
  /* instruction in 32 bits */
  u_int32 packed_opcode;
  /* beq and bne relative address offset */
  int32 offset = (imm - (current_addr + addr_inc_amount)) / addr_inc_amount;;
  /* instruction in written format */
  char debug_str[80];

  /* only generate on main pass */
  if (pass != SYMBOL_PASS)
  {
    /* switch on token op */
    switch (kop)
    {
        /* BEQ and BNE need to swap rs and rt as they are backwards
         * -ev
         */
      case kBEQ:
        packed_opcode = formI (0x04, rt, rs, offset);
        break;
      case kBNE:
        packed_opcode = formI (0x05, rt, rs, offset);
        break;
      case kBLEZ:
        packed_opcode = formI (0x06, rs, 0, offset);
        break;
      case kBGTZ:
        packed_opcode = formI (0x07, rs, 0, offset);
        break;
      case kADDI:
        packed_opcode = formI (0x08, rs, rt, imm);
        break;
      case kADDIU:
        packed_opcode = formI (0x09, rs, rt, imm);
        break;
      case kSLTI:
        packed_opcode = formI (0x0a, rs, rt, imm);
        break;
      case kSLTIU:
        packed_opcode = formI (0x0b, rs, rt, imm);
        break;
      case kANDI:
        packed_opcode = formI (0x0c, rs, rt, imm);
        break;
      case kORI:
        packed_opcode = formI (0x0d, rs, rt, imm);
        break;
      case kXORI:
        packed_opcode = formI (0x0e, rs, rt, imm);
        break;
      case kLUI:
        packed_opcode = formI (0x0f, 0, rt, imm);
        break;
      case kLW:
        packed_opcode = formI (0x23, rs, rt, imm);
        break;
      case kLBU:
        packed_opcode = formI (0x24, rs, rt, imm);
        break;
      case kLHU:
        packed_opcode = formI (0x25, rs, rt, imm);
        break;
      case kSB:
        packed_opcode = formI (0x28, rs, rt, imm);
        break;
      case kSH:
        packed_opcode = formI (0x29, rs, rt, imm);
        break;
      case kSW:
        packed_opcode = formI (0x2b, rs, rt, imm);
        break;
      case kTSL: /* test and set load  for locks */
        packed_opcode = formI (0x30, rs, rt, imm);
        break;
      case kLL: /* load linked used with store conditional */
        packed_opcode = formI (0x30, rs, rt, imm);
        break;
      case kSC: /* used with load linked for atomic rmw */
        packed_opcode = formI (0x38, rs, rt, imm);
        break;
      case kHALT: /* i placed halt here -evillase */
        packed_opcode = formI (0x3f, 0x1f, 0x1f, 0xffff);
        break;
      default:
        printf ("Operand is %d\n", kop);
        assert ("Unknown opcode in genIType" == 0);
    }
    /* write instruction in assembly */
    switch (kop)
    {
      case kBEQ:
      case kBNE:
        sprintf (debug_str, "%s $%d,$%d,%d",
            getOpcodeName (kop), rt, rs, offset*addr_inc_amount);
        break;
      case kLUI:
        sprintf (debug_str,"%s $%d,%d",
            getOpcodeName (kop), rt, (int32)imm);
        break;
      case kLW:
      case kLBU:
      case kLHU:
      case kSB:
      case kSH:
      case kSW:
      case kTSL:
        sprintf (debug_str, "%s $%d,%d($%d)",
            getOpcodeName (kop), rt, (int32)imm, rs);
        break;
      case kHALT:
        sprintf(debug_str,"%s", getOpcodeName (kop));
        break;
      default:
        sprintf (debug_str,"%s $%d,$%d,%d",
            getOpcodeName (kop), rt, rs, (int32)imm);
        break;
    }
    /* output generated instruction */
    outputCode (current_addr, packed_opcode, debug_str);
  }
  /* increment to next address space */
  current_addr += addr_inc_amount;
}


/* generate jtype instructions */
void genJType (u_int32 kop, u_int32 addr)
{
  /* FIXME i probably need to fix address translation */
  /* instruction in 32 bits */
  u_int32 packed_opcode;
  /* written instruction */
  char debug_str[80];

  /* only generate on main pass */
  if (pass != SYMBOL_PASS)
  {
    switch(kop)
    {
      case kJ:
        packed_opcode = formJ (0x02, ((addr >> 2) & 0x03ffffff));
        break;
      case kJAL:
        packed_opcode = formJ (0x03, ((addr >> 2) & 0x03ffffff));
        break;
      default:
        printf("Operand is %d\n", kop);
        assert("Unknown opcode in genJType" == 0);
    }
    /* write assembly for comments */
    switch (kop)
    {
      default:
        sprintf (debug_str,"%s %08X",
            getOpcodeName (kop), addr);
        break;
    }
    /* output generated instruction */
    outputCode (current_addr, packed_opcode, debug_str);
  }
  /* increment address */
  current_addr += addr_inc_amount;
}

/* psuedo op generate */
void genPush (u_int32 reg)
{
  /* hoops because no subiu */
  /* changed to addiu */
  /* ori to load inc_amount into temp */
  //genIType (kORI, temp_reg, zero_reg, addr_inc_amount);
  /* subtract to move stack pointer by inc_amount */
  //genRType (kSUBU, stack_reg, stack_reg, temp_reg);

  /* addiu to subtract word size */
  genIType (kADDIU, stack_reg, stack_reg, -1*addr_inc_amount);
  /* store to save value to stack */
  genIType (kSW, reg, stack_reg, 0);
}

/* psuedo op generate */
void genPop (u_int32 reg)
{
  /* load value from stack */
  genIType (kLW, reg, stack_reg, 0);
  /* increment stack by inc_amount */
  genIType (kADDIU, stack_reg, stack_reg, addr_inc_amount);
}

/* R-type
   opcode(6), rs(5), rt(5), rd(5), shamt(5), funct(6) */
u_int32 formR (u_int32 op,
           u_int32 rs,
           u_int32 rt,
           u_int32 rd,
           u_int32 shamt,
           u_int32 funct)
{
  u_int32 tmp = ((op & 0x3f) << 26 | /* opcode */
      (rs & 0x1f) << 21 | /* rs */
      (rt & 0x1f) << 16 | /* rt */
      (rd & 0x1f) << 11 | /* rd */
      (shamt & 0x1f) << 6 | /* shamt */
      (funct & 0x3f)); /* funct */

  /* return packed instruction */
  return tmp;
}

/* I-type
 * opcode(6), rs(5), rt(5), immediate(16) */
u_int32 formI (u_int32 op,
           u_int32 rs,
           u_int32 rt,
           u_int32 imm)
{
  u_int32 tmp = ((op & 0x3f) << 26 | /* opcode */
      (rs & 0x1f) << 21 | /* rs */
      (rt & 0x1f) << 16 | /* rt */
      (imm & 0xffff)); /* imm */

  /* return packed instruction */
  return tmp;
}

/* J-type
 * opcode(6), addr(26) */
u_int32 formJ (u_int32 op,
           u_int32 addr)
{
  u_int32 tmp = ((op & 0x3f) << 26 | /* opcode */
      (addr & 0x3ffffff)); /* addr */

  /* return packed instruction */
  return tmp;
}

/* output instruction to file */
/* msb -- lsb = big endian*/
/* easier to read */
static void outputCode (u_int32 address,
                        u_int32 instr,
                        char *debug_str)
{
  /* output for legacy ram file */
  if (output_type == OUTPUT_RAM_FILE)
  {
    /* put more info in file */
    if (debug)
    {
      fprintf (output_file, "%08x / %08x ;    %s\n", address, instr, debug_str);
    }
    else
    {
      fprintf (output_file, "%08x / %08x ;\n", address, instr);
    }
  }
  else if (output_type == OUTPUT_INTELHEX)
  {
    /* vars to compute checksum and addr */
    int i, j, checksum = 0;
    /* hex format address is 16 bits
     * also our memory model is 16 bits
     * but only allows under 30kbytes of storage
     */
    u_int16 addr = address / WORD_SIZE;
    char bytes[17];
    char byte[3];

    /* compute check sum */
    sprintf (bytes, "%02X%04X%02X%08X", 4, addr, 0, instr);
    for (i = 0; i < strlen(bytes)-1; i++)
    {
      byte[0] = bytes[i++];
      byte[1] = bytes[i];
      byte[2] = '\0';
      sscanf (byte, "%2X", &j);
      /* add fields 2 3 4 5 to other
       * sum each 2digit hex value
       */
      checksum += j;
    }

    /* two ways to do this
     * subtract sum from x100
     * or invert and add 1
     */
    /* keep only the least significant byte */
    checksum &= 0xff;
    /* invert the result */
    checksum = ~checksum;
    /* add 1 to get check sum */
    checksum += 1;
    /* again lsb is what we want */
    checksum &= 0xff;

    /* six fields for intel hex format
     * 1 - :
     * 2 - byte count - 04
     * 3 - address 16bits
     * 4 - record type - 00
     * 5 - data - 32bits
     * 6 - checksum - ???
     */
    /* hex format */
    if (debug) {
      fprintf (output_file, ":%02X%04X%02X%08X%02X #%s\n", 4, addr, 0, instr, checksum, debug_str);
    }
    else {
      fprintf (output_file, ":%02X%04X%02X%08X%02X\n", 4, addr, 0, instr, checksum);
    }
  }
}

/* output intel hex format */
/* no longer used */
static void outputIntelHex (void)
{

  int32 i;
  int32 tmp_int;
  u_int8 check_sum;
  u_int8 len;

  char tmp_str[3];

  if (strlen(s_record) == 0)
  {
    return;
  }

  len = (strlen(s_record) >> 1) + 3;

  check_sum = 0;
  tmp_str[2] = '\0';
  for (i = 0; i < strlen(s_record); i++)
  {
    tmp_str[0] = s_record[i++];
    tmp_str[1] = s_record[i];
    sscanf (tmp_str, "%x", &tmp_int);
    check_sum += tmp_int;
  }

  check_sum += len;
  check_sum += (s_record_addr_base & 0xff00) >> 8;
  check_sum += (s_record_addr_base & 0xff);

  check_sum = ~check_sum;

   /*
   printf("S1 len=%02x addr=%04x [%s] checksum=%02x\n",len,
      s_record_addr_base,s_record,check_sum);
   */

  fprintf (output_file, "S1%02X%04X%s%02X\n", len, s_record_addr_base,
          s_record, check_sum);

  /* zero out memory */
  memset (s_record,'\0', S_RECORD_MAX_SIZE);
}

/* start to output intel hex */
static void outputIntelHexStart (void)
{
  /* header here */
}

/* stop outputting intel hex */
static void outputIntelHexStop (void)
{
  /* footer here */
  fprintf (output_file, ":00000001FF\n");
}

/* initialize symbol table */
u_int32 symTableInit (void)
{
  /* allocate memory for table */
  symbol_table = (SymEntry **)calloc(SYM_TABLE_SIZE, sizeof(SymEntry*));

  /* check if we got virtual memory */
  if (!symbol_table)
  {
    fprintf (stderr, "Calloc failed creating symbol table\n");
    exit (ERROR);
  }

  /* initialize table to null */
  memset (symbol_table, '\0', (SYM_TABLE_SIZE * sizeof(SymEntry*)));

  /* return status */
  return (SUCCESS);
}

/* delete symbol table */
void symTableDelete (void)
{
  /* counter */
  int32 i;

  /* go through each entry in table */
  for (i = 0; i < SYM_TABLE_SIZE; i++)
  {
    /* if it has been used */
    if (symbol_table[i] != NULL)
    {
      /* free the memory */
      free (symbol_table[i]);
      /* set value to null */
      symbol_table[i] = NULL;
    }
  }
  /* free memory for table */
  free (symbol_table);
  /* set table to null */
  symbol_table = NULL;
}

/* insert symbol into table */
int32 symTableEnter (SymEntry *entry)
{
  /* table key and loop counter */
  int32 key,i;

  /* check if we have symbol in table */
  if (symTableLookup (entry->label))
  {
    /* can't have more than one symbol that is the same */
    fprintf (stderr, "Line %d: Label (%s) is not unique\n", lineno, entry->label);
    exit (ERROR);
  }

  /* hash function to find symbol a home */
  for (i = 0; i < SYM_TABLE_SIZE; i++)
  {
    /* generate a hash key double hashed
     * no clustering
     */
    key = (hash1 (entry->label) + i * hash2 (entry->label)) % SYM_TABLE_SIZE;
    /* is the location free ? */
    if (symbol_table[key] == NULL)
    {
      /* found a blank spot */
      //printf("hashing (%s) at (%d) i=%d\n",entry->label,key,i);
      /* add symbol to table */
      symbol_table[key] = entry;
      /* return hash key */
      return (key);
    }
  }
  /* no room, throw error */
  return (ERROR);
}

/* look up a symbol in the table */
SymEntry *symTableLookup (char *label)
{
  /* key and counter for hash */
  int32 i,key;

  /* loop through possible locations */
  for (i = 0; i < SYM_TABLE_SIZE; i++)
  {
    /* get symbol hash key */
    key = (hash1 (label) + i * hash2 (label)) % SYM_TABLE_SIZE;
    //printf("Lookup up (%s) at (%d)\n",label,key);
    /* symbol not here */
    if (symbol_table[key] == NULL)
    {
      /* if its not here its not anywhere else
       * so break we are done */
      break;
    }
    /* do we have a match */
    if (!strcmp (symbol_table[key]->label, label))
    {
      /* return symbol address */
      return (symbol_table[key]);
    }
  }
  /* symbol is not in the table */
  return(NULL);
}

/* hash function for symbol */
static int32 hash1 (char *string)
{
  /* can't enter a null symbol */
  if (string == NULL)
  {
    fprintf (stderr, "Attempt to enter NULL string into symbol table\n");
    exit (ERROR);
  }
  /* generate hash from symbol name */
  return ((string[0] + string[strlen(string)]) % SYM_TABLE_SIZE);
}

/* next hash function to complete double hash */
static int32 hash2 (char *string)
{
  /* hash key */
  int32 key;

  /* can't enter null into table */
  if (string == NULL)
  {
    fprintf (stderr, "Attempt to enter NULL string into symbol table\n");
    exit (ERROR);
  }
  /* generate hash from symbol name, different function */
  key = 1 + ((string[0] + string[(strlen(string)>>1) + string[strlen(string)]])
             % (SYM_TABLE_SIZE-1));
  return (key);
}

/* translate opcode token to string */
static char *getOpcodeName (u_int32 kop)
{
  switch (kop)
  {
  case kADD:
    return ("ADD   ");
  case kADDI:
    return ("ADDI  ");
  case kADDIU:
    return ("ADDIU ");
  case kADDU:
    return ("ADDU  ");
  case kAND:
    return ("AND   ");
  case kANDI:
    return ("ANDI  ");
  case kBEQ:
    return ("BEQ   ");
  case kBNE:
    return ("BNE   ");
  case kJ:
    return ("J     ");
  case kJAL:
    return ("JAL   ");
  case kJR:
    return ("JR    ");
  case kLBU:
    return ("LBU   ");
  case kLHU:
    return ("LHU   ");
  case kLUI:
    return ("LUI   ");
  case kLW:
    return ("LW    ");
  case kNOR:
    return ("NOR   ");
  case kOR:
    return ("OR    ");
  case kORI:
    return ("ORI   ");
  case kSLT:
    return ("SLT   ");
  case kSLTI:
    return ("SLTI  ");
  case kSLTIU:
    return ("SLTIU ");
  case kSLTU:
    return ("SLTU  ");
  case kSLL:
    return ("SLL   ");
  case kSRL:
    return ("SRL   ");
  case kSLLV:
    return ("SLLV   ");
  case kSRLV:
    return ("SRLV   ");
  case kSB:
    return ("SB    ");
  case kSH:
    return ("SH    ");
  case kSW:
    return ("SW    ");
  case kSUB:
    return ("SUB   ");
  case kSUBU:
    return ("SUBU  ");
  case kTSL:
    return ("TSL   ");
  case kLL:
    return ("LL    ");
  case kSC:
    return ("SC    ");
  case kXOR:
    return ("XOR   ");
  case kXORI:
    return ("XORI  ");
  case kHALT:
    return ("HALT  ");
  case kNOP:
    return ("SLL   ");
  default:
    return ("UNKNOWN");
  }
}

