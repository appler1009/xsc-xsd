/****************************************************************/
/* LZ77 data decompression					*/
/* Copyright (c) 1994 by XelaSoft				*/
/* version history:						*/
/*   version 0.9, start date: 11-27-1994			*/
/****************************************************************/
#define DIVHEADER
#ifndef __MSDOS__
#include "machine.h"
#endif

/* #define pos_tst */
/* #define hoho */

#include <stdio.h>
#include <fcntl.h>
#ifdef __MSDOS__
#include <alloc.h>
#include <mem.h>
#include <model.h>
#else
#ifndef __clang__
#include <malloc.h>
#endif
#include <memory.h>
#define O_BINARY (0)
#endif

#ifndef UNIX_C
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "pcmsx.h"
#include "lz77tdef.h"
#include "lz77huf.h"

#ifdef MSX_C
#pragma nonrec
#endif

/****************************************************************/
/* some defines for fucking MSX linker that only knows 6 chars  */
/****************************************************************/
#define outbuf outbb1
#define outbufpos outbb2
#define outbufcnt outbb3

/****************************************************************/
/* some usefull macro's, etc					*/
/****************************************************************/
#define slwinsize (1 << slwinsnrbits)
#define outbufsize (slwinsize+4096)
#define inbufsize (32768)

/****************************************************************/
/* global vars							*/
/****************************************************************/
static TINY *inbuf;        /* the input buffer     */
static TINY *inbufpos;     /* pos in input buffer  */
static unsigned inbufcnt;  /* #elements in the input buffer */
static FD infile;          /* the input file        */

static TINY *outbuf;       /* the output buffer     */
static TINY *outbufpos;    /* pos in output buffer  */
static unsigned outbufcnt; /* #elements in the output buffer */
static FD outfile;         /* the output file       */

static TINY orglen[4];     /* lengte ongecrunchte file */
static TINY pcklen[4];     /* lengte gecrunchte file */

#ifdef __MSDOS__
static unsigned long nrread; /* #gelezen bytes        */
#else
static unsigned nrread;    /* #gelezen bytes        */
#endif

static TINY bitflg;  /* flag with the bits */
static TINY bitcnt;  /* #resterende bits   */

/****************************************************************/
/* The function prototypes					*/
/****************************************************************/
void main();         /* the main procedure                   */
FD   inopen();       /* open the input file                  */
int chkheader();     /* controleer de fileheader             */
void unlz77();       /* perform the real decompression       */
void explain();      /* explain the usage of the program     */
void error();        /* give an error message and abort      */
void charout();      /* put a character in the output stream */
void flushoutbuf();  /* flush the output buffer              */
unsigned rdstrlen(); /* read string length                   */
unsigned rdstrpos(); /* read string pos                      */
TINY charin();       /* read a char                          */
unsigned readnum();  /* read a number from the input stream  */
TINY bitin();        /* read a bit                           */
#ifdef hoho
TINY chinhoho();
#endif

/****************************************************************/
/* de hoofdlus							*/
/****************************************************************/
void main(paramcnt, paramval)
int paramcnt;
char *paramval[];
{
#ifdef __MSDOS__
  printf("XelaSoft Decompression 1.0 for PC\n");
#endif
#ifdef UNIX_C
  printf("XelaSoft Decompression 1.0 for UNIX\n");
#endif
#ifdef MSX_C
  printf("XelaSoft Decompression 1.0 for MSX\n");
#endif
  printf("Copyright (c) 1995 XelaSoft\n\n");

  if (paramcnt != 3)
    explain();
  if (!(outbuf = (TINY *)malloc(outbufsize)))
    error("Not enough memory\n");
  if (!(inbuf = (TINY *)malloc(inbufsize)))
    error("Not enough memory\n");

  inithufinfo(); /* initialize the cpdist tables */

  infile = inopen((TINY *)paramval[1], O_RDWR | O_BINARY);
#ifdef MSX_C
  if ((outfile = creat(paramval[2])) != ERROR) {
#else
  if ((outfile = open(paramval[2],
		      O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
		      0400 | 0200 | 0040 | 0004)) != ERROR) {
#endif
    inbufcnt = 0;       /* er zit nog niks in de input buffer  */
    nrread   = 0;       /* nog niks ingelezen                  */

    outbufcnt = 0;      /* er zit nog niks in de output buffer */
    outbufpos = outbuf; /* dadelijk vooraan in laden           */
    bitcnt = 0;         /* nog geen bits gelezen               */
    if (chkheader() != ERROR) {
      unlz77();         /* decrunch de data echt               */
#ifndef MSX_C
      fprintf(stderr, "\n");
#endif
    }
    else
      printf("Wrong header structure\n");
    close(outfile);
  }
  else
    printf("Error creating %s\n", paramval[2]);
  close(infile);
  free(outbuf);
}

/****************************************************************/
/* Open the input file                                          */
/****************************************************************/
FD inopen(name, flags)
TINY *name;
int flags;
{
  FD file;
#ifndef UNIX_C
  TINY path[MAXPATH];
  TINY drive[MAXDRIVE];
  TINY dir[MAXDIR];
  TINY fname[MAXFILE];
  TINY ext[MAXEXT];

  fnsplitname, drive, dir, fname, ext);
  fnmerge(path, drive, dir, fname, ".XSA");
  name = path;
#else
  TINY namebuf[300];

  strcpy(namebuf, name);
  strcat(namebuf, ".xsa");
  name = namebuf;
#endif

  if ((file = open(name, flags)) == ERROR) {
    printf("Error opening %s\n", name);
    exit(128);
  }
  return file;
}

/****************************************************************/
/* controleer de fileheader             			*/
/****************************************************************/
int chkheader()
{
  TINY temp;
  TINY buf[4];

#ifdef hoho
  return ERROR+1;
#else
  if (read(infile, buf, 4) != 4)
    return ERROR;

  for (temp = 0; (temp != 4) && (buf[temp] == "PCK\010"[temp]); temp++)
    ;
  if (temp != 4)
    return ERROR;

  if (read(infile, orglen, 4) != 4)
    return ERROR;

  if (read(infile, pcklen, 4) != 4)
    return ERROR;

  printf("Original filename: ");
  while ((temp = read(infile, buf, 1) ? buf[0]: 0))
    putchar((char)temp);
  putchar('\n');

  return ERROR+1;
#endif
}

/****************************************************************/
/* the actual decompression algorithm itself			*/
/****************************************************************/
void unlz77()
{
  TINY strlen = 0;
  unsigned strpos;
#ifdef pos_tst
  TINY temp;
#endif
#ifdef hoho
  TINY temp, temp2;
#endif

  do {
#ifndef hoho
    switch (bitin()) {
#ifdef pos_tst
      case 0 : temp = charin();
	       printf("#%u ", (unsigned)temp);
	       charout(temp); break;
#else
      case 0 : charout(charin()); break;
#endif
      case 1 : strlen = rdstrlen();
	       if (strlen == (maxstrlen+1))
		 break;
	       strpos = rdstrpos();
#ifdef pos_tst
	       printf("(%u,%u) ", strpos, (unsigned)strlen);
#endif
	       while (strlen--)
		 charout(*(outbufpos-strpos));
	       strlen = 0;
	       break;
    }
#else
    switch (chinhoho()) {
      case '#' : temp2 = 0;
		 while (((temp = chinhoho()) >= '0') && (temp <= '9'))
		   temp2 = 10*temp2 + temp-'0';
		 charout(temp2);
		 break;
      case '(' : strpos = 0;
		 while (((temp = chinhoho()) >= '0') && (temp <= '9'))
		   strpos = 10*strpos + (unsigned)(temp-'0');
		 strlen = 0;
		 while (((temp = chinhoho()) >= '0') && (temp <= '9'))
		   strlen = 10*strlen + temp-'0';
		 while (strlen--)
		   charout(*(outbufpos-strpos));
		 strlen = 0;
		 break;
      case 255 : strlen = maxstrlen+1;
		 break;
    }
#endif
  }
  while (strlen != (maxstrlen+1));
  flushoutbuf();
}

/****************************************************************/
/* explain the usage of the program and abort			*/
/****************************************************************/
void explain()
{
  printf("Usage:\n");
  printf(">xsd inname outname\n\n");
  printf("The file named inname.xsa will be decompressed and saved as outname\n\n");
  printf("Note: the compression format is compatible with d2f's and f2d's format\n");
  exit(128);
}

/****************************************************************/
/* give an error message and abort				*/
/****************************************************************/
void error(string)
char *string;
{
  puts(string);
  exit(128);
}

/****************************************************************/
/* Put the next character in the input buffer.                  */
/* Take care that minimal 'slwinsize' chars are before the      */
/* current position.                                            */
/****************************************************************/
void charout(ch)
TINY ch;
{
  if ((outbufcnt++) == outbufsize) {
    write(outfile, outbuf, outbufsize-slwinsize);
    memcpy(outbuf, outbuf+outbufsize-slwinsize, slwinsize);
    outbufpos = outbuf+slwinsize;
    outbufcnt = slwinsize+1;
  }
  *(outbufpos++) = ch;
}

/****************************************************************/
/* flush the output buffer					*/
/****************************************************************/
void flushoutbuf()
{
  if (outbufcnt) {
    write(outfile, outbuf, outbufcnt);
    outbufcnt = 0;
  }
}

/****************************************************************/
/* read string length						*/
/****************************************************************/
unsigned rdstrlen()
{
  TINY len;
  TINY nrbits;

  if (!bitin())
    return 2;
  if (!bitin())
    return 3;
  if (!bitin())
    return 4;

  for (nrbits = 2; (nrbits != 7) && bitin(); nrbits++)
    ;

  len = 1;
  while (nrbits--)
    len = (len << 1) | bitin();

  return (unsigned)(len+1);
}

/****************************************************************/
/* read string pos						*/
/****************************************************************/
unsigned rdstrpos()
{
  TINY nrbits;
  TINY cpdindex;
  unsigned strpos;
  huf_node *hufpos;

  hufpos = huftbl+2*tblsize-2;

#ifdef pos_tst
  putchar('.');
#endif
  while (hufpos->child1)
    if (bitin()) {
#ifdef pos_tst
      putchar('1'); 
#endif
      hufpos = hufpos->child2;
    }
    else {
#ifdef pos_tst
      putchar('0'); 
#endif
      hufpos = hufpos->child1;
    }
  cpdindex = hufpos-huftbl;
#ifdef pos_tst
  printf(".%d", cpdindex); 
#endif
  tblsizes[cpdindex]++;

  if (cpdbmask[cpdindex] >= 256) {
    TINY strposmsb, strposlsb;

    strposlsb = (unsigned)charin();  /* lees LSB van strpos */
    strposmsb = 0;
    for (nrbits = cpdext[cpdindex]-8; nrbits--; strposmsb |= bitin())
      strposmsb <<= 1;
    strpos = (unsigned)strposlsb | (((unsigned)strposmsb)<<8);
  }
  else {
#ifdef MSX_C
    TINY temp = 0;
    for (nrbits = cpdext[cpdindex]; nrbits--; temp |= bitin())
      temp <<= 1;
    strpos = (unsigned)temp;
#else
    strpos=0;
    for (nrbits = cpdext[cpdindex]; nrbits--; strpos |= bitin())
      strpos <<= 1;
#endif
  }    
  if (!(updhufcnt--))
    mkhuftbl(); /* make the huffman table */

  return strpos+cpdist[cpdindex];
}

/****************************************************************/
/* read a bit from the input file				*/
/****************************************************************/
TINY bitin()
{
  TINY temp;

  if (!bitcnt) {
    bitflg = charin();  /* lees de bitflg in */
    bitcnt = 8;         /* nog 8 bits te verwerken */
  }
  temp = bitflg & 1;
  bitcnt--;  /* weer 1 bit verwerkt */
  bitflg >>= 1;

  return temp;
}

/****************************************************************/
/* Get the next character from the input buffer.                */
/****************************************************************/
#ifdef hoho
TINY chinhoho()
{
  TINY temp;

  while (((temp = charin()) == 10) || (temp == 13))
    ;
  return temp;
}
#endif
TINY charin()
{
  if (!(inbufcnt--)) {
    if (!(inbufcnt = read(infile, (inbufpos=inbuf), inbufsize))) {
#ifdef hoho
      return 255;
#else
      flushoutbuf();
      close(outfile);
      close(infile);
      error("Unexpected end of file\n");
#endif
    }
    else {
      nrread += inbufcnt;
#ifdef __MSDOS__
      fprintf(stderr, "\015nr bytes read: %6lu", nrread);
#else
      fprintf(stderr, "\015nr bytes read: %6u", nrread);
#endif
      inbufcnt--;
    }
  }
  return *(inbufpos++);
}
