#include <iostream>
#include <math.h>
#include <typeinfo>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

class bimodal
{

    public:
    int* bht;
    int bht_size;
    int index ;
    int value_m;
    bimodal() = default;
    bimodal(int value_m) { 
        this->value_m = value_m;          
        bht_size = pow(2, value_m);
        bht = new int[bht_size];  
        for (int i = 0; i < bht_size; ++i) {
            bht[i] = 2; 
        }
        

    }
    
    void update_bht(unsigned long address,char predict){
        index = (address>>2) & (1<<value_m)-1;
        int tp = bht[index] ;

        if (predict == 't'){
            if(bht[index]<3){
            bht[index]++;
            }
        }
        if (predict == 'n'){
            if(bht[index]>0){
            bht[index]--;
            }
        }
    }
    void print_bht(){
        for(int i =0;i < bht_size;i++){
            cout << i << "\t" << bht[i] << "\n";
        }
    }

};

int main(int argc, char *argv[])
{
    char *trace_file;
    int n_value;
    string type = argv[1];
    int m_value = stoi(argv[2]);
    if (type == "bimodal"){
        n_value = 0;
        trace_file = argv[3];
    }
    else{
        n_value = stoi(argv[3]);
        trace_file = argv[4];
    }
    cout << "Type: " << type << " M-value: " << m_value << " N-value: " << n_value << " Trace file: " << trace_file << endl;

    FILE *fp = fopen(trace_file, "r");
    if (fp == nullptr)
    {
        cerr << "Error opening trace file." << endl;
        return 1;
    }
    bimodal b1;
    b1 = bimodal(m_value);
    char predict;
    unsigned long address;

    while (fscanf(fp, " %lx %c ", &address, &predict) != EOF)
    {
        b1.update_bht(address,predict);

        if (predict == 't')
        {
            // cout << "\ntaken";
        }
        if (predict == 'n')
        {
            // cout << "\nnot taken";
        }
        }

    b1.print_bht();

}
