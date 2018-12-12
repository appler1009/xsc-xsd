#ifndef LZ77INDheader
#define LZ77INDheader

#define slwinsize (1 << slwinsnrbits)
#define slwinmask (slwinsize - 1)
#ifdef MSX_C
#define inbufsize 1024
#else
#define inbufsize 64512
#endif

#endif

