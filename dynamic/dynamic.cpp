#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// Enum for Instruction States
enum class InstructionState {
    IF, // Instruction Fetch
    ID, // Instruction Decode/Dispatch
    IS, // Issue
    EX, // Execute
    WB  // Writeback
};

// Structure for Instructions
struct Instruction {
    int tag;                    // Unique tag for each instruction
    string pc;
    int op;                    // Operation type (0, 1, or 2)
    InstructionState state;     // Current state of the instruction
    int dest;                   // Destination register
    int src1, src2;             // Source registers
    bool src1Ready, src2Ready;  // Operand readiness
    int IF_start, IF_duration;
    int ID_start, ID_duration;
    int IS_start, IS_duration;
    int EX_start, EX_duration;
    int WB_start, WB_duration;
    
    Instruction(int t, const std::string& p, int type, int d, int s1, int s2)
        : tag(t), pc(p), op(type), dest(d), src1(s1), src2(s2),
          state(InstructionState::IF), src1Ready(false), src2Ready(false) {}
};

// RMT Structure
struct RMTEntry {
    bool valid = false;
    int tag = -1;
};

vector<RMTEntry> RMT(32); // Assuming 32 registers

// Global Variables
int cyclecount = 0;
int instructionCounter = 0;
int dispatch_width;
int scheduling_queue_size;
std::ifstream fin;
bool pipeLine_empty = true;

// Registers & Queues
vector<Instruction> issue_list;
vector<Instruction> execute_list;
vector<Instruction> rob;
vector<Instruction> dispatch_list;

// Function to Fetch Instructions
void Fetch(int n) {
    std::string line;
    int fetched_count = 0;
    
    if (fin.eof())
        return;
    
    // Fetch up to 'n' instructions
    while (fetched_count < n && getline(fin, line)) {
        std::istringstream iss(line);
        std::string pc;
        int opType, dest, src1, src2;
        
        if (!(iss >> pc >> opType >> dest >> src1 >> src2))
            continue;
        
        int tag = instructionCounter++;
        Instruction instr(tag, pc, opType, dest, src1, src2);
        instr.IF_start = cyclecount;
        instr.state = InstructionState::ID;
        
        dispatch_list.push_back(instr);
        rob.push_back(instr);
        fetched_count++;
    }
}

// Dispatch Stage with Register Renaming
void Dispatch() {
    vector<Instruction> temp_list;

    // Collect instructions in the ID state
    for (auto& instr : dispatch_list) {
        if (instr.state == InstructionState::ID) {
            temp_list.push_back(instr);
        }
    }

    // Sort by tag to maintain program order
    sort(temp_list.begin(), temp_list.end(), [](const Instruction& a, const Instruction& b) {
        return a.tag < b.tag;
    });

    // Rename and move to issue list
    for (auto& instr : temp_list) {
        instr.src1Ready = (instr.src1 == -1 || !RMT[instr.src1].valid);
        instr.src2Ready = (instr.src2 == -1 || !RMT[instr.src2].valid);

        if (instr.src1 != -1 && RMT[instr.src1].valid) {
            instr.src1 = RMT[instr.src1].tag;
        }
        if (instr.src2 != -1 && RMT[instr.src2].valid) {
            instr.src2 = RMT[instr.src2].tag;
        }

        if (instr.dest != -1) {
            RMT[instr.dest].valid = true;
            RMT[instr.dest].tag = instr.tag;
        }

        instr.state = InstructionState::IS;
        instr.IS_start = cyclecount;
        issue_list.push_back(instr);
    }

    // Remove dispatched instructions
    dispatch_list.erase(remove_if(dispatch_list.begin(), dispatch_list.end(),
                                  [](Instruction& instr) {
                                      return instr.state == InstructionState::IS;
                                  }), dispatch_list.end());
}

// Print the Issue List
void PrintIssueList() {
    cout << "Issue List Contents:" << endl;
    cout << "--------------------------------------------" << endl;
    cout << left << setw(10) << "Tag" << setw(15) << "PC" 
         << setw(10) << "Op" << setw(10) << "Dest" 
         << setw(10) << "Src1" << setw(10) << "Src2" << endl;
    
    for (const auto& instr : issue_list) {
        cout << left << setw(10) << instr.tag
             << setw(15) << instr.pc
             << setw(10) << instr.op
             << setw(10) << instr.dest
             << setw(10) << instr.src1
             << setw(10) << instr.src2 << endl;
    }
}

// Print RMT
void PrintRMT() {
    cout << "Register Mapping Table (RMT) Contents:" << endl;
    cout << "--------------------------------------------" << endl;
    cout << left << setw(10) << "Reg" << setw(10) << "Valid" << setw(10) << "Tag" << endl;
    
    for (int i = 0; i < RMT.size(); i++) {
        cout << left << setw(10) << i
             << setw(10) << RMT[i].valid
             << setw(10) << (RMT[i].valid ? RMT[i].tag : -1) << endl;
    }
}

int main(int argc, char *argv[]) {
    dispatch_width = stoi(argv[1]);
    scheduling_queue_size = stoi(argv[2]);
    fin.open(argv[3]);

    Fetch(dispatch_width);
    Dispatch();
    PrintIssueList();
    PrintRMT();

    Fetch(dispatch_width);
    Dispatch();
    PrintIssueList();
    PrintRMT();

    return 0;
}
