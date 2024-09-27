#include <iostream>
#include <bitset>
using namespace std;
#define vacant -1 
struct DATASTORE {
    unsigned long* TagSelect;   //selection bit array
    bool *isValid;
    bool *isDirty;
};

struct TAGSTORE {
    long long TAG;         // Single TAG value, not an array
    unsigned long frequency;
    DATASTORE dataStore;
};

int main() {
    unsigned long TAG_MASK = 0;
    unsigned long INDEX_MASK = 0;
    unsigned long BLOCK_OFFSET_MASK = 0;
    
    unsigned long address = 1073955232;  // Example address
    unsigned long ADDRESS_SIZE = 32;
    unsigned long CACHE_ASSOC = 2;
    unsigned long NUM_SETS = 32;
    unsigned long data_blocks = 16;
    unsigned long READ_HIT = 0;
    
    unsigned long BLOCK_OFFSET_BITS = 4;
    unsigned long INDEX_BITS = 5;
    unsigned long TAG_BITS = 23;
    unsigned long DATA_BLOCK_BITS = 0;  // Set this based on your logic
    
    struct TAGSTORE **tagStore;
    
    // Allocate the cache with NUM_SETS sets and CACHE_ASSOC associativity
    tagStore = new TAGSTORE *[NUM_SETS];
    for (unsigned long i = 0; i < NUM_SETS; i++) {
        tagStore[i] = new TAGSTORE[CACHE_ASSOC];
    }

    // Initialize the cache
    for (unsigned long i = 0; i < NUM_SETS; i++) {
        for (unsigned long j = 0; j < CACHE_ASSOC; j++) {
            tagStore[i][j].frequency = j;

            tagStore[i][j].dataStore.TagSelect = new unsigned long[data_blocks];
            tagStore[i][j].dataStore.isDirty = new bool[data_blocks];
            tagStore[i][j].dataStore.isValid = new bool[data_blocks];
            for (unsigned long k = 0; k < data_blocks; k++) {
                tagStore[i][j].dataStore.TagSelect[k] = 0;
                tagStore[i][j].dataStore.isDirty[k] = false;
                tagStore[i][j].dataStore.isValid[k] = false;
            }
        }
    }

    // Generate BLOCK_OFFSET_MASK
    for (unsigned long i = 0; i < BLOCK_OFFSET_BITS; i++) {
        BLOCK_OFFSET_MASK = (BLOCK_OFFSET_MASK << 1) + 1;
    }

    // Generate INDEX_MASK
    for (unsigned long i = 0; i < INDEX_BITS; i++) {
        INDEX_MASK = (INDEX_MASK << 1) + 1;
    }
    INDEX_MASK = INDEX_MASK << (DATA_BLOCK_BITS + BLOCK_OFFSET_BITS);

    // Generate TAG_MASK
    for (unsigned long i = 0; i < TAG_BITS; i++) {
        TAG_MASK = (TAG_MASK << 1) + 1;
    }
    TAG_MASK = TAG_MASK << (INDEX_BITS + BLOCK_OFFSET_BITS);

    // Extract parts from the address
    unsigned long TAG = (address & TAG_MASK) >> (ADDRESS_SIZE - TAG_BITS);
    unsigned long index = (address & INDEX_MASK) >> (DATA_BLOCK_BITS + BLOCK_OFFSET_BITS);
    unsigned long blockoffset = address & BLOCK_OFFSET_MASK;

    std::cout << "\nTAG (bits):\t\t" << std::bitset<32>(TAG) << std::endl;
    std::cout << "\nindex (bits):\t\t" << std::bitset<32>(index) << std::endl;
    std::cout << "\nblockoffset (bits):\t" << std::bitset<32>(blockoffset) << std::endl;

    // Check for read hit
    for (unsigned long j = 0; j < CACHE_ASSOC; j++) {
        // Compare TAG directly without indexing it
        if (tagStore[index][j].TAG == TAG && tagStore[index][j].dataStore.TagSelect[blockoffset] == TAG && tagStore[index][j].dataStore.isValid[blockoffset]) {
            READ_HIT++;
            break;
        }
        else {
            READ_HIT++;
            // std::cout << "tagStore[index][j].TAG[" << READ_HIT << "] TAG && tagStore[index][j].dataStore.TagSelect[blockoffset]" << ( tagStore[26][0].dataStore.isValid[blockoffset]) <<std::endl;
        }
        
    }

    // std::cout << "  " << tagStore[26][0].TAG << "  " << (TAG && tagStore[index][0].dataStore.TagSelect[blockoffset])<< "  " <<( TAG && tagStore[26][0].dataStore.isValid[blockoffset]) <<std::endl;

    // Clean up memory
    // for (unsigned long i = 0; i < NUM_SETS; i++) {
    //     for (unsigned long j = 0; j < CACHE_ASSOC; j++) {
    //         delete[] tagStore[i][j].dataStore.TagSelect;
    //         delete[] tagStore[i][j].dataStore.isDirty;
    //         delete[] tagStore[i][j].dataStore.isValid;
    //     }
    //     delete[] tagStore[i];
    // }
    // delete[] tagStore;
        //     for (unsigned long i = 0; i < NUM_SETS; i++) {
        //     for (unsigned long j = 0; j < CACHE_ASSOC; j++) {
        //         if (tagStore[i][j].dataStore.TagSelect == nullptr) {
        //             std::cerr << "Error: TagSelect for set " << i << ", way " << j << " is not allocated." << std::endl;
        //         } else {
        //             std::cout << "TagSelect[" << i << "][" << j << "] allocated successfully." << std::endl;
        //         }
        //     }
        // }
    //     if (CACHE_ASSOC > 1 || data_blocks > 1)
    // {
    //     cout << "\n\n===== L2 Address Array contents =====";
    // }
    for (unsigned long i = 0; i < NUM_SETS; i++)
    {
        cout << "\nset" << dec << i << ":\t";
        for (unsigned long j = 0; j < CACHE_ASSOC; j++)
        {
            for (unsigned long k = 0; k < CACHE_ASSOC; k++)
            {
                if (tagStore[i][k].frequency == j)
                {
                    if (CACHE_ASSOC > 1 || data_blocks > 1)
                    {
                        // for (unsigned long l = 0; l < CACHE_ASSOC; l++)
                        // {
                            if (tagStore[i][k].TAG == vacant)
                            {
                                cout << "0";
                            }
                            else
                            {
                                cout << " " << hex << tagStore[i][k].TAG;
                            }
                        if (tagStore[i][k].dataStore.isDirty[k] == true)
                        {
                            cout << "D ||";
                        }
                        else
                        {
                            cout << "N ||";
                        }
                        
                        // cout << "\t||";
                    }
                    // if (data_blocks >= 1)
                    // {

                    //     if (tagStore[i][k].TAG == vacant)
                    //     {
                    //         cout << "\t0 ";
                    //     }
                    //     else
                    //     {
                    //         cout << hex << tagStore[i][k].TAG << " ";
                    //     }

                    //     if (tagStore[i][k].dataStore.isDirty[k] == true)
                    //     {
                    //         cout << "D ||\t";
                    //     }
                    //     else
                    //     {
                    //         cout << "N ||\t";
                    //     }
                    // }
                }
            }
        }
    }

    return 0;
}
