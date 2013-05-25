#ifndef RANDOM_H
#define RANDOM_H

#ifndef WIN32
#include <stdint.h>
#endif //WIN32

#define MT_BUFFER_NUM 10
#define MT_LEN 624


void mt_init();
uint32_t mt_trandom();
static int32_t trandom(uint32_t max=2147483647LU)
	{
	if(max<=1)return 0;
	uint32_t seed=mt_trandom();
	seed=seed%2147483647LU;
	seed=seed/((2147483647LU/max)+1);

	return((int32_t)seed);
	}
static int32_t loadtrandom(uint32_t max=2147483647LU)
	{
	uint32_t seed=mt_trandom();
	seed=seed%max;

	return((int32_t)seed);
	}
void push_trandom_uniform_seed(uint32_t newseed);
void push_trandom_double_seed(uint32_t newseed1,uint32_t newseed2);
void push_trandom_triple_seed(uint32_t newseed1,uint32_t newseed2,uint32_t newseed3);
void pop_trandom_uniform_seed();
void trandom_twist();

void r_num();
int32_t basic_random(int32_t max=2147483647);


#endif
