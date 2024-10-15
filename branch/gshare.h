#include <iostream>
#include <bitset>
#include <iomanip>
#include <math.h>
#include <typeinfo>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

class gshare
{

    public:
    int* bht;
    int bht_size;
    int index;
    int m_bits;
    int n_bits;
    int xor_value;
    int value_m;
    int value_n;
    int lower_bits;
    unsigned int gbh;
    unsigned long miss_predict = 0;
    gshare() = default;
    gshare(int value_m,int value_n) { 
        this->value_m = value_m;
        this->value_n = value_n;          
        bht_size = pow(2, value_m);
        bht = new int[bht_size];  
        for (int i = 0; i < bht_size; ++i) {
            bht[i] = 2; 
        }
        gbh =0 ;
        

    }
    
    void update_bht(unsigned long address,char predict,bool taken){
        // std::cout << "\naddress: " << std::bitset<32>(address) << std::endl;
        m_bits = (address>>2) & (1<<value_m)-1;
        // std::cout << "m_bits: " << std::bitset<5>(m_bits) << std::endl;

        n_bits = m_bits >> (value_m - value_n);
        // std::cout << "n_bits: " << std::bitset<2>(n_bits) << std::endl;
        // int tp = bht[index] ;

        
        // std::cout <<"gbh value"<<std::bitset<2>(gbh) << std::endl; 
        xor_value=n_bits ^ gbh;
        // std::cout << "XOR result: " << std::bitset<2>(xor_value) << std::endl;

        lower_bits = m_bits & ((1 << (value_m - value_n)) - 1);
        // std::cout << "Lower bits: " << std::bitset<3>(lower_bits) << std::endl;

        index = (xor_value << (value_m - value_n)) | lower_bits;
        // std::cout << "Final Index: " << std::bitset<5>(index) << std::endl;
        // gbh = ((gbh << 1) | taken) & ((1 << value_n) - 1);
        gbh = (gbh >> 1) | (taken << (value_n - 1));
        if (predict == 't'){
            if((bht[index]<2 )){
                miss_predict++;
            }
            if(bht[index]<3){
            bht[index]++;
            }
            

        }
        if (predict == 'n'){
            if((bht[index]>1 )){
                miss_predict++;
            }
            if(bht[index]>0){
            bht[index]--;
            }
        
        }
        
    }
    void print_bht(unsigned long total_prediction){
        // float miss_rate=(float(miss_predict) / float(total_prediction))*100;
        float miss_rate = (static_cast<float>(miss_predict) / static_cast<float>(total_prediction)) * 100;
        cout<<"OUTPUT"<<"\n";
        
        cout<<"number of predictions:\t"<< total_prediction<<"\n";
        cout<<"number of mispredictions:\t"<<miss_predict<<"\n";
        cout << fixed << setprecision(2);
        cout<<"misprediction rate:\t"<<miss_rate<<"%\n";
        cout<<"FINAL\tGSHARE\tCONTENTS"<<"\n";
        for(int i =0;i < bht_size;i++){
            cout << i << "\t" << bht[i] << "\n";
        }
        
    }

};
