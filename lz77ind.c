/****************************************************************/
/* The C module for the index search LZ77 compression		*/
/* algorithm							*/
/*   version   startdate					*/
/*   0.9       15-4-1995					*/
/****************************************************************/
#ifndef __MSDOS__
#include "machine.h"
#endif

/* #define debug */

#ifdef debug
#define tstprint(x) printf(x)
#define tstputchar(x) putchar(x)
#else
#define tstprint(x)
#define tstputchar(x)
#endif

#include <stdio.h>
#ifndef UNIX_C
#include <io.h>
#endif
#include <fcntl.h>

#ifdef __MSDOS__
#include <dos.h>
#include <model.h>
#include <mem.h>
#include <alloc.h>
#else
#include <memory.h>
#ifndef __clang__
#include <malloc.h>
#endif
#endif

#include "pcmsx.h"
#include "lz77tdef.h"
#include "lz77ind.h"
#include "lz77main.h"
#include "lz77out.h"

#ifdef MSX_C
#include <tmem.h>
#pragma nonrec
#else
#define recursive
#endif

#ifndef UNIX_C
#define inline
#endif

/****************************************************************/
/* Some usefull macros, etc.					*/
/****************************************************************/
#ifndef MSX_C
#define lognrindexptrs 8 /* 2log of nr index ptrs per char    */
#else
#define lognrindexptrs 6
#endif
#define nrindexptrs (1<<lognrindexptrs)  /* #index ptrs per char */
#define indexpmsk (nrindexptrs-1)
#ifdef MSX_C
#define index(ch, offs) *(strptrs + (unsigned)ch*64 + ((offs) & indexpmsk))
#else
#define index(ch, offs) *(strbufptrs[(TINY)(ch)] + ((offs) & indexpmsk))
#endif

#define offsindex(chindex, offs) *(chindex + ((offs) & indexpmsk))

typedef struct Indexinfo {
  unsigned firstptr;
  unsigned nrptrs;
} Indexinfo;

/****************************************************************/
/* the local and global vars					*/
/****************************************************************/
static FD   infile;        /* the input file                  */
static TINY *inbuf;        /* the input buffer                */
static TINY *inbufpos;     /* pos in input buffer             */
static unsigned inbufcnt;  /* #elements in the input buffer   */
static TINY *mwindow;      /* the (wrap around) moving window */

static BOOL eofflg;        /* end of file flag for input file */
#ifdef __MSDOS__
static long nrread;
#else
static unsigned nrread;
#endif

static TINY ***strbufptrs;  /* buffer with ptrs to strptrs     */
static Indexinfo *indexinfo;/* firstptr and nrptrs             */
static unsigned freepos;    /* first free pos in moving window */

/****************************************************************/
/* The function prototypes					*/
/****************************************************************/
FD   inopen();       /* open input files and init vars        */
long inglen();       /* get length of input file              */
void inclose();      /* close the input files                 */
void freebufs();     /* free all used buffers                 */
void lz77();         /* perform the real compression          */
void lz77real();     /* the real compression		      */
void init_index();   /* initialize the index information      */
static inline void DeleteString(); /* delete a string from the indexptrs */
static inline void fAddString(); /* add str without srch best match */
unsigned AddString();/* add str with srch for best match      */
int  getc2();        /* get a character from the input stream */
#ifdef tree_tst
void prtnumber();
#endif

/****************************************************************/
/* open de input file en init alle vars die voor input nodig	*/
/* zijn								*/
/****************************************************************/
FD inopen(name, flags)
char *name;
unsigned flags;
{
  unsigned cnt;

  inbuf  = zmalloc(inbufsize);
  mwindow = zmalloc(slwinsize + maxstrlen);
  strbufptrs = (TINY ***)zmalloc(256 * sizeof(TINY **));
  for (cnt = 0; cnt != 256; cnt++)
    strbufptrs[cnt] = (TINY **)zmalloc(nrindexptrs * sizeof(TINY *));
  indexinfo = (Indexinfo *)zmalloc(256 * sizeof(Indexinfo));

  eofflg = FALSE;   /* end of file has not been reached yet */
  inbufcnt = 0;     /* er zit nog niks in de input buffer   */
  nrread   = 0;     /* er is nog niks ingelezen             */
  return (infile = open(name, flags));
}

/****************************************************************/
/* Get length of the input file					*/
/****************************************************************/
long inglen()
{
  return (long)nrread;
}

/****************************************************************/
/* sluit de input file						*/
/****************************************************************/
void inclose()
{
  close(infile);
}

/****************************************************************/
/* geef alle input/output/werk buffers vrij			*/
/****************************************************************/
void freebufs()
{
  unsigned cnt;

  free(inbuf);
  free(mwindow);
  for (cnt = 0; cnt != 256; cnt++)
    free(strbufptrs[cnt]);
  free(strbufptrs);
  free(indexinfo);
}

/****************************************************************/
/* the actual compression algorithm itself			*/
/****************************************************************/
void lz77()
{
  int      ch;            /* last read character         */
  unsigned lookahead_cnt; /* nrbytes in lookahead buffer */

  for (lookahead_cnt = 0; lookahead_cnt != maxstrlen; lookahead_cnt++) {
    if ((ch = getc2()) == EOF)
      break;
    mwindow[(unsigned)lookahead_cnt] = ch;
    mwindow[slwinsize+(unsigned)lookahead_cnt] = ch;
  }
  init_index();    /* init the index for the first string */
  freepos = 1;
  if (lookahead_cnt)
    lz77real(lookahead_cnt);
  strout(mwindow, (TINY)(maxstrlen+1), 0); /* sluit file af */
}

/****************************************************************/
/* add a string to the index without searching for a best match	*/
/****************************************************************/
static inline void fAddString(winpos)
unsigned winpos;     /* start pos of the string to add */
{
  register Indexinfo *indexinfoptr;
  register TINY *winposptr;
  register TINY ch;

  indexinfoptr = indexinfo + (ch = *(winposptr = mwindow+winpos));
  index(ch, indexinfoptr->firstptr + (indexinfoptr->nrptrs++)) =
    winposptr;
}

/****************************************************************/
/* delete a string from the index information			*/
/****************************************************************/
static inline void DeleteString(strpos)
unsigned strpos;  /* start pos of the string */
{
  register Indexinfo *indexinfoptr;

  if (strpos >= freepos)
    return; /* this string is not in the moving window */

  (indexinfoptr = indexinfo + mwindow[strpos])->nrptrs--;
  indexinfoptr->firstptr++;
}

/****************************************************************/
/* perform the real compression loop				*/
/****************************************************************/
void lz77real(lookahead_cnt)
unsigned lookahead_cnt;
{
  int      ch;         /* last read character                   */
  unsigned winpos;     /* logic pos in sliding window           */
  unsigned strlen;     /* length of the string                  */
  unsigned strpos;     /* logic pos of string in sliding window */
  unsigned replace_cnt;/* #strings to replace                   */

  winpos = 0;     /* start at position 0           */
  strlen = 0;     /* start with a string of size 0 */

  /* compress data forever */
  while (TRUE) {

    if (strlen <= 1) {
      charout(mwindow[winpos]);
      replace_cnt = 1;
    }
    else {
      if (strlen > lookahead_cnt)
	strlen = lookahead_cnt;   /* strlen must be <= lookahead_cnt */
      strout(mwindow+strpos, (TINY)(replace_cnt = strlen),
	     (winpos-strpos) & slwinmask);
    }

    while (--replace_cnt) {
      DeleteString((winpos+maxstrlen) & slwinmask);
      if ((ch=getc2()) == EOF)
	lookahead_cnt--;  /* one character less in the buffer */
      else {
	mwindow[(winpos+maxstrlen) & slwinmask] = ch; /* store new char */
	if ((winpos+maxstrlen) >= slwinsize)
	  mwindow[winpos+maxstrlen] = ch;
      }
      fAddString((winpos = (winpos+1) & slwinmask));
      if (freepos < slwinsize)
	freepos++;
    }
    DeleteString((winpos+maxstrlen) & slwinmask);
    if ((ch=getc2()) == EOF) {
      if (!(--lookahead_cnt)) /* one character less in the buffer */
	return;  /* nothing left, leave the compression loop */
    }
    else {
      mwindow[(winpos+maxstrlen) & slwinmask] = ch; /* store new char */
      if ((winpos+maxstrlen) >= slwinsize)
	mwindow[winpos+maxstrlen] = ch;
    }
    strlen = AddString((winpos = (winpos+1) & slwinmask), &strpos);
    if (freepos < slwinsize)
      freepos++;
  }
}

/****************************************************************/
/* initialize the index information				*/
/****************************************************************/
void init_index()
{
  tstprint("in init_index()\n");

  /* reset all ptrs and counters */
  memset(indexinfo, 0, 256*sizeof(Indexinfo));

  /* init. index info for first string */
  indexinfo[mwindow[0]].nrptrs++;
  index(mwindow[0], 0) = mwindow;
  tstprint("out init_index()\n");
}

/****************************************************************/
/* Add a string to the index, search for largest match		*/
/* Note: on similar match lengths, the smallest stringpos will	*/
/*       be choosen to obtain short string codes		*/
/****************************************************************/
unsigned AddString(winpos, strposptr)
unsigned winpos;     /* start pos of the string to add */
unsigned *strposptr; /* ptr to the variable holding the strpos on match */
{
  unsigned indexcnt;         /* index die nu bekeken wordt */
  unsigned match_length;
  unsigned firstptr;
  register unsigned count;
  register TINY *str1, *str2;
  TINY *winposptr;
  TINY **chindex;
  Indexinfo *indexinfoptr;

  firstptr = (indexinfoptr=indexinfo+*(winposptr=mwindow+winpos))->firstptr;
  chindex = strbufptrs[*winposptr];
  match_length = 0;
  if ((indexcnt = indexinfoptr->nrptrs) > nrindexptrs)
    indexcnt = nrindexptrs;
  while (indexcnt--) {
    str1 = winposptr;
    str2 = offsindex(chindex, firstptr+indexcnt);
    for (count = maxstrlen-1;(*(++str1) == *(++str2)) && --count;)
      ;
    if ((maxstrlen-count) > match_length) {
      /* Found a longer match */
      match_length = maxstrlen-count;
      *strposptr = str2-match_length-mwindow;
      if (!count) {
	(*strposptr)++;
	break; /* Found an exact match. No further search needed */
      }
    }
  }
  offsindex(chindex, firstptr + indexinfoptr->nrptrs++) = winposptr;
  return match_length;
}

/****************************************************************/
/* Get the next character from the input buffer.                */
/* Take care that minimal 'slwinsize' chars are before the      */
/* current position.                                            */
/****************************************************************/
int getc2()
{
  if (!(inbufcnt--)) {
    if (eofflg) {
      inbufcnt = 0;
      return EOF;  /* end of file has been reached */
    }
    if (!(inbufcnt = read(infile, (inbufpos=inbuf), inbufsize))) {
      eofflg = TRUE;
      return EOF;
    }
    else {
      nrread += inbufcnt;
#ifdef __MSDOS__
      fprintf(stderr, "\015nr bytes read: %5lu", nrread);
#else
      fprintf(stderr, "\015nr bytes read: %5u", nrread);
#endif
      inbufcnt--;
    }
  }
  return (int)(*(inbufpos++));
}

#ifdef tree_tst
/****************************************************************/
/* print een nummer uit, voorafgegaan door nullen               */
/****************************************************************/
void prtnumber(fp, number)
FILE *fp;
unsigned number;
{
  unsigned count;
  unsigned divider = 10000;
  TINY digit;

  for (count = 0; count != 5; count++) {
    digit = '0';
    while (number >= divider) {
      number -= divider;
      digit++;
    }
    putc((int)digit, fp);
    divider /= 10;
  }
}
#endif
