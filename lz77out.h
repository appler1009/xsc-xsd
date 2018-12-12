#ifndef __LZ77OUTheader__
#define __LZ77OUTheader__

typedef FD BD;

#define bitout(n) { if (!setflg) bitout2();\
                    if (n) bitflg |= setflg;\
                    setflg <<= 1; }

extern BD   bitcreat();
extern int  bitclose();
extern void byteout();
extern void bitout2();
extern void fileout();
extern void strout();
extern void charout();

#endif
