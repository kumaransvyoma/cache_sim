#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#include <vector>
#include <bitset>
#include <algorithm>
using namespace std;

#define vacant -1
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

    unsigned long TAG_MASK = 0;
    unsigned long INDEX_MASK = 0;
    unsigned long BLOCK_OFFSET_MASK = 0;


    unsigned long CACHE_BLOCK_MISS = 0;
    unsigned long SECTOR_MISS = 0;

    // vector<vector<TAGSTORE>> tagStore;
    struct TAGSTORE **tagStore;
    bool isPowerOfTwo(unsigned long x)
    {
        return (x > 0) && ((x & (x - 1)) == 0);
    }

    // void resetlru(unsigned long index, unsigned long lru)
    // {
    //     for (unsigned long j = 0; j < ASSOC; j++)
    //     {
    //         if (tagStore[index][j].lru < lru)
    //         {
    //             tagStore[index][j].lru++;
    //         }
    //         else if (tagStore[index][j].lru == lru)
    //         {
    //             tagStore[index][j].lru = 0;
    //         }
    //     }
    // }
void resetlru(unsigned long index, unsigned long lruIndex)
{
    for (unsigned long j = 0; j < ASSOC; j++)
    {
        // Dereference lru pointer to compare and update its value
        if (*(tagStore[index]->lru + j) < lruIndex)
        {
            (*(tagStore[index]->lru + j))++;
        }
        else if (*(tagStore[index]->lru + j) == lruIndex)
        {
            *(tagStore[index]->lru + j) = 0;  // Reset the LRU counter for this entry
        }
    }
}



    // void checkMissType(unsigned long index, unsigned long assoc)
    // {
    //     bool isBlockMiss = false;
    //     bool isSectorMiss = true;
    //     for (unsigned long k = 0; k < BLOCKSIZE; k++)
    //     {
    //         if (tagStore[index][assoc].dataStore.isValid[k])
    //         {
    //             isBlockMiss = true;
    //             isSectorMiss = false;
    //             break;
    //         }
    //     }
    //     if (isBlockMiss)
    //     {
    //         CACHE_BLOCK_MISS++;
    //     }
    //     else if (isSectorMiss)
    //     {
    //         SECTOR_MISS++;
    //     }
    // }

public:

    unsigned long READ_HIT = 0;
    unsigned long READ_MISS = 0;
    unsigned long Reads = 0;
    unsigned long writes = 0;
    unsigned long WRITE_HIT = 0;
    unsigned long WRITE_MISS = 0;
    CACHE() = default;

    CACHE(unsigned long size, unsigned long assoc, unsigned long blockSize)
        : SIZE(size), ASSOC(assoc), BLOCKSIZE(blockSize)
    {
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

        // Generate BLOCK_OFFSET_MASK
        for (unsigned long i = 0; i < block_offset_size; i++)
        {
            BLOCK_OFFSET_MASK = (BLOCK_OFFSET_MASK << 1) + 1;
        }

        // Generate INDEX_MASK
        for (unsigned long i = 0; i < INDEX_SIZE; i++)
        {
            INDEX_MASK = (INDEX_MASK << 1) + 1;
        }
        INDEX_MASK <<= block_offset_size;

        // Generate TAG_MASK
        for (unsigned long i = 0; i < tag_bits; i++)
        {
            TAG_MASK = (TAG_MASK << 1) + 1;
        }
        TAG_MASK <<= (INDEX_SIZE + block_offset_size);


        tagStore = new TAGSTORE *[NUM_SETS];
            for (unsigned long i = 0; i < NUM_SETS; i++) {
                 tagStore[i] = new TAGSTORE();
                // tagStore[i]->TAG = (unsigned*)malloc(ASSOC * sizeof(long));
                // tagStore[i]->lru = (unsigned*)malloc(ASSOC * sizeof(long));
                tagStore[i]->TAG = (long long*)malloc(ASSOC * sizeof(long long));  // Correct: Use long long type for TAG
                tagStore[i]->lru = (int*)malloc(ASSOC * sizeof(int));
                tagStore[i]->dataStore = new DATASTORE[ASSOC];  // Allocate array of DATASTORE for each set
                for (int j = 0; j < ASSOC; j++) {
                    tagStore[i]->dataStore[j].isValid = false;
                    tagStore[i]->dataStore[j].isDirty = false;
                }
                // tagStore[i]->dataStore.isValid = false;
                // tagStore[i]->dataStore.isDirty = false;
                fill(tagStore[i]->lru, tagStore[i]->lru + ASSOC, -1);
            }


        // Initialize cache
        // tagStore.resize(NUM_SETS, vector<TAGSTORE>(ASSOC));
        // for (unsigned long i = 0; i < NUM_SETS; i++)
        // {
        //     for (unsigned long j = 0; j < ASSOC; j++)
        //     {
        //         tagStore[i][j].TAG = vacant;
        //         tagStore[i][j].lru = j;
        //         tagStore[i][j].dataStore.TagSelect.resize(BLOCKSIZE, vacant);
        //         tagStore[i][j].dataStore.isValid.resize(BLOCKSIZE, false);
        //         tagStore[i][j].dataStore.isDirty.resize(BLOCKSIZE, false);
        //     }
        // }
    }
void writeToAddress(unsigned long address)
{
        unsigned long TAG = (address & TAG_MASK) >> (32 - tag_bits);
        unsigned long index = (address & INDEX_MASK) >> block_offset_size;
        unsigned long blockoffset = address & BLOCK_OFFSET_MASK;
        writes++;
    // for (unsigned long j = 0; j < ASSOC; j++)
    // {
        bool hit = false;
            for (unsigned long k = 0; k < ASSOC; k++)
            {       
            if ((tagStore[index]->TAG[k]) == TAG )
            {

                WRITE_HIT++;
                hit = true;
                // resetFrequency(index, tagStore[index][j].frequency);
                // resetlru(index, *(tagStore[index][j].lru));
                // hit = true;
                
            tagStore[index]->dataStore[k].isValid = true;
            tagStore[index]->dataStore[k].isDirty = true;
            // hitUpdateLru(index,TAG,k);
            updatelru(index,TAG,true);
            break;
            }
            }
                if(!hit){
                WRITE_MISS++;
                // (tagStore[index]->TAG[j]) = TAG;
                updatelru(index,TAG,true);
            // tagStore[index]->dataStore[k].isValid = true;
            // tagStore[index]->dataStore[k].isDirty = true;
                // cout << "Trace_file woooooooow:\t"  << (tagStore[index]->TAG[j]) << endl;
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
                hitUpdateLru(index, TAG, j);
                // tagStore[index]->dataStore[k].isValid = true;
                // tagStore[index]->dataStore[k].isDirty = false;
                break;
            }
        }
            // else {
            if (!hit)
        {
                READ_MISS++;
                
                // cout << dec <<*(tagStore[index]->lru ) << endl; 
                updatelru(index,TAG,false);

                // (tagStore[maxIndex]->TAG[j]) = TAG;
                // cout << "Trace_file woooooooow:\t"  << (tagStore[index]->TAG[j]) << endl;
        }
        

        

        // if (!hit)
        // {
        //     READ_MISS++;
        //     for (unsigned long j = 0; j < ASSOC; j++)
        //     {
        //         if (tagStore[index][j].TAG == vacant)
        //         {
        //             tagStore[index][j].TAG = TAG;
        //             tagStore[index][j].dataStore.isValid[blockoffset] = true;
        //             resetlru(index, tagStore[index][j].lru);
        //             break;
        //         }
        //     }
        // }
        
    }
    void hitUpdateLru(int index, unsigned tag, int hit_cell){
        int old_count = tagStore[index]->lru[hit_cell];
        tagStore[index]->lru[hit_cell]=0;
        tagStore[index]->TAG[hit_cell]=tag;
        int replaced_cell = hit_cell;
        for(int j=0;j<ASSOC;j++){
            if(tagStore[index]->lru[j]>=0 && replaced_cell!=j && tagStore[index]->lru[j]<old_count)
                tagStore[index]->lru[j] = tagStore[index]->lru[j] + 1;
        }
    }
void updatelru(int index, unsigned tag,bool write){
                int* maxElementPtr = max_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int* minElementPtr = min_element(tagStore[index]->lru, tagStore[index]->lru + ASSOC);
                int maxIndex = maxElementPtr - tagStore[index]->lru;
                // cout << "maxindex\n" << minElementPtr;
            // int minIndex = minElementPtr - tagstore[index].lru;
                int replaced_cell;
        for(int j=0;j<L1_ASSOC;j++){
            if(tagStore[index]->TAG[j]==tag){
                hitUpdateLru(index, tag, j);
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
            }
            replaced_cell =maxIndex;
        }

        for(int j=0;j<ASSOC;j++){
            if(tagStore[index]->lru[j]>=0 && replaced_cell!=j)
                tagStore[index]->lru[j] = tagStore[index]->lru[j] + 1;
        }
        
}
    void CacheStatus()
    {
        for (unsigned long i = 0; i < NUM_SETS; i++)
        {
            cout << "\nSet " << dec << i << ":\t";
            for (unsigned long j = 0; j < ASSOC; j++)
            {
                // Find the correct block by lru
                // for (unsigned long k = 0; k < ASSOC; k++)
                // {
                    // if (tagStore[i].lru == j)
                    // {
                        // Print the tag
                        if (tagStore[i]->TAG[j] == vacant)
                        {
                            cout << "[0]";
                        }
                        else
                        {
                            cout << hex << tagStore[i]->TAG[j]   ;
                        }

                        // Print the dirty bit
                        if (tagStore[i]->dataStore[j].isDirty == true) // Using [0] assuming first block for simplicity
                        {
                            cout << " D ";
                        }
                        if(tagStore[i]->dataStore[j].isDirty == false)
                        {
                            cout << " \t";
                        }
                    
                
            }
        }
        //     for(int i=0;i<ASSOC;i++)
        //     cout<<setw(15)<<"BLOCK"<<i
        //         <<setw(15)<<"LRU"<<i;

        // cout<<"\n\n";
        // for(int i=0;i<NUM_SETS;i++){
        //     // cout<<"SET "<<i<<endl;
        //     cout<<"SET "<<i<<"\t";
        //     // cout<<"SET "<<i<<"\t"<<setw(15)<<tagstore[i].tag<<setw(15)<<tagstore[i].index<<setw(15)<<tagstore[i].ofst<<setw(15);
        //     // cout<<"Tag : "<<tagstore[i].tag<<"\n"<<"Index : "<<tagstore[i].index<<"\n"<<"OFST : "<<tagstore[i].ofst<<"\n"<<"BLOCKS : ";
        //     for(int j=0;j<ASSOC;j++){
        //         cout<<hex<<setw(15)<<tagStore[i]->TAG[j]
        //             <<dec<<setw(15)<<tagStore[i]->lru[j];
        //         }
        //     cout<<"\n";
        //     }
        // READ_MISS = Reads - READ_HIT;
        // WRITE_MISS = writes - WRITE_HIT;
            cout<< dec <<"\nno of reads : "<<Reads
            <<"\nno of read misses : "<<READ_MISS
            <<"\nno of read hits : "<<READ_HIT
            <<"\n\nno of Writes : "<<writes
            <<"\n\nno of Writes misses : "<<WRITE_MISS << "\n";
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
    CACHE VC;
    CACHE L2;

    // if (L2_SIZE != 0 && VC_NUM_BLOCKS != 0)
    // {
    //     L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE);
    //     VC = CACHE(VC_NUM_BLOCKS * L1_BLOCKSIZE, VC_NUM_BLOCKS, L1_BLOCKSIZE);
    //     L2 = CACHE(L2_SIZE, L2_ASSOC, L1_BLOCKSIZE);
    // }
    // else if (L2_SIZE == 0 && VC_NUM_BLOCKS != 0)
    // {
    //     L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE);
    //     VC = CACHE(VC_NUM_BLOCKS * L1_BLOCKSIZE, VC_NUM_BLOCKS, L1_BLOCKSIZE);
    // }
    // else if (L2_SIZE != 0 && VC_NUM_BLOCKS == 0)
    // {
    //     L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE);
    //     L2 = CACHE(L2_SIZE, L2_ASSOC, L1_BLOCKSIZE);
    // }
    // else
    // {
        L1 = CACHE(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE);
    // }

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
