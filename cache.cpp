#include <stdio.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#include <vector>
// #include "CACHE.h"
using namespace std;

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
    unsigned long* TagSelect;   //selection bit array
    bool *isValid;
    bool *isDirty;
};

struct TAGSTORE
{
    long long* TAG;         //TAG array of size ADDR_TAGS
    unsigned long frequency;
    DATASTORE dataStore;
};

class CACHE {
private:
    unsigned long SIZE;           
    unsigned long ASSOC;          
    unsigned long BLOCKSIZE;      
    unsigned long NUM_SETS;       
    unsigned long INDEX_SIZE;
    unsigned long block_offset_size;
    unsigned long tag_bits;
    unsigned long tagsize;
    unsigned long datasize;
    unsigned long data_blocks;
    struct TAGSTORE **tagStore;
    // unsigned long SETS;

    std::vector<std::vector<int>> tagArray; 
    std::vector<std::vector<int>> dataArray;
    std::vector<std::vector<int>> lruCounter; 

    bool isPowerOfTwo(int x) {
        return (x > 0) && ((x & (x - 1)) == 0);
    }

public:
    CACHE() {
        // Default constructor logic
    }
    CACHE(int size, int assoc, int blockSize) : SIZE(size), ASSOC(assoc), BLOCKSIZE(blockSize) {
        if (!isPowerOfTwo(BLOCKSIZE)) {
            std::cerr << "Error: BLOCKSIZE must be a power of two." << std::endl;
            exit(EXIT_FAILURE);
        }
        // cout<<SIZE<<" "<<BLOCKSIZE<<" "<<ASSOC<<" \n";
        NUM_SETS = SIZE / (BLOCKSIZE * assoc);
        std::cout << NUM_SETS << std::endl;
        data_blocks=8;
        if (NUM_SETS!=1){
        if (!isPowerOfTwo(NUM_SETS)) {
            std::cerr << "Error: Number of sets must be a power of two." << std::endl;
            exit(EXIT_FAILURE);
        }}
        INDEX_SIZE = std::log2(NUM_SETS);
        std::cout << INDEX_SIZE << std::endl;
        block_offset_size = std::log2(BLOCKSIZE);
        tag_bits = 32 - INDEX_SIZE - block_offset_size;
        std::cout << tag_bits << std::endl;

        tagStore = new TAGSTORE *[NUM_SETS];
        for (unsigned long i = 0; i < NUM_SETS; i++)
        {
            tagStore[i] = new TAGSTORE[ASSOC];
        }
        // std::cout<<"wow"<<tagStore[1][0].TAG[0]<<std::endl;

        for (unsigned long i = 0; i < NUM_SETS; i++)
        {
            for (unsigned long j = 0; j < ASSOC; j++)
            {
                // tagStore[i][j].TAG = new long long[ADDR_TAGS];
                // for (unsigned long k = 0; k < ADDR_TAGS; k++)
                // {
                //     tagStore[i][j].TAG[k] = vacant;
                // }
                tagStore[i][j].frequency = j;

                tagStore[i][j].dataStore.TagSelect = new unsigned long[data_blocks];
                tagStore[i][j].dataStore.isDirty = new bool[data_blocks];
                tagStore[i][j].dataStore.isValid = new bool[data_blocks];
                for (unsigned long k = 0; k < data_blocks; k++)
                {
                    tagStore[i][j].dataStore.TagSelect[k] = 0;
                    tagStore[i][j].dataStore.isDirty[k] = false;
                    tagStore[i][j].dataStore.isValid[k] = false;
                }
            }
        }
        // std::cout<< "wow" << tagStore[1][0].dataStore.TagSelect[1] << std::endl;
        for (unsigned long i = 0; i < NUM_SETS; i++) {
            for (unsigned long j = 0; j < ASSOC; j++) {
                if (tagStore[i][j].dataStore.TagSelect == nullptr) {
                    std::cerr << "Error: TagSelect for set " << i << ", way " << j << " is not allocated." << std::endl;
                } else {
                    std::cout << "TagSelect[" << i << "][" << j << "] allocated successfully." << std::endl;
                }
            }
        }


        
    };
};
int main(int argc, char *argv[])
{

    L1_SIZE = strtoul(argv[1], 0, 10);
    L1_ASSOC = strtoul(argv[2], 0, 10);
    L1_BLOCKSIZE = strtoul(argv[3], 0, 10);
    L2_BLOCKSIZE = L1_BLOCKSIZE;
    VC_NUM_BLOCKS = strtoul(argv[4], 0, 10);
    L2_SIZE = strtoul(argv[5], 0, 10);
    L2_ASSOC = strtoul(argv[6], 0, 10);
    Trace_file = argv[7];

    cout<< "=====Simulator configuration=====";
    cout<< "\n L1_SIZE:\t" << L1_SIZE;
    cout<< "\n L1_ASSOC:\t" << L1_ASSOC;
    cout<< "\n L1_BLOCKSIZE:\t" << L1_BLOCKSIZE;
    cout<< "\n VC_NUM_BLOCKS:  " << VC_NUM_BLOCKS;
    cout<< "\n L2_SIZE:\t" << L2_SIZE;
    cout<< "\n L2_ASSOC:\t" << L2_ASSOC;
    cout<< "\n trace_file:\t" << Trace_file << "\n";
    // VC_ASSOC = VC_NUM_BLOCKS / L1_BLOCKSIZE;
    VC_SIZE=VC_NUM_BLOCKS * L1_BLOCKSIZE;
    CACHE L1;
    CACHE VC;
    CACHE L2;


    if (L2_SIZE != 0 && VC_NUM_BLOCKS != 0)
    {
        L1 = CACHE( L1_SIZE,L1_ASSOC,L1_BLOCKSIZE);
        VC = CACHE( VC_SIZE,VC_NUM_BLOCKS,L1_BLOCKSIZE);
        L2 = CACHE( L2_SIZE, L2_ASSOC, L1_BLOCKSIZE);
        // L1 = CACHE( 1024,2,16);
        // VC = CACHE( 32,32,16);
        // L2 = CACHE( 8192, 4, 16);

    }
    else if(L2_SIZE == 0 && VC_NUM_BLOCKS != 0)
    {
        L1 = CACHE( L1_SIZE,L1_ASSOC,L1_BLOCKSIZE);
        VC = CACHE( VC_NUM_BLOCKS,VC_NUM_BLOCKS,L1_BLOCKSIZE);
    }
    else if(L2_SIZE != 0 && VC_NUM_BLOCKS == 0)
    {
        L1 = CACHE( L1_SIZE,L1_ASSOC,L1_BLOCKSIZE);
        L2 = CACHE( L2_SIZE, L2_ASSOC, L1_BLOCKSIZE);
    }
    else 
    {
        L1 = CACHE( L1_SIZE,L1_ASSOC,L1_BLOCKSIZE);
        
    }
    // Cache myCache(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE);

}

