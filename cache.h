#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include <iterator>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data; // the actual data stored in the cache/memory
	bool valid;
	int index;
	// add more things here if needed
};

struct Stat
{
	int missL1; 
	int missL2; 
	int accL1;
	int accL2;
	int accVic;
	int missVic;
	// add more stat if needed. Don't forget to initialize!
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	cacheBlock Victim[VICTIM_SIZE]; // FA victim cache
	
	
	Stat myStat;
	// add more things here
public:
	cache();
	Stat getStat();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	void update(cache* L, int L1check, int Viccheck, int L2check);// add more functions here ...	
	int searchL1(int adr, cacheBlock *L1);
	int searchL2(int adr, cacheBlock (*L2)[8]);
	int searchVic(int adr, cacheBlock *V);
	int getTag(cacheBlock L);
	int getData(cacheBlock L);
	int getValid(cacheBlock L);
	void ViclruUpdate(int index, cacheBlock *L);
	void L2lruUpdate(int index1, int index2, cacheBlock (*L)[8]);
	void L1victimReplace(cacheBlock& La, cacheBlock& Lb); 		// Just a simpler way to do evict and insert number in this special case(swap)
	cacheBlock L1VicEvict(int index, cacheBlock* L);
	cacheBlock L2Evict(int setIndex, int wayIndex, cacheBlock (*L)[8]);
	int findFirstZeroVic(cacheBlock *L);
	int findFirstZeroL2(int index, cacheBlock(*L)[8]);
};


