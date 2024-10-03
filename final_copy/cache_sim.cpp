#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#include <vector>
#include <bitset>
#include <algorithm>
#include "victim.h"
using namespace std;


#define ADDRESS_SIZE 32

unsigned long L1_BLOCKSIZE;
unsigned long L1_SIZE;
unsigned long L1_ASSOC;

unsigned long VC_SIZE;
unsigned long VC_NUM_BLOCKS;
unsigned long VC_ASSOC;

unsigned long L2_BLOCKSIZE;
unsigned long L2_SIZE;
unsigned long L2_ASSOC;

char *Trace_file;


class CACHE
{
private:
    unsigned long SIZE;
    unsigned long ASSOC;
    unsigned long BLOCKSIZE;
    unsigned long NUM_SETS;
    unsigned long INDEX_SIZE;
    unsigned long block_offset_size;
    unsigned long tag_bits;
    // unsigned long nextLevel;
    CACHE *nextlevel;

    unsigned long TAG_MASK = 0;
    unsigned long INDEX_MASK = 0;
    unsigned long BLOCK_OFFSET_MASK = 0;

    struct TAGSTORE **tagStore;
    bool isPowerOfTwo(unsigned long x)
    {
        return (x > 0) && ((x & (x - 1)) == 0);
    }

public:
    unsigned long READ_HIT = 0;
    unsigned long READ_MISS = 0;
    unsigned long Reads = 0;
    unsigned long writes = 0;
    unsigned long WRITE_HIT = 0;
    unsigned long WRITE_MISS = 0;
    unsigned long VC_HIT;
    unsigned long SWAP_HITS = 0;
    unsigned long WRITE_BACKS =0;
    bool swap = false;
    unsigned long temp;
    unsigned long vc_blocks;
    unsigned long addressofread;

    VC_CACHE *VC;
    CACHE() = default;

    CACHE(unsigned long size, unsigned long assoc, unsigned long blockSize, unsigned long VC_BLOCKS)
        : SIZE(size), ASSOC(assoc), BLOCKSIZE(blockSize), vc_blocks(VC_BLOCKS)
    {

        if (!isPowerOfTwo(BLOCKSIZE))
        {
            cerr << "Error: Blocksize is not power of two." << endl;
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


        // VC_CACHE VC;
        if (vc_blocks != 0)
        {
            
            VC = new VC_CACHE(vc_blocks * BLOCKSIZE, vc_blocks, BLOCKSIZE);
        }
        tagStore = new TAGSTORE *[NUM_SETS];
        for (unsigned long i = 0; i < NUM_SETS; i++)
        {
            tagStore[i] = new TAGSTORE();

            tagStore[i]->TAG = (long long *)malloc(ASSOC * sizeof(long long));
            tagStore[i]->lru = (int *)malloc(ASSOC * sizeof(int));
            tagStore[i]->dataStore = new DATASTORE[ASSOC]; 
            for (int j = 0; j < ASSOC; j++)
            {
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
            if ((tagStore[index]->TAG[k]) == TAG)
            {
                WRITE_HIT++;
                hit = true;
                tagStore[index]->dataStore[k].isDirty = true;

                updatelru(index, TAG, true);
                break;
            }
        }
        if (vc_blocks != 0)
        {
            VC->readFromAddress(address);
            if (VC->hit == true)
            {
                VC_HIT++;
                int *maxElementPtr = max_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int maxIndex = maxElementPtr - tagStore[index]->lru;
                temp = tagStore[index]->TAG[maxIndex];
                tagStore[index]->TAG[maxIndex] = VC->tag_value;
                VC->tagStore[0]->TAG[VC->block_value] = true;
                SWAP_HITS++;
                // VC->updatelru(0, tag, false);
                READ_MISS++;
                return;
            }
        }
        if (!hit)
        {
            WRITE_MISS++;
            updatelru(index, TAG, true);
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
            if ((tagStore[index]->TAG[j]) == TAG)
            {

                READ_HIT++;
                hit = true;
                // if (nextlevel == NULL){
                //     SWAP = true
                //     L1.
                // }
                refreshLruOnHit(index, TAG, j);
                // tagStore[index]->dataStore[k].isValid = true;
                // tagStore[index]->dataStore[k].isDirty = false;
                break;
            }
        }
        if (vc_blocks != 0)
        {
            VC->readFromAddress(address);
            if (VC->hit == true)
            {
                SWAP_HITS++;
                temp = VC->ispresent((TAG << (INDEX_SIZE)) | (index));
                int *maxElementPtr = max_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int maxIndex = maxElementPtr - tagStore[index]->lru;
                // temp = tagStore[index]->TAG[maxIndex];
                // addressofread = (tagStore[index]->TAG[maxIndex] << (INDEX_SIZE + block_offset_size)) | (index << block_offset_size) | 0;
                temp = temp >> (INDEX_SIZE);
                tagStore[index]->TAG[maxIndex] = temp;
                refreshLruOnHit(index, temp, maxIndex);
                // VC->tagStore[0]->TAG[j] = true;
                // SWAP_HITS++;
                // VC->updatelru(0, tag, false);
                READ_MISS++;
                VC_HIT++;
                return;
            }
        }
        // else {
        if (!hit)
        {

            READ_MISS++;
            updatelru(index, TAG, false);
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

    void updatelru(int index, unsigned tag, bool write)
    {
        int *maxElementPtr = max_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
        int *minElementPtr = min_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
        int maxIndex = maxElementPtr - tagStore[index]->lru;
        // cout << "maxindex\n" << minElementPtr;
        // int minIndex = minElementPtr - tagstore[index].lru;
        int replaced_cell;
        unsigned long replaced_block =0;

        for (int entry = 0; entry < ASSOC; entry++) {

            if (tagStore[index]->TAG[entry] == tag) {

                refreshLruOnHit(index, tag, entry);
                return;
            }
        }


        if (*minElementPtr < 0) {
            for (int slot = 0; slot < ASSOC; slot++) {

                if (tagStore[index]->lru[slot] < 0) {
                    tagStore[index]->lru[slot] = 0;
                    tagStore[index]->TAG[slot] = tag;
                    replaced_cell = slot;
                    break;
                }
            }
        }


        else
        {
            
            tagStore[index]->lru[maxIndex] = 0;

            if (vc_blocks !=0){
            replaced_block = tagStore[index]->TAG[maxIndex];
            }
            if(tagStore[index]->dataStore[maxIndex].isDirty == true){
                WRITE_BACKS++;
            }
            tagStore[index]->TAG[maxIndex] = tag;
            replaced_cell = maxIndex;
            if (write)
            {
                
                tagStore[index]->dataStore[maxIndex].isValid = true;
                tagStore[index]->dataStore[maxIndex].isDirty = true;
            }
            if (write == false)
            {
                tagStore[index]->dataStore[maxIndex].isDirty = false;
                tagStore[index]->dataStore[maxIndex].isValid = true;
            }
            replaced_cell = maxIndex;
        }

        for (int j = 0; j < ASSOC; j++)
        {
            if (tagStore[index]->lru[j] >= 0 && replaced_cell != j)
           
                tagStore[index]->lru[j] = tagStore[index]->lru[j] + 1;
        }
        if (vc_blocks !=0 && replaced_block !=0)
        {
            VC->write((replaced_block << (INDEX_SIZE)) | (replaced_cell << INDEX_SIZE));
        }
    }
    void CacheStatus()
    {
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
                    cout << hex << tagStore[i]->TAG[j];
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
        if (vc_blocks !=0){
        VC->CacheStatus();
        }
        float MISS_RATE = (float(WRITE_MISS) + float(READ_MISS) )/((Reads)+(writes));
        // READ_MISS = Reads - READ_HIT;
        // WRITE_MISS = writes - WRITE_HIT;
        // cout << dec << "\nno of reads : " << Reads
        //      << "\nno of read misses : " << READ_MISS
        //      << "\nno of read hits : " << READ_HIT
        //      << "\n\nno of Writes : " << writes
        //      << "\n\nno of Writes misses : " << WRITE_MISS 
        //      << "\n\nno of Writes backs : " << WRITE_BACKS << "\n"; 
            //   << "\nno of swaps : " << VC->READ_MISS<< "\n";
        cout << "\n\n===== Simulation Results =====";
        cout << "\na. number of L1 reads:\t\t\t" << dec << Reads;
        cout << "\nb. number of L1 read misses:\t\t" << dec << READ_MISS;
        cout << "\nc. number of L1 writes:\t\t\t" << dec << writes;
        cout << "\nd. number of L1 write misses:\t\t" << dec << WRITE_MISS;
        cout << "\ne. number of swap requests:\t\t" << 0;        
        cout << "\nf. swap request rate:\t\t" << dec << 0;
        cout << "\ng. number of swaps:\t\t "<< dec << 0;
        cout << "\nh. h. combined L1+VC miss rate:\t\t" << dec << MISS_RATE;
        cout << "\ni. number writebacks from L1/VC:\t" << dec << WRITE_BACKS;
        cout << "\nj. number of L2 reads:\t\t" << 0;
        cout << "\nk. number of L2 read misses:\t\t" << 0;
        cout << "\nl. number of L2 writes:\t\t"  << 0;
        cout << "\nm. number of L2 write misses:\t\t" << 0 ;
        cout << "\nn. L2 miss rate:\t\t\t" << 0;
        cout << "\no. number of writebacks from L2:\t" << dec << 0;
        cout << "\np. total memory traffic:\t\t" << READ_MISS + WRITE_MISS + WRITE_BACKS<< "\n";
        cout << "\n===== Simulation results (performance) =====";
        cout << "\n1. average access time:\t\t" << 0;
        cout << "\n2. energy-delay product:\t" << 0;
        cout << "\n3. total area:\t\t\t" << 0 <<"\n";

    }

        unsigned long generateBlockOffsetMask(unsigned long size) {
            unsigned long mask = 0;
            for (unsigned long count = 0; count < size; ++count) {
                mask = (mask << 1) | 1;  
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

int main(int argc, char *argv[])
{
    if (argc < 8)
    {
        cerr << "Usage: " << argv[0] << " <L1_SIZE> <L1_ASSOC> <L1_BLOCKSIZE> <VC_NUM_BLOCKS> <L2_SIZE> <L2_ASSOC> <Trace_file>" << endl;
        return 1;
    }

    L1_SIZE = strtoul(argv[1], 0, 10);
    L1_ASSOC = strtoul(argv[2], 0, 10);
    L1_BLOCKSIZE = strtoul(argv[3], 0, 10);
    L2_BLOCKSIZE = L1_BLOCKSIZE;
    VC_NUM_BLOCKS = strtoul(argv[4], 0, 10);
    L2_SIZE = strtoul(argv[5], 0, 10);
    L2_ASSOC = strtoul(argv[6], 0, 10);
    Trace_file = argv[7];

    cout << "===== Simulator configuration =====" << endl;
    cout << "L1_SIZE:\t" << L1_SIZE << endl;
    cout << "L1_ASSOC:\t" << L1_ASSOC << endl;
    cout << "L1_BLOCKSIZE:\t" << L1_BLOCKSIZE << endl;
    cout << "VC_NUM_BLOCKS:\t" << VC_NUM_BLOCKS << endl;
    cout << "L2_SIZE:\t" << L2_SIZE << endl;
    cout << "L2_ASSOC:\t" << L2_ASSOC << endl;
    cout << "Trace_file:\t" << Trace_file << endl;
    // VC_SIZE=VC_NUM_BLOCKS * L1_BLOCKSIZE;
    CACHE L1;
    // CACHE VC;
    // CACHE L2;

    // if (L2_SIZE != 0 && VC_NUM_BLOCKS != 0)
    // {
    //     L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE,&VC);
    //     VC = CACHE(VC_NUM_BLOCKS * L1_BLOCKSIZE, VC_NUM_BLOCKS, L1_BLOCKSIZE,&L2);
    //     L2 = CACHE(L2_SIZE, L2_ASSOC, L1_BLOCKSIZE,NULL);
    // }
    if (L2_SIZE == 0 && VC_NUM_BLOCKS != 0)
    {
        L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE, VC_NUM_BLOCKS);
        // VC = VC_CACHE(VC_NUM_BLOCKS * L1_BLOCKSIZE, VC_NUM_BLOCKS, L1_BLOCKSIZE,NULL);
    }
    // else if (L2_SIZE != 0 && VC_NUM_BLOCKS == 0)
    // {
    //     L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE, &L2);
    //     L2 = CACHE(L2_SIZE, L2_ASSOC, L1_BLOCKSIZE,NULL);
    // }
    else
    {
        L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE, 0);
    }

    FILE *fp = fopen(Trace_file, "r");
    if (fp == nullptr)
    {
        cerr << "Error opening trace file." << endl;
        return 1;
    }

    char operation;
    unsigned long address;
    stringstream ss;

    while (fscanf(fp, " %c %lx", &operation, &address) != EOF)
    {
        if (operation == 'r' || operation == 'R')
        {
            L1.readFromAddress(address);
            // cout << "Trace_file woooooooow:\t"  << endl;
        }
        if (operation == 'w' || operation == 'w')
        {
            L1.writeToAddress(address);
        }
    }

    fclose(fp);
    cout << "\n===== L1 contents =====";
    L1.CacheStatus();

    return 0;
}
