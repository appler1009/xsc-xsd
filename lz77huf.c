/****************************************************************/
/* De lz77 huffman module					*/
/* Copyright (c) 1995 XelaSoft					*/
/* 26-4-1995, Alex Wulms					*/
/*            Pelikaanhof 127 c					*/
/*            2312 EG Leiden					*/
/*            071-124846					*/
/****************************************************************/
#define DIVHEADER
#ifndef __MSDOS__
#include "machine.h"
#else
#include <model.h>
#endif

#include <stdio.h>
/* #define alt_out */

#include "pcmsx.h"
#include "lz77tdef.h"

#ifdef MSX_C
#pragma nonrec
#endif

/****************************************************************/
/* some usefull macro's, etc					*/
/****************************************************************/

/****************************************************************/
/* global vars							*/
/****************************************************************/
unsigned updhufcnt;
unsigned cpdist[tblsize+1];
unsigned cpdbmask[tblsize];
#ifdef bits5
unsigned cpdext[] = { /* Extra bits for distance codes */
          0,  0,  0,  0,  1,  1,  2,  2,  2,  2,  3,
          3,  3,  4,  4,  4,  5,  5,  5,  5,  5,  6,
          6,  6,  6,  7,  7,  8, 10, 11, 11, 11 };
#else
unsigned cpdext[] = { /* Extra bits for distance codes */
          0,  0,  0,  0,  1,  2,  3,  4,
          5,  6,  7,  8,  9, 10, 11, 12};
#endif

SHORT tblsizes[tblsize];
huf_node huftbl[2*tblsize-1];

/****************************************************************/
/* The function prototypes					*/
/****************************************************************/
void mkhuftbl(); /* maak de huffman boom */
void inithufinfo(); /* initialize the cpdist tables */

/****************************************************************/
/* maak de huffman codeer informatie				*/
/****************************************************************/
void mkhuftbl()
{
  unsigned count;
  huf_node  *hufpos;
  huf_node  *l1pos, *l2pos;
  SHORT tempw;
 
#ifdef alt_out
  printf("\nmaking the huffman tree\n");
#endif
  /* Initialize the huffman tree */
  for (count=0, hufpos=huftbl; count != tblsize; count++) {
    (hufpos++)->weight=1+(tblsizes[count] >>= 1);
#ifdef alt_out
    printf("%2u, %5u\n", count, (unsigned)hufpos->weight);
#endif
  }
  for (count=tblsize; count != 2*tblsize-1; count++)
    (hufpos++)->weight=-1;

  /* Place the nodes in the correct manner in the tree */
  while (huftbl[2*tblsize-2].weight == -1) {
    for (hufpos=huftbl; !(hufpos->weight); hufpos++)
      ;
    l1pos = hufpos++;
    while (!(hufpos->weight))
      hufpos++; 
    if (hufpos->weight < l1pos->weight) {
      l2pos = l1pos;
      l1pos = hufpos++;
    }
    else
      l2pos = hufpos++;
    while ((tempw=(hufpos)->weight) != -1) {
      if (tempw) {
        if (tempw < l1pos->weight) {
          l2pos = l1pos;
          l1pos = hufpos;
        }
        else if (tempw < l2pos->weight)
          l2pos = hufpos;
      }
      hufpos++;
    }
    hufpos->weight = l1pos->weight+l2pos->weight;
    (hufpos->child1 = l1pos)->weight = 0;
    (hufpos->child2 = l2pos)->weight = 0;
  }
  updhufcnt = maxhufcnt;
}

/****************************************************************/
/* initialize the huffman info tables                           */
/****************************************************************/
void inithufinfo()
{
  unsigned offs, count;

  for (offs=1, count=0; count != tblsize; count++) {
    cpdist[count] = offs;
    offs += (cpdbmask[count] = 1<<cpdext[count]);
  }
  cpdist[count] = offs;
/*  printf("eind offset: %u, eindcount %u\n", offs, count); */

  for (count = 0; count != tblsize; count++) {
    tblsizes[count] = 0; /* reset the table counters */
    huftbl[count].child1 = 0;  /* mark the leave nodes */
  }
  mkhuftbl(); /* make the huffman table */
}
