#include "platform.h"
#include <string.h>
#include <math.h>
#include <iosfwd>
#include <iostream>
#include <ios>
#include <streambuf>
#include <istream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <zlib.h>

#include "svector.h"
using std::string;

#include "endian.h"

#include "files.h"

#include "enabler.h"

#include "textlines.h"

#include "basics.h"

#include "random.h"

extern int32_t basic_seed;
extern int mt_index[MT_BUFFER_NUM];
extern short mt_cur_buffer;
extern short mt_virtual_buffer;
extern uint32_t mt_buffer[MT_BUFFER_NUM][MT_LEN];

//public domain RNG stuff by Michael Brundage
	//with some modifications by me to handle more buffers

void mt_init()
{
	mt_cur_buffer=0;
	mt_virtual_buffer=0;

    mt_buffer[0][0]=GetTickCount();
    int i;
    for(i=1;i<MT_LEN;i++)
		{
		//2010: better init line from wikipedia, ultimate source unknown
		mt_buffer[0][i]=1812433253UL * (mt_buffer[0][i-1] ^ (mt_buffer[0][i-1]>>30)) + i;
		}
    mt_index[0]=MT_LEN*sizeof(uint32_t);

	int32_t j;
	for(j=0;j<20;j++)trandom_twist();
}

#define MT_IA           397
#define MT_IB           (MT_LEN - MT_IA)
#define UPPER_MASK      0x80000000
#define LOWER_MASK      0x7FFFFFFF
#define MATRIX_A        0x9908B0DF
#define TWIST(b,i,j)    ((b)[i] & UPPER_MASK) | ((b)[j] & LOWER_MASK)
#define MAGIC(s)        (((s)&1)*MATRIX_A)

uint32_t mt_trandom()
{
    uint32_t * b = mt_buffer[mt_cur_buffer];
    int idx = mt_index[mt_cur_buffer];
    uint32_t s;
    int i;
	
    if (idx == MT_LEN*sizeof(uint32_t))
    {
        idx = 0;
        i = 0;
        for (; i < MT_IB; i++) {
            s = TWIST(b, i, i+1);
            b[i] = b[i + MT_IA] ^ (s >> 1) ^ MAGIC(s);
        }
        for (; i < MT_LEN-1; i++) {
            s = TWIST(b, i, i+1);
            b[i] = b[i - MT_IB] ^ (s >> 1) ^ MAGIC(s);
        }
        
        s = TWIST(b, MT_LEN-1, 0);
        b[MT_LEN-1] = b[MT_IA-1] ^ (s >> 1) ^ MAGIC(s);
    }
    mt_index[mt_cur_buffer] = idx + sizeof(uint32_t);
     return *(uint32_t *)((unsigned char *)b + idx);
}

void trandom_twist()
{
    uint32_t * b = mt_buffer[mt_cur_buffer];
    uint32_t s;
    int i;
	
    i = 0;
    for (; i < MT_IB; i++) {
        s = TWIST(b, i, i+1);
        b[i] = b[i + MT_IA] ^ (s >> 1) ^ MAGIC(s);
    }
    for (; i < MT_LEN-1; i++) {
        s = TWIST(b, i, i+1);
        b[i] = b[i - MT_IB] ^ (s >> 1) ^ MAGIC(s);
    }
    
    s = TWIST(b, MT_LEN-1, 0);
    b[MT_LEN-1] = b[MT_IA-1] ^ (s >> 1) ^ MAGIC(s);
}

//back to my crap - tarn
void pop_trandom_uniform_seed()
{
	if(mt_virtual_buffer>0)mt_virtual_buffer--;
	mt_cur_buffer=mt_virtual_buffer;
	if(mt_cur_buffer>=MT_BUFFER_NUM)mt_cur_buffer=MT_BUFFER_NUM-1;
}

void push_trandom_uniform_seed(uint32_t newseed)
{
	mt_virtual_buffer++;
	mt_cur_buffer=mt_virtual_buffer;
	if(mt_cur_buffer>=MT_BUFFER_NUM)
		{
		mt_cur_buffer=MT_BUFFER_NUM-1;
		errorlog_string("Random Buffer Overload");
		}

    short i;

	uint32_t * b = mt_buffer[mt_cur_buffer];

	b[0]=newseed;
    for(i=1;i<MT_LEN;i++)
		{
		//2010: better init line from wikipedia, ultimate source unknown
		b[i]=1812433253UL * (b[i-1] ^ (b[i-1]>>30)) + i;
		}
    mt_index[mt_cur_buffer]=MT_LEN*sizeof(uint32_t);

	trandom_twist();
}

void push_trandom_double_seed(uint32_t newseed1,uint32_t newseed2)
{
	mt_virtual_buffer++;
	mt_cur_buffer=mt_virtual_buffer;
	if(mt_cur_buffer>=MT_BUFFER_NUM)
		{
		mt_cur_buffer=MT_BUFFER_NUM-1;
		errorlog_string("Random Buffer Overload");
		}

    short i;

	uint32_t * b = mt_buffer[mt_cur_buffer];

	uint64_t s1=newseed1;
	uint64_t s2=newseed2;
	uint64_t p1=(s1+s2)*(s1+s2+1)+s2*2;
	b[0]=(uint32_t)p1;
    for(i=1;i<MT_LEN;i++)
		{
		b[i]=1812433253UL * (b[i-1] ^ (b[i-1]>>30)) + i;
		}
    mt_index[mt_cur_buffer]=MT_LEN*sizeof(uint32_t);

	trandom_twist();
}

void push_trandom_triple_seed(uint32_t newseed1,uint32_t newseed2,uint32_t newseed3)
{
	mt_virtual_buffer++;
	mt_cur_buffer=mt_virtual_buffer;
	if(mt_cur_buffer>=MT_BUFFER_NUM)
		{
		mt_cur_buffer=MT_BUFFER_NUM-1;
		errorlog_string("Random Buffer Overload");
		}

    short i;

	uint32_t * b = mt_buffer[mt_cur_buffer];

	uint64_t s1=newseed1;
	uint64_t s2=newseed2;
	uint64_t p1=(s1+s2)*(s1+s2+1)+s2*2;
	uint64_t s3=newseed3;
	uint64_t p2=(p1+s3)*(p1+s3+1)+s3*2;
	b[0]=(uint32_t)p2;
    for(i=1;i<MT_LEN;i++)
		{
		b[i]=1812433253UL * (b[i-1] ^ (b[i-1]>>30)) + i;
		}
    mt_index[mt_cur_buffer]=MT_LEN*sizeof(uint32_t);

	trandom_twist();
}

//picks a random number from 0 to max-1
int32_t basic_random(int32_t max)
{
	r_num();

	return (int32_t)((uint32_t)basic_seed/((1073741824UL/(uint32_t)max)+1UL));
}

//sets seed to a random number from 0 to 1 billion
void r_num()
{
	basic_seed=(int32_t)(((uint32_t)basic_seed*907725UL+99979777UL)%1073741824UL);
}