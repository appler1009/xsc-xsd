#ifndef LZ77TDEFheader
#define LZ77TDEFheader

#ifdef MSX_C
#define SHORT TINY
#else
#define SHORT unsigned
#endif

#define slwinsnrbits (13)
#define maxstrlen (254)

#define maxhufcnt (127)
#define logtblsize (4)
#define tblsize (1<<logtblsize)

typedef struct huf_node {
  SHORT weight;
  struct huf_node *child1, *child2;
} huf_node;

#endif
