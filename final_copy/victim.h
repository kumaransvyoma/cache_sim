#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#include <vector>
#include <bitset>
#include <algorithm>
// #include "victim.h"
using namespace std;

#define ADDRESS_SIZE 32

struct DATASTORE
{
    
    bool isValid;
    bool isDirty;
};

struct TAGSTORE
{
    long long *TAG; // TAG value
    int *lru;
    DATASTORE *dataStore;
};

class VC_CACHE
{
public:

    unsigned long SIZE;
    unsigned long ASSOC;
    unsigned long BLOCKSIZE;
    unsigned long NUM_SETS;
    unsigned long INDEX_SIZE;
    unsigned long block_offset_size;
    unsigned long tag_bits;
    // unsigned long nextLevel;
    // CACHE *nextlevel;

    unsigned long TAG_MASK = 0;
    unsigned long INDEX_MASK = 0;
    unsigned long BLOCK_OFFSET_MASK = 0;
    bool hit;
    unsigned long tag_value;
    unsigned long block_value;
    unsigned long READ_HIT = 0;
    unsigned long READ_MISS = 0;
    unsigned long Reads = 0;
    unsigned long writes = 0;
    unsigned long WRITE_HIT = 0;
    unsigned long WRITE_MISS = 0;
    unsigned long SWAP_HITS = 0;
    bool swap = false;
    unsigned long temp;
    unsigned long replaced_tag;
    VC_CACHE() = default;

    struct TAGSTORE **tagStore;
    bool isPowerOfTwo(unsigned long x)
    {
        return (x > 0) && ((x & (x - 1)) == 0);
    }

    VC_CACHE(unsigned long size, unsigned long assoc, unsigned long blockSize)
        : SIZE(size), ASSOC(assoc), BLOCKSIZE(blockSize)  

{
    // struct TAGSTORE **tagStore;
        if (!isPowerOfTwo(BLOCKSIZE))
        {
            cerr << "Error: BLOCKSIZE must be a power of two." << endl;
            exit(EXIT_FAILURE);
        }

        NUM_SETS = SIZE / (BLOCKSIZE * ASSOC);
        if (NUM_SETS != 1 && !isPowerOfTwo(NUM_SETS))
        {
            cerr << "Error: Number of sets must be a power of two." << endl;
            exit(EXIT_FAILURE);
        }

        INDEX_SIZE = log2(NUM_SETS);
        block_offset_size = log2(BLOCKSIZE);
        tag_bits = 32 - INDEX_SIZE - block_offset_size;

        BLOCK_OFFSET_MASK = generateBlockOffsetMask(block_offset_size);
        INDEX_MASK = generateIndexMask(INDEX_SIZE, block_offset_size);
        TAG_MASK = generateTagMask(tag_bits, INDEX_SIZE, block_offset_size);

        tagStore = new TAGSTORE *[NUM_SETS];
            for (unsigned long i = 0; i < NUM_SETS; i++) {
                 tagStore[i] = new TAGSTORE();

                tagStore[i]->TAG = (long long*)malloc(ASSOC * sizeof(long long));  // Correct: Use long long type for TAG
                tagStore[i]->lru = (int*)malloc(ASSOC * sizeof(int));
                tagStore[i]->dataStore = new DATASTORE[ASSOC];  // Allocate array of DATASTORE for each set
                for (int j = 0; j < ASSOC; j++) {
                    tagStore[i]->dataStore[j].isValid = false;
                    tagStore[i]->dataStore[j].isDirty = false;
                }

                fill(tagStore[i]->lru, tagStore[i]->lru + ASSOC, -1);
            }
    }


void writeToAddress(unsigned long address)
{
        unsigned long TAG = (address & TAG_MASK) >> (32 - tag_bits);
        unsigned long index = (address & INDEX_MASK) >> block_offset_size;
        unsigned long blockoffset = address & BLOCK_OFFSET_MASK;
        writes++;
        bool hit = false;
            for (unsigned long k = 0; k < ASSOC; k++)
            {       
            if ((tagStore[index]->TAG[k]) == TAG )
            {
                WRITE_HIT++;
                hit = true;
            tagStore[index]->dataStore[k].isDirty = true;

            updatelru(index,TAG,true);
            break;
            }
            }
                if(!hit){
                WRITE_MISS++;
                updatelru(index,TAG,true);
                }
}
    void readFromAddress(unsigned long address)
    {
        unsigned long TAG = (address & TAG_MASK) >> (32 - tag_bits);
        unsigned long index = (address & INDEX_MASK) >> block_offset_size;
        unsigned long blockoffset = address & BLOCK_OFFSET_MASK;
        bool hit = false;
        Reads++;
        for (unsigned long j = 0; j < ASSOC; j++)
        {
             if ((tagStore[index]->TAG[j]) == TAG )
            {
                
                READ_HIT++;
                hit = true;
                // if (nextlevel == NULL){
                //     SWAP = true
                //     L1.
                // }
                tag_value=TAG;
                block_value=j;
                refreshLruOnHit(index, TAG, j);
                // tagStore[index]->dataStore[k].isValid = true;
                // tagStore[index]->dataStore[k].isDirty = false;
                break;
            }
        }
            // else {
            if (!hit)
        {

                READ_MISS++;
                // requesttonextr(address);
            
                // cout << dec <<*(tagStore[index]->lru ) << endl; 
                // if (nextlevel != NULL){
                updatelru(index,TAG,false);
                // }
        }

    }

    void refreshLruOnHit(int indexSet, unsigned updatedTag, int posHit) {
        
        int oldLruValue = tagStore[indexSet]->lru[posHit];
        tagStore[indexSet]->lru[posHit] = 0;
        tagStore[indexSet]->TAG[posHit] = updatedTag;
        for (int entry = 0; entry < ASSOC; ++entry) {
            if (entry != posHit && tagStore[indexSet]->lru[entry] < oldLruValue && tagStore[indexSet]->lru[entry] >= 0) {

                tagStore[indexSet]->lru[entry]++;
            }
        }
}

    void updatelru(int index, unsigned tag,bool write){
                int* maxElementPtr = max_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int* minElementPtr = min_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int maxIndex = maxElementPtr - tagStore[index]->lru;
                // cout << "maxindex\n" << minElementPtr;
            // int minIndex = minElementPtr - tagstore[index].lru;
                int replaced_cell;
        for(int j=0;j<ASSOC;j++){
            if(tagStore[index]->TAG[j]==tag){
                refreshLruOnHit(index, tag, j);
                return;
            }

        }
            if(*minElementPtr < 0){
            for(int i=0;i<ASSOC;i++){
                if(tagStore[index]->lru[i]<0){    
                    tagStore[index]->lru[i]=0; 
                    tagStore[index]->TAG[i]=tag;
                    replaced_cell = i;
                    // cout << "lru\t" << tagStore[index]->lru  ;
                    break;
                }
            }
        }
        // end

        else{
            // cout<<"Inside after cache"<<endl;
            tagStore[index]->lru[maxIndex]=0;
            tagStore[index]->TAG[maxIndex]=tag;
            if(write){
            tagStore[index]->dataStore[maxIndex].isValid = true;
            tagStore[index]->dataStore[maxIndex].isDirty = true;
            }
            if(write ==false){
                tagStore[index]->dataStore[maxIndex].isDirty = false;
                tagStore[index]->dataStore[maxIndex].isValid = true;
            }
            replaced_cell =maxIndex;
        }

        for(int j=0;j<ASSOC;j++){
            if(tagStore[index]->lru[j]>=0 && replaced_cell!=j)
                tagStore[index]->lru[j] = tagStore[index]->lru[j] + 1;
        }
        
}
    unsigned long write( unsigned tag){
                int index =0;
                int* maxElementPtr = max_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int* minElementPtr = min_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int maxIndex = maxElementPtr - tagStore[index]->lru;
                int replaced_cell;

            if(*minElementPtr < 0){
            for(int i=0;i<ASSOC;i++){
                if(tagStore[index]->lru[i]<0){    
                    tagStore[index]->lru[i]=0; 
                    tagStore[index]->TAG[i]=tag;
                    replaced_cell = i;
                    // cout << "lru\t" << tagStore[index]->lru  ;
                    break;
                }
            }
        }

        else {
            int isDuplicate = 1;
           
            for (int slot = 0; slot < ASSOC; slot++) {
                if (tagStore[0]->TAG[slot] == tag) {
                    isDuplicate = 0;
                    replaced_tag = tagStore[0]->TAG[slot];
                    tagStore[0]->TAG[slot] = tag;
                    replaced_cell = slot;
                }
            }
            
            if (isDuplicate == 1) {
                tagStore[0]->lru[maxIndex] = 0;
                replaced_tag = tagStore[0]->TAG[maxIndex];
                tagStore[0]->TAG[maxIndex] = tag;
                replaced_cell = maxIndex;
            }
        }


        for(int j=0;j<ASSOC;j++){
            if(tagStore[index]->lru[j]>=0 && replaced_cell!=j)
                tagStore[index]->lru[j] = tagStore[index]->lru[j] + 1;
        }
        return replaced_tag;
    }
    void CacheStatus()
    {
        cout << "\n\n ===== VC contents =====";
        for (unsigned long i = 0; i < NUM_SETS; i++)
        {
            cout << "\nSet " << dec << i << ":\t";
            for (unsigned long j = 0; j < ASSOC; j++)
            {

                if (tagStore[i]->TAG[j] == -1)
                {
                    cout << "[0]";
                }
                else
                {
                    cout << hex << tagStore[i]->TAG[j] << "\t" ;
                }

                if (tagStore[i]->dataStore[j].isDirty == true)
                {
                    cout << " D ";
                }
                if (tagStore[i]->dataStore[j].isDirty == false)
                {
                    cout << " \t";
                }
            }
        }
    }
    bool ispresent(unsigned int tag)
    {
        for (int i = 0; i < ASSOC; i++)
        {
            if (tagStore[0]->TAG[i] == tag)
            {
                // VC_HIT++;
                return write(tag);
            }
        }
        return 0;
    }

            unsigned long generateBlockOffsetMask(unsigned long size) {
            unsigned long mask = 0;
            for (unsigned long count = 0; count < size; ++count) {
                mask = (mask << 1) | 1;  // Shift mask left by 1 and set the lowest bit to 1
            }
            return mask;
        }

        unsigned long createMask(unsigned long size) 
        {
            return (1UL << size) - 1; 
        }

     
        unsigned long generateIndexMask(unsigned long indexSize, unsigned long blockOffsetSize) {
            unsigned long mask = createMask(indexSize);  
            return mask << blockOffsetSize; 
        }

        
        unsigned long generateTagMask(unsigned long tagBits, unsigned long indexSize, unsigned long blockOffsetSize) {
            unsigned long mask = createMask(tagBits);  
            return mask << (indexSize + blockOffsetSize);  
        }
};