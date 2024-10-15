#include <iostream>
#include <iomanip>
#include <math.h>
#include <typeinfo>
#include <string>
#include <fstream>
#include <sstream>
#include"gshare.h"
using namespace std;

class bimodal
{

    public:
    int* bht;
    int bht_size;
    int index;
    int value_m;
    unsigned long miss_predict = 0;
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
        cout<<"FINAL\tBIMODAL\tCONTENTS"<<"\n";
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
    // cout << "Type: " << type << " M-value: " << m_value << " N-value: " << n_value << " Trace file: " << trace_file << endl;

    FILE *fp = fopen(trace_file, "r");
    if (fp == nullptr)
    {
        cerr << "Error opening trace file." << endl;
        return 1;
    }
    // bimodal
    if(n_value == 0){
    bimodal b1;
    b1 = bimodal(m_value);
    char predict;
    unsigned long address;
    unsigned long  prediction_count =0;
    while (fscanf(fp, " %lx %c ", &address, &predict) != EOF)
    {
        prediction_count++;
        b1.update_bht(address,predict);
        }

    b1.print_bht(prediction_count);
    }

    // gshare 
    if(n_value > 0){
    gshare g1;
    g1 = gshare(m_value,n_value);
    char predict;
    bool state ;
    unsigned long address;
    unsigned long  prediction_count =0;
    while (fscanf(fp, " %lx %c ", &address, &predict) != EOF)
    {
        prediction_count++;
        
        if (predict == 't')
        {
            state=1;
        }
        if (predict == 'n')
        {
            state=0;
        }
        g1.update_bht(address,predict,state);
        }

    g1.print_bht(prediction_count);
    
    }

}
