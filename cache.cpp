#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++)
		{L1[i].valid = false;
		L1[i].lru_position =0;
		L1[i].index = i;} // lru_index for L1 doesn't matter
	for (int i=0; i<L2_CACHE_SETS; i++)
		for (int j=0; j<L2_CACHE_WAYS; j++)
			{L2[i][j].valid = false;
			L2[i][j].lru_position =j;
			L2[i][j].index = i;} // Initialize lru_position for each way from left to right with 0-8
	for (int i=0; i<VICTIM_SIZE; i++)
		{Victim[i].valid = false;
		 Victim[i].lru_position = i;
		 Victim[i].index = i;}
	// Do the same for Victim Cache ...

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.missVic =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;
	this->myStat.accVic =0;
	// Add stat for Victim cache ... 
	
}
Stat cache::getStat(){
	return this->myStat;
}
void cache::update(cache* L, int L1check, int Viccheck, int L2check){
    if (L1check != -1){L->myStat.accL1++;} //Check and find data in L1
	//Check and find data in victim, miss in L1
	else if (L1check == -1 && Viccheck != -1) {L->myStat.accL1++; L->myStat.missL1++; L->myStat.accVic++;}
	//Check and find data in L2, miss in L1 and vic find or miss in L2
	else if (L1check == -1 && Viccheck == -1 && L2check != -1){
        L->myStat.accL1++; L->myStat.missL1++; L->myStat.accVic++; L->myStat.missVic++; L->myStat.accL2++; }
    else if ((L1check == -1 && Viccheck == -1 && L2check == -1)){L->myStat.accL1++; L->myStat.missL1++; L->myStat.accVic++; L->myStat.missVic++; L->myStat.accL2++; L->myStat.missL2++;}
}
int cache::getTag(cacheBlock L){return L.tag;}
int cache::getData(cacheBlock L){return L.data;}
int cache::getValid(cacheBlock L){return L.valid;}


int cache::searchL1(int adr, cacheBlock *L1){
	int IndexNum = static_cast<unsigned int>(adr<<26)>>28;
	int tagNum = adr >> 6;
	if ((tagNum == getTag(L1[IndexNum])) && (getValid(L1[IndexNum]) == 1)) {return IndexNum;}
	else
	return -1;
}

int cache::searchL2(int adr, cacheBlock (*L2)[8]){
	int IndexNum = static_cast<unsigned int>(adr<<26)>>28;
	int tagNum = adr >> 6;
	for (int j=0; j < L2_CACHE_WAYS; j++){
		if ((tagNum == getTag(L2[IndexNum][j])) && (getValid(L2[IndexNum][j]) == 1)) {return j;}
	}
	return -1;
}
int cache::searchVic(int adr,  cacheBlock *V){
    int IndexNum = static_cast<unsigned int>(adr<<26)>>28;
    int tagNum = adr >> 6;						//Get the tagnumber from the address, here we make victim cache behave similarly as L1,L2
	for(int i=0; i < VICTIM_SIZE; i++){			//Iterate all entries to find any matches with the tag. 
		if ((tagNum == getTag(V[i])) && (getValid(V[i]) == 1) && (IndexNum == V[i].index)) {return i;} //If there's a match, return the index, otherwise return -1.
	}
	return -1;
}
void cache::ViclruUpdate(int index, cacheBlock *L){ //Function to update Victim lruindex, set the most recently used index position to N-1, the other index just -1. If smaller than 0, just set to 0.
	for(int i=0;i<VICTIM_SIZE;i++){
		L[i].lru_position = (i == index) ? VICTIM_SIZE - 1:L[i].lru_position -1;
		if (L[i].lru_position < 0) {L[i].lru_position = 0;}
	}
}
void cache::L2lruUpdate(int index1, int index2, cacheBlock (*L)[8]){ //Function to update L2 lruindex, set the most recently used index position to N-1, the other index just -1. If smaller than 0, just set to 0.
	for(int i=0;i<L2_CACHE_WAYS;i++){				//Prior calling this function, we should use searchL2 to ensure that there is a hit. If hit, update lru with the given index, if not look into L2, and do nothing to its lru at all.
		L[index1][i].lru_position = (i == index2) ? 7:L[index1][i].lru_position-1;
		if (L[index1][i].lru_position < 0) {L[index1][i].lru_position = 0;}
	}
}

void cache::L1victimReplace(cacheBlock& La, cacheBlock& Lb){
    La.tag = Lb.tag; La.data = Lb.data; La.valid = Lb.valid; La.index = Lb.index;
}

cacheBlock cache::L1VicEvict(int L1index, cacheBlock* L){	
	if (L[L1index].valid != 0) {cacheBlock temp = L[L1index]; L[L1index].tag = 0;L[L1index].data = 0; L[L1index].valid = 0; return temp;}
	return L[L1index];
}

cacheBlock cache::L2Evict(int setIndex, int wayIndex, cacheBlock (*L)[8]){ //Store in temp before use.
	if(L[setIndex][wayIndex].valid != 0){cacheBlock temp = L[setIndex][wayIndex]; L[setIndex][wayIndex].valid = 0; L[setIndex][wayIndex].tag = 0; L[setIndex][wayIndex].data = 0; return temp;}
	return L[setIndex][wayIndex];
}
void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{	//Initialize the Check Index with invalid value.
	int L1CheckIndex = -1; int VicCheckIndex = -1; int L2CheckIndex = -1; unsigned int rowInd = static_cast<unsigned int>(adr<<26) >> 28;
	if(MemR == 0 && MemW == 1){	    //Store instruction, We search upper level hierarchy for a hit.
		L1CheckIndex = searchL1(adr,this->L1);
		if (L1CheckIndex!=-1){  
			// Case where a hit is in the L1, data renewed and write into memory.
			this->L1[L1CheckIndex].data = *data;	myMem[adr] = *data;
		}
		else {						//Miss in L1, check in Victim（L1CheckIndex == -1)
			VicCheckIndex = searchVic(adr,this->Victim);
			if(VicCheckIndex != -1){
				// Case where a hit in victim
				this -> Victim[VicCheckIndex].data = *data; myMem[adr] = *data;
				ViclruUpdate(VicCheckIndex,this->Victim);//Update victim lru index on a vic hit, and renew data.
			}
			else{					//Miss in Vic too, hit in L2(VicCheckIndex == -1,L2CheckIndex != -1)
			L2CheckIndex = searchL2(adr, this->L2);
			if(L2CheckIndex != -1){  
				this -> L2[rowInd][L2CheckIndex].data = *data; myMem[adr] = *data; //Update Value in L2 and Mem
				L2lruUpdate(rowInd, L2CheckIndex, this->L2); // Update L2 lru index;
			} 
			else{					//Also miss in L2, do nothing in the cache, only update the main mem
				myMem[adr] = *data;
			}
			}
		}
	}
	else{							//Load instruction, we search for data in upper level hierarchy first
		L1CheckIndex = searchL1(adr,this->L1);
		if (L1CheckIndex!=-1){ 		// Hit in L1
			update(this, L1CheckIndex, VicCheckIndex, L2CheckIndex);
		}
		else{						// Miss in L1, check in Victim(L1CheckIndex == -1 in first case)
			VicCheckIndex = searchVic(adr,this->Victim);
			if(VicCheckIndex != -1){//Hit in Victim
				cacheBlock temp = L1[rowInd];						//Swape the cacheBlocks in the victim and L1 
				L1victimReplace(L1[rowInd], Victim[VicCheckIndex]);
				L1victimReplace(Victim[VicCheckIndex], temp);
				update(this, L1CheckIndex, VicCheckIndex, L2CheckIndex);  // Update stat and victim lru position
				ViclruUpdate(VicCheckIndex,this->Victim);
			}
			else{				 	// Miss in Vic, check in L2(L2Hit in first case)
				L2CheckIndex = searchL2(adr,this->L2);	
				if(L2CheckIndex != -1){ // Hit L2, bring data to L1(Remove from L2), update Lru, tag, evicted line L1 -> victim, evicted line victim -> L2
				update(this, L1CheckIndex,VicCheckIndex,L2CheckIndex);
				cacheBlock Hit = L2Evict(rowInd, L2CheckIndex, L2);
				cacheBlock L1temp = L1VicEvict(rowInd,L1);
					if (L1temp.valid == 0) {L1[rowInd] = Hit;} //if L1 fails to evict(empty), directly store the hit block into L1.
                    else{//L1 evict something, store the hit block into L1 and store L1temp to vic,vic to L2
						L1[rowInd] = Hit;
                        bool Vic_empty = false;
                        for(int i=0;i<VICTIM_SIZE;i++){
                            if (Victim[i].valid == 0){Vic_empty = true;Victim[i] = L1temp; ViclruUpdate(i,Victim);break;}} //If Victim is empty, fill in.
							if (Vic_empty == false){//Victim not empty, replace the one with the least lru(0)
								for(int m=0;m<VICTIM_SIZE;m++){
									if (Victim[m].lru_position == 0) {
										cacheBlock Victemp = L1VicEvict(m,Victim);
										Victim[m] = L1temp; ViclruUpdate(m,Victim); 
                                        bool L2_empty = false;  //Handle Vicevict into L2 below:
                                        for(int j=0;j<L2_CACHE_WAYS;j++){ // fill L2 if L2 has empty
                                            if(L2[Victemp.index][j].valid==0)
                                            {L2_empty=true;L2[Victemp.index][j] = Victemp;L2lruUpdate(Victemp.index,j,L2);break;}}
                                        if(L2_empty == false){
                                            for(int k=0;k<L2_CACHE_WAYS;k++){ if(L2[Victemp.index][k].lru_position==0){L2[Victemp.index][k] = Victemp;L2lruUpdate(Victemp.index,k,L2);break;}}
										} //First evict the one with 0lru, fill in and update lru
                                    break;}
                                }
							}			 								
						}
					}
				
				else{					// Miss in all caches, bring from main memory to L1. (Just Update tag and data) 
					cacheBlock L1temp = L1VicEvict(rowInd,L1);
					update(this,-1,-1,-1);
                    if (L1temp.valid == 0) {L1[rowInd].tag = (adr >> 6); L1[rowInd].data = *data; L1[rowInd].valid=1; L1[rowInd].index = rowInd; L1[rowInd].lru_position=0;} //if L1 fails to evict(empty), directly change tag and data bit without doing anything to the rest of the cache
					else{				//L1 evict something, renew data and evict L1temp to vic
                        L1[rowInd].tag = (adr >> 6); L1[rowInd].data = *data; L1[rowInd].valid = 1;
																//Victim not empty, replace the one with the least lru(0)
                                bool Vic_empty = false;
                                for(int i=0;i<VICTIM_SIZE;i++){
                                    if (Victim[i].valid == 0){Vic_empty = true;Victim[i] = L1temp; ViclruUpdate(i,Victim);break;}} //If Victim is empty, fill in.
                                    if (Vic_empty == false){//Victim not empty, replace the one with the least lru(0)
                                        for(int m=0;m<VICTIM_SIZE;m++){
                                            if (Victim[m].lru_position == 0) {
                                                cacheBlock Victemp = L1VicEvict(m,Victim);
                                                Victim[m] = L1temp; ViclruUpdate(m,Victim);
                                                bool L2_empty = false;  //Handle Vicevict into L2 below:
                                                for(int j=0;j<L2_CACHE_WAYS;j++){ // fill L2 if L2 has empty
                                                    if(L2[Victemp.index][j].valid==0)
                                                    {L2_empty=true;L2[Victemp.index][j] = Victemp;L2lruUpdate(Victemp.index,j,L2);break;}}
                                                if(L2_empty == false){
                                                    for(int k=0;k<L2_CACHE_WAYS;k++){ if(L2[Victemp.index][k].lru_position==0){L2[Victemp.index][k] = Victemp;L2lruUpdate(Victemp.index,k,L2);break;}}
                                                } //First evict the one with 0lru, fill in and update lru
                                            break;}
                                        }
                                    }
						
					}
				}
			}
		}
	}
}
