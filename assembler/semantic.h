#ifndef SYMANTIC_H
#define SYMANTIC_H

/* structure of entry in symbol table */
typedef struct _SymEntry {
  /* entry name */
   char *label;
   /* entry location */
   u_int32  address;
} SymEntry;

/* types of files we can generate */
enum OutputType
{
   OUTPUT_RAM_FILE = 1, /* legacy for flex10k board and memory */
   OUTPUT_INTELHEX,     /* for cycloneii board and memory */
   OUTPUT_MIF           /* incase we need a third type */
};


/* is number signed or unsigned */
#define SIGNED      1
#define UNSIGNED    2


/* start compilation */
void start (void);
/* finish compilation */
void finish (void);

/* change address to new value */
void processOrg (u_int32);
/* set memory location to value */
void processWord (u_int32);
/* set memory location to value */
void processHalfWord (u_int32);
/* check address aligment */
void checkAlignment ();

/* ?? */
void processLabel (char *);
u_int32 resolveLabel (char *);
/* check if int is in size and sign range */
void checkInt (int32 num,int32 flag, int32 bits);

/* rtype instructions */
void genRType (u_int32 kop, u_int32 rd, u_int32 rs, u_int32 rt_shmt);

/* itype instructions */
void genIType (u_int32 kop, u_int32 rt, u_int32 rs, u_int32 imm);

/* jtype instructions */
void genJType (u_int32 kop, u_int32 addr);

/* psuedo instructions */
void genPush (u_int32);
void genPop (u_int32);

/* forms */
u_int32 formR (u_int32 op, u_int32 rs, u_int32 rt, u_int32 rd, u_int32 shamt, u_int32 funct);
u_int32 formI (u_int32 op, u_int32 rs, u_int32 rt, u_int32 imm);
u_int32 formJ (u_int32 op, u_int32 addr);


/* initialize symbol table */
u_int32 symTableInit (void);
/* delete symbol table */
void symTableDelete (void);
/* add entry in table */
int32 symTableEnter (SymEntry *);
/* lookup entry in table */
SymEntry *symTableLookup (char *);

#endif /* SYMANTIC_H */
