/****************************************************************/
/* LZ77 data compression, main module				*/
/* Copyright (c) 1994 by XelaSoft				*/
/* version history:						*/
/*   version 0.9, start date: 11-25-1994			*/
/****************************************************************/
#define DIVHEADER
#ifndef __MSDOS__
#include "machine.h"
#endif

#define spd_tst

#include <stdio.h>

#ifndef UNIX_C
#include <process.h>
#include <io.h>
#endif

#ifdef __MSDOS__
#include <model.h>
#include <alloc.h>
#include <mem.h>
#include <dir.h>
#ifdef spd_tst
#include <time.h>
#endif
#else
#define O_BINARY 0
#include <malloc.h>
#include <memory.h>
#endif
#include <fcntl.h>

#ifdef MSX_C
#include <tmem.h>
#endif

#include "pcmsx.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifdef MSX_C
#ifdef spd_tst
#define jiffy (*((unsigned *)0xfc9e))
#endif
#pragma nonrec
#else
#define recursive
#endif

#ifdef MSX_C
#pragma nonrec
#endif

#include "lz77out.h"
#include "lz77core.h"

/****************************************************************/
/* some usefull macro's, etc					*/
/****************************************************************/
#define maxstrlen (254)

/****************************************************************/
/* The function prototypes					*/
/****************************************************************/
void main();     /* the main procedure                    */
void mkheader(); /* maak de header                        */
void savelens(); /* save the org len and dest len         */
TINY *zmalloc(); /* get memory and check for null ptr     */
void explain();  /* explain the usage of the program      */
void error();    /* give an error message and abort       */

/****************************************************************/
/* de hoofdlus							*/
/****************************************************************/
void main(paramcnt, paramval)
int paramcnt;
char *paramval[];
{
#ifdef spd_tst
#ifdef MSX_C
  unsigned tijdsduur;
#endif
#ifdef __MSDOS__
  clock_t tijdsduur;
#endif
#endif
#ifdef __MSDOS__
  printf("XelaSoft Compression 1.0 for PC\n");
#endif
#ifdef UNIX_C
  printf("XelaSoft Compression 1.0 for UNIX\n");
#endif
#ifdef MSX_C
  printf("XelaSoft Compression 1.0 for MSX\n");
#endif
  printf("Copyright (c) 1995 XelaSoft\n\n");

  if (paramcnt != 3)
    explain();

  if (inopen(paramval[1], O_RDONLY | O_BINARY) != ERROR) {
    if (bitcreat(paramval[2]) != ERROR) {

#ifdef spd_tst
#ifdef MSX_C
      jiffy = 0;
#endif
#endif
      mkheader((TINY *)paramval[1]);  /* maak de header      */
      lz77();                         /* crunch de data echt */
      flushout();
      savelens(inglen(), bitglen()); /* put the lens in the header */
#ifdef spd_tst
#ifdef MSX_C
      tijdsduur = jiffy;
      printf("\ntijd: %5d ints (50 of 60 Hz)\n", tijdsduur);
#endif
#endif
      bitclose();
#ifdef UNIX_C
      fprintf(stderr, "\n");
#endif
    }
    else
      printf("error creating %s\n", paramval[2]);
    inclose();
  }
  else
    printf("error opening %s\n", paramval[1]);
  freebufs();  /* geef alle buffers vrij */
}

/****************************************************************/
/* maak de header aan						*/
/****************************************************************/
void mkheader(name)
TINY *name;
{
  unsigned temp;
  long orglen;
#ifndef UNIX_C
  TINY path[MAXPATH];
  TINY drive[MAXDRIVE];
  TINY dir[MAXDIR];
  TINY fname[MAXFILE];
  TINY ext[MAXEXT];
#endif

  fileout('P');
  fileout('C');
  fileout('K');
  fileout((TINY)8);  /* LZ77 met codering 8 */

  /* reserve space for org and dest length */
  for (temp = 0; temp != 8; temp++)
    fileout((TINY)0);

#ifndef UNIX_C
  fnsplit(name, drive, dir, fname, ext);
  fnmerge(path, "", dir, fname, ext);
  name = path;
#endif
  do
    fileout((temp = *(name++)));
  while (temp);
}

/****************************************************************/
/* put the org len and the dest len in the header		*/
/****************************************************************/
void savelens(orglen, destlen)
unsigned long orglen, destlen;
{
  unsigned cnt;

  bitspos((long)4);
  for (cnt = 0; cnt != 4; cnt++) {
    fileout((TINY)orglen);
    orglen >>= 8;
  }
  for (cnt = 0; cnt != 4; cnt++) {
    fileout((TINY)destlen);
    destlen >>= 8;
  }
}

/****************************************************************/
/* get memory and check for null ptr				*/
/****************************************************************/
TINY *zmalloc(nrbytes)
unsigned nrbytes;
{
  TINY *temp;

  if (!(temp = (TINY *)malloc(nrbytes)))
    error("Out of memory\n"); /* could not get the memory */

  return temp;  /* and return the memory pointer */
}

/****************************************************************/
/* explain the usage of the program and abort			*/
/****************************************************************/
void explain()
{
  printf("Usage:\n");
  printf(">xsc infile outfile\n\n");
  printf("The file named infile will be compressed to the file named outfile.xsa\n\n");
  printf("Note: the compression format is compatible with d2f's and f2d's format\n");
  exit(128);
}

/****************************************************************/
/* give an error message and abort				*/
/****************************************************************/
void error(string)
char *string;
{
  printf(string);
  exit(128);
}
