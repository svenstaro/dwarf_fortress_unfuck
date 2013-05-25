#ifndef ENDIAN_H
#define ENDIAN_H

inline unsigned short byteswap(unsigned short x)
{
#if defined(__ppc__) || defined(__ppc64__)
       return (x << 8 | x >> 8);
#else
       return x;
#endif
}

inline unsigned long byteswap(unsigned long x)
{
#if defined(__ppc__) || defined(__ppc64__)
       return
               ( (x << 24) & 0xFF000000) |
               ( (x <<  8) & 0x00FF0000) |
               ( (x >>  8) & 0x0000FF00) |
               ( (x >> 24) & 0x000000FF) ;
#else
       return x;
#endif
}

inline unsigned int byteswap(unsigned int x)
{
#if defined(__ppc__) || defined(__ppc64__)
       return
               ( (x << 24) & 0xFF000000) |
               ( (x <<  8) & 0x00FF0000) |
               ( (x >>  8) & 0x0000FF00) |
               ( (x >> 24) & 0x000000FF) ;
#else
       return x;
#endif
}

inline short byteswap(short x) { return byteswap( (unsigned short) x ); }
inline long byteswap(long x) { return byteswap( (unsigned long) x ); }
inline int byteswap(int x) { return byteswap( (unsigned int) x ); }
#endif
