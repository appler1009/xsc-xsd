#ifndef LZ77HUFheader
#define LZ77HUFheader

extern unsigned updhufcnt;
extern unsigned cpdist[tblsize+1];
extern unsigned cpdbmask[tblsize];
extern unsigned cpdext[tblsize];

extern SHORT tblsizes[tblsize];
extern huf_node huftbl[2*tblsize-1];

extern void mkhuftbl();
extern void inithufinfo();

#endif
