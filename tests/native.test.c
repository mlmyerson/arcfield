#include <assert.h>
#include <stdio.h>
#include "../native/engine.c"
int main(void){tiles[0]=2;double*in=(double*)schedule_ptr();double input[]={0,0,0,0,0,0,0,2};for(int i=0;i<8;i++)in[i]=input[i];assert(configure(1,1,5,1,2,123));step(300);assert(collision_count()==1);assert(exit_count()==0);assert(active_count()==0);assert(tiles[0]!=2);assert(bins[0]==1);assert(!configure(1000,1000,5,1,2,1));puts("PASS: native C head-on collision, mutation, accounting and input limits");}
