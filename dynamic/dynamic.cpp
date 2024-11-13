#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

enum class InstructionState {
    IF, // Instruction Fetch
    ID, // Instruction Decode/Dispatch
    IS, // Issue
    EX, // Execute
    WB  // Writeback
};

struct Instruction {
    int tag;
    string pc;
    int op;
    InstructionState state;
    int dest, src1, src2;
    bool src1Ready, src2Ready;
    int IF_start, IF_duration;
    int ID_start, ID_duration;
    int IS_start, IS_duration;
    int EX_start, EX_duration;
    int WB_start, WB_duration;

    Instruction(int t, const std::string& p, int type, int d, int s1, int s2)
        : tag(t), pc(p), op(type), dest(d), src1(s1), src2(s2),
          state(InstructionState::IF), src1Ready(false), src2Ready(false) {}
};

class Register {
public:
    bool empty;
    vector<Instruction> reg;

    Register() : empty(true) {}
};

int cyclecount = 0;
int instructionCounter = 0;
Register DQ;
std::vector<Instruction> issue_list;
std::vector<Instruction> execute_list;
std::vector<Instruction> rob;
std::vector<Instruction> dispatch_list;
bool pipeline_empty = true;

bool Advance_Cycle(ifstream& fin) {
    // Check if all pipeline stages are empty and the file has reached the end
    return !(fin.eof() && DQ.empty );
}

void Fetch(ifstream& fin) {
    if (fin.eof() || !DQ.empty) return;

    string line;
    int fetch_count = 0;

    // Fetch up to 5 instructions
    
    while (getline(fin, line) ) {
        istringstream iss(line);
        string pc;
        int opType, dest, src1, src2;
        if (!(iss >> pc >> opType >> dest >> src1 >> src2)) {
            cerr << "Error parsing instruction." << endl;
            break;
        }

        int tag = instructionCounter++;
        Instruction instr(tag, pc, opType, dest, src1, src2);
        instr.IF_start = cyclecount;
        instr.IF_duration = 1;
        instr.ID_start = cyclecount + 1;
        instr.state = InstructionState::ID;

        dispatch_list.push_back(instr);
        rob.push_back(instr);
        fetch_count++;
    }

    if (fetch_count > 0) {
        DQ.empty = false;
    } else {
        DQ.empty = true; // No more instructions to fetch
    }
}

void PrintDQ() {
    if (dispatch_list.empty()) {
        cout << "DQ is empty." << endl;
        return;
    }

    cout << "DQ Register Contents:" << endl;
    cout << "--------------------------------------------" << endl;
    cout << left << setw(10) << "Tag" << setw(15) << "PC" << setw(10) << "Op"
         << setw(10) << "Dest" << setw(10) << "Src1" << setw(10) << "Src2"
         << setw(10) << "State" << endl;

    for (const auto& instr : dispatch_list) {
        cout << left << setw(10) << instr.tag
             << setw(15) << instr.pc
             << setw(10) << instr.op
             << setw(10) << instr.dest
             << setw(10) << instr.src1
             << setw(10) << instr.src2
             << setw(10) << static_cast<int>(instr.state) // Print state as integer
             << endl;
    }

    cout << "--------------------------------------------" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <n_value> <s_value> <trace_file>" << endl;
        return 1;
    }

    int n_value = stoi(argv[1]);
    int s_value = stoi(argv[2]);
    string trace_file = argv[3];

    ifstream fin(trace_file);
    if (!fin.is_open()) {
        cerr << "Error opening trace file." << endl;
        return 1;
    }

    DQ.empty = true;

    // Main simulation loop
    while (Advance_Cycle(fin)) {
        cout << "Cycle: " << cyclecount << endl;
        Fetch(fin);
        PrintDQ();
        cyclecount++;
    }

    cout << "Simulation complete." << endl;
    return 0;
}
