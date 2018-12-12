/****************************************************************/
/* De lz77 uitvoer module					*/
/* Copyright (c) 1995 XelaSoft					*/
/* 28-1-1995, Alex Wulms					*/
/*            Pelikaanhof 127 c					*/
/*            2312 EG Leiden					*/
/*            071-124846					*/
/****************************************************************/
#define DIVHEADER
#ifndef __MSDOS__
#include "machine.h"
#endif

/* #define alt_out */
/* #define shwh_tst */
#define mbitout

#include <stdio.h>
#ifndef UNIX_C
#include <io.h>
#endif
#include <fcntl.h>

#ifndef __MSDOS__
#define O_BINARY (0)
#endif

#ifdef __MSDOS__
#include <model.h>
#include <alloc.h>
#include <dir.h>
#else
#include <malloc.h>
#endif


#include "pcmsx.h"
#include "lz77tdef.h"
#include "lz77huf.h"

#ifdef MSX_C
#pragma nonrec
#include <lz77outm.h>
#else
#define recursive
#endif

/****************************************************************/
/* some usefull macro's, etc					*/
/****************************************************************/
#ifdef MSX_C
#define obufsize 1024
#else
#define obufsize 32768
#endif
typedef FD BD;  /* A bit descripter is a file descriptor */
#ifdef mbitout
#define bitout(n) { if (!setflg) bitout2();\
                    if (n) bitflg |= setflg;\
                    setflg <<= 1; }
#endif

/****************************************************************/
/* global vars							*/
/****************************************************************/
static unsigned bytecnt;     /* #bytes in the byte output buffer */
static TINY     *bytebuf;    /* the byte + bitflg output buffer  */
static unsigned bitflgcnt;   /* the current bitflg pos in buffer */
static TINY     bitflg;      /* the current bitflg               */
static TINY     setflg;      /* the bit which has to be set      */
static FD	bitfile;     /* FD of the bitstream file         */
#ifdef __MSDOS__
static long nrwritten;
#else
static unsigned nrwritten;
#endif

typedef struct huf_code {
  SHORT nrbits;
  unsigned bitcode;
} huf_code;

static huf_code hufcodtbl[tblsize];

/****************************************************************/
/* The function prototypes					*/
/****************************************************************/
void strout();   /* send a string to the output           */
void charout();  /* send a char to the output             */
void cnvtohuffcodes();/* zet de boom om in huffman codes  */
#ifdef shwh_tst
void shwhuffcodes();/* show the huffman codes             */
#endif
TINY findmsb();  /* vind hoogste bit van een getal        */
BD   bitcreat(); /* create the bitstream file             */
unsigned long bitglen(); /* get length of a bitstream file*/
void bitspos();  /* go to a position in a bitstrem file   */
void bitclose(); /* close the bitstream file              */
void byteout();  /* send a byte to the bitstream          */
#ifndef mbitout
void bitout();   /* send a bit to the bitstream           */
#else
void bitout2();  /* send a bit to the bitstream           */
#endif
void flushout(); /* flush the bytebuffer                  */
void fileout();  /* send a byte to the output file        */

/****************************************************************/
/* stuur een string code naar de uitvoer			*/
/* opm: als strlen == maxstrlen+1, hoeft de strpos niet		*/
/*      uitgevoerd te worden want dat is de EOF code		*/
/****************************************************************/
void strout(string, strlen, strpos)
TINY *string;
TINY strlen;
unsigned strpos;
{
  TINY mskflg, temp;
  unsigned masklong;
  TINY count;
#ifdef alt_out
  TINY orgstrlen = strlen;
  unsigned orgstrpos = strpos;
#endif

  if (strlen==1)
    charout(string[0]);
  else {
    /* 1 ==> is strcode: strlen, strpos */
    bitout((TINY)1);

    if ((--strlen) <= 3) {
      /* org len bitcode */
      /* 4       110     */
      /* 3       10      */
      /* 2       0       */
      while(--strlen)
	bitout((TINY)1);
      bitout((TINY)0);
    }
    else {
      /* org len bitcode                */
      /*   >4    111x..xy lencode       */
      /* met x..xy = 1..10, z.d.d.      */
      /*      #(1..1) = strlnrbits-2-1, */
      /*     lencode = strlen-1 zonder  */
      /*      het hoogste bit           */
      bitout((TINY)1);
      bitout((TINY)1);
      bitout((TINY)1);

      temp = (mskflg = findmsb((TINY)strlen)) >> 2;
      while ((temp >>= 1))
	bitout((TINY)1);
      if (strlen < 128)
	/* terminating 0 alleen bij 0..4 bits   */
	/* 5 bits is per definitie 128 en hoger */
	bitout((TINY)0);

      /* output de laagste bits van strlen      */
      while ((mskflg >>= 1))
	bitout((TINY)strlen & mskflg);
    }

    if (strlen != maxstrlen) {
      /* Zet nu nog de strpos achter de code aan. */
      /* Gebruik een variabele lengte notatie     */
      /* waarbij een kleine strpos minder bits    */
      /* gebruikt dan een grote strpos.           */
      for (count = 0; strpos >= cpdist[count+1]; count++)
	;
      tblsizes[count]++;
      masklong = 1<<(hufcodtbl[count].nrbits-1);
#ifdef alt_out
      putchar('.');
#endif
      while (masklong) {
#ifdef alt_out
	putchar('0'+!!(hufcodtbl[count].bitcode & masklong));
#endif
	bitout(hufcodtbl[count].bitcode & masklong);
	masklong >>= 1;
      }
#ifdef alt_out
      printf(".%d ", count);
#endif
      strpos -= cpdist[count];
      if (cpdbmask[count] >= 256) {
	byteout((TINY)strpos);
	strpos >>= 8;
	temp = (TINY)(cpdbmask[count]>>8);
      }
      else
	temp = (TINY)cpdbmask[count];
      while ((temp >>= 1))
	bitout((TINY)strpos & temp);
    }
    if (!(updhufcnt--)) {
      mkhuftbl(); /* make the huffman table */
      cnvtohuffcodes(huftbl+2*tblsize-2, 0, 0); /* conv into bitstream info */
    }
  }
#ifdef alt_out
    printf("(%u,%u) ", orgstrpos, orgstrlen);
#endif
}

/****************************************************************/
/* stuur een karakter naar de uitvoer				*/
/****************************************************************/
void charout(ch)
TINY ch;
{
#ifdef alt_out
  printf("#%u ", (unsigned)ch);
#endif
  /* 0 ==> is een char: character */
  bitout((TINY)0);
  byteout(ch);
}

/****************************************************************/
/* convert to huffman codes					*/
/****************************************************************/
recursive void cnvtohuffcodes(nodeptr, bitcode, nrbits)
huf_node *nodeptr;
unsigned bitcode;
SHORT nrbits;
{
  if (!(nodeptr->child1)) {
    unsigned node=nodeptr-huftbl;
    hufcodtbl[node].nrbits = nrbits;
    hufcodtbl[node].bitcode = bitcode;
  }
  else {
    cnvtohuffcodes(nodeptr->child1, bitcode<<=1, ++nrbits);
    cnvtohuffcodes(nodeptr->child2, 1+bitcode, nrbits);
  }
}

/****************************************************************/
/* show the huffman codes					*/
/****************************************************************/
#ifdef shwh_tst
void shwhuffcodes()
{
  unsigned count;

  printf("huffman codes:\ncnt, len, code\n");
  for (count = 0; count != tblsize; count++)
    printf("%3u, %3u, %u\n",
	   count, hufcodtbl[count].nrbits,
	   hufcodtbl[count].bitcode);
}
#endif

/****************************************************************/
/* vind het hoogste bit van een getal				*/
/****************************************************************/
TINY findmsb(n)
TINY n;
{
  TINY mskflg = 128;

  while (!(n & mskflg) && mskflg)
    mskflg >>= 1;

  return mskflg;
}

/****************************************************************/
/* create the bitstream file					*/
/* out: BD if opened succesfully				*/
/*      ERROR if not opened succesfully				*/
/****************************************************************/
BD bitcreat(name)
char *name;
{
#ifdef __MSDOS__
  TINY path[MAXPATH];
  TINY drive[MAXDRIVE];
  TINY dir[MAXDIR];
  TINY fname[MAXFILE];
  TINY ext[MAXEXT];
#endif
#ifdef UNIX_C
  TINY namebuf[300];
#endif

  if (!(bytebuf = (TINY *)malloc(obufsize))) {
    printf("Out of memory\n");
    return ERROR;  /* not enough memory for the output buffer */
  }

#ifdef MSX_C
  if ((bitfile = creat(name)) == ERROR) {
#endif
#ifdef __MSDOS__
  fnsplit(name, drive, dir, fname, ext);
  fnmerge(path, drive, dir, fname, ".XSA");
  if ((bitfile = open(path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
		      0400 | 0200 | 0040 | 0004)) == ERROR) {
#endif
#ifdef UNIX_C
  strcpy(namebuf, name);
  strcat(namebuf, ".xsa");
  if ((bitfile = open(namebuf, O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
		      0400 | 0200 | 0040 | 0004)) == ERROR) {
#endif
    free(bytebuf);
    return ERROR;
  }
  nrwritten = 0; /* nothing written yet */
  bitflg = 0;    /* reset the bitflag                            */
  bitflgcnt = 0; /* bitflag can be placed in beginning of buffer */
  bytecnt = 1;   /* first byte can be placed at addr 1           */
  setflg = 1;
  inithufinfo(); /* initialize the cpdist tables */
  cnvtohuffcodes(huftbl+2*tblsize-2, 0, 0); /* conv into bitstream info */

  return (BD)bitfile;
}

/****************************************************************/
/* get length of the bitstream file				*/
/****************************************************************/
unsigned long bitglen()
{
  return (long)nrwritten;
}

/****************************************************************/
/* go to a position in the bitstream file			*/
/****************************************************************/
void bitspos(newpos)
unsigned long newpos;
{
  lseek(bitfile, newpos, SEEK_SET);
}

/****************************************************************/
/* close the bitstream file					*/
/****************************************************************/
void bitclose()
{
  free(bytebuf);  /* free the bit & byte buffer  */
  close(bitfile); /* and close the file          */
}

/****************************************************************/
/* Send a byte to the output					*/
/****************************************************************/
void byteout(n)
TINY n;
{
  bytebuf[bytecnt++] = n;
}

/****************************************************************/
/* Send a bit to the file					*/
/****************************************************************/
#ifndef mbitout
void bitout(n)
TINY n;
{
  if (!setflg) {
    if (bytecnt >= (obufsize-9))
      flushout();           /* flush the bit and byte buffer        */
    else
      bytebuf[bitflgcnt] = bitflg; /* store the bitflag */
    bitflgcnt = bytecnt++;  /* bitflag may be placed at current pos */
    bitflg = 0;             /* reset the bitflag                    */
    setflg = 1;
  }
  if (n)
    bitflg |= setflg;
  setflg <<= 1;
}

/****************************************************************/
/* Send a bit to the file					*/
/****************************************************************/
#else
void bitout2()
{
  if (bytecnt >= (obufsize-9))
    flushout();           /* flush the bit and byte buffer        */
  else
    bytebuf[bitflgcnt] = bitflg; /* store the bitflag */
  bitflgcnt = bytecnt++;  /* bitflag may be placed at current pos */
  bitflg = 0;             /* reset the bitflag                    */
  setflg = 1;
}
#endif

/****************************************************************/
/* flush the byte & bitbuffer					*/
/****************************************************************/
void flushout()
{
  bytebuf[bitflgcnt] = bitflg; /* store the last bitflag */
  write(bitfile, bytebuf, bytecnt);
  nrwritten += bytecnt;
  bytecnt = 0;
}

/****************************************************************/
/* stuur 1 byte naar de uitvoer file				*/
/****************************************************************/
void fileout(n)
TINY n;
{
  write(bitfile, &n, 1);
  nrwritten++;
}
