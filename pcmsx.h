/*
  Een aantal definities om de verschillen tussen de ASCII MSX C library
  en de Borland Turbo C library af te vangen
*/

#ifndef PCMSXheader
#define PCMSXheader

#ifndef MSX_C 
/* Definities uit de MSX compiler voor de PC compiler */
typedef int FD;
#define nonrec
#define BOOL int
#define CPMEOF 0x1a
#define ERROR (-1)
#define TINY unsigned char
#define TRUE (1)
#define FALSE (0)
#else
/* Definities uit de PC compiler voor de MSX compiler */
#ifndef void
#define void int
#endif
#define fgetc getc
#define fgetchar getc
#endif

#endif

