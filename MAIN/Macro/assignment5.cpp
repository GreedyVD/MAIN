#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <string>

// Structures for the tables
struct MNTEntry {
    std::string macro_name;
    int mdt_index, pntab_index, kpdt_index, evnt_index, ssnt_index;
    int pos_count, key_count;
};

struct MDTEntry {
    int index;
    std::string instruction;
};

struct PNTABEntry {
    int index;
    std::string parameter_name;
};

// Hardcoded tables
std::vector<MNTEntry> MNT = {
    {"MAC1", 0, 1, 1, 0, 0, 2, 2},
    {"MAC2", 3, 3, 3, 0, 0, 1, 2},
    {"MAC3", 6, 4, 5, 1, 1, 2, 2}
};

std::vector<MDTEntry> MDT = {
    {0, "LDA (P,1)"}, {1, "ADD (P,2)"}, {2, "MEND"},
    {3, "STA (P,3)"}, {4, "SUB K3"}, {5, "MEND"},
    {6, "LDA (P,4)"}, {7, "ADD (P,5)"}, {8, "SUB (E,1)"},
    {9, "BR (SS,1)"}, {10, "MEND"}
};

std::vector<PNTABEntry> PNTAB = {
    {1, "A"}, {2, "B"}, {3, "X"}, {4, "C"}, {5, "D"}
};

// Global Actual Parameter Table (APT)
std::vector<std::pair<std::string, std::string>> APT;

// Function to print the Macro Name Table (MNT)
void printMNT() {
    std::cout << "Macro Name Table (MNT):\n";
    std::cout << "Index\tMacro Name\tMDT Index\tPNTAB Index\tKPDT Index\tEVNT Index\tSSNT Index\t#Pos\t#Key\n";
    for (size_t i = 0; i < MNT.size(); ++i) {
        const MNTEntry& entry = MNT[i];
        std::cout << i + 1 << "\t" << entry.macro_name << "\t\t" 
                  << entry.mdt_index << "\t\t" 
                  << entry.pntab_index << "\t\t" 
                  << entry.kpdt_index << "\t\t" 
                  << entry.evnt_index << "\t\t" 
                  << entry.ssnt_index << "\t\t" 
                  << entry.pos_count << "\t" 
                  << entry.key_count << "\n";
    }
    std::cout << std::endl;
}

// Function to print the Macro Definition Table (MDT)
void printMDT() {
    std::cout << "Macro Definition Table (MDT):\n";
    std::cout << "Index\tInstruction\n";
    for (const auto& entry : MDT) {
        std::cout << entry.index << "\t" << entry.instruction << "\n";
    }
    std::cout << std::endl;
}

// Function to add to the Actual Parameter Table
void updateAPT(const MNTEntry& mnt_entry, const std::vector<std::string>& args) {
    for (int i = 0; i < mnt_entry.pos_count; ++i) {
        std::string formal_param = "(P," + std::to_string(i + 1) + ")";
        std::string actual_param = args[i];

        // Add to APT
        APT.push_back({formal_param, actual_param});
    }
}

// Function to print the Actual Parameter Table (APT)
void printAPT() {
    std::cout << "Actual Parameter Table (APT):\n";
    for (const auto& pair : APT) {
        std::cout << "Formal Parameter: " << pair.first << " -> Actual Parameter: " << pair.second << std::endl;
    }
    std::cout << std::endl;
}

// Function to expand a macro
void expandMacro(const std::string& macro_name, const std::vector<std::string>& args, std::stringstream& expanded_code) {
    for (const auto& mnt_entry : MNT) {
        if (mnt_entry.macro_name == macro_name) {
            // Update the Actual Parameter Table (APT)
            updateAPT(mnt_entry, args);

            int mdt_index = mnt_entry.mdt_index;

            // Expand and print the macro code
            while (MDT[mdt_index].instruction != "MEND") {
                std::string instruction = MDT[mdt_index].instruction;

                // Replace formal parameters with actual parameters in the instruction
                for (int i = 1; i <= mnt_entry.pos_count; ++i) {
                    std::string placeholder = "(P," + std::to_string(i) + ")";
                    size_t pos = instruction.find(placeholder);
                    if (pos != std::string::npos) {
                        instruction.replace(pos, placeholder.length(), args[i - 1]); // Using args[i - 1] for correct indexing
                    }
                }

                expanded_code << instruction << std::endl;
                ++mdt_index;
            }
            break;
        }
    }
}

// Function to read the program and process macro calls
void processProgram(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return;
    }

    std::string line;
    bool inside_macro_definition = false;  // Ignore macro definition lines

    // Buffer to store expanded code to print after the APT
    std::stringstream expanded_code;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string word;
        ss >> word;

        // Ignore MACRO and MEND lines
        if (word == "MACRO") {
            inside_macro_definition = true;
        } else if (word == "MEND") {
            inside_macro_definition = false;
        } else if (inside_macro_definition) {
            // Skip lines inside the macro definition
            continue;
        } else if (word == "MAC1" || word == "MAC2" || word == "MAC3") {
            // Extract arguments
            std::vector<std::string> args;
            std::string arg;
            while (ss >> arg) {
                // Remove commas from arguments if present
                if (arg.back() == ',') {
                    arg.pop_back();
                }
                args.push_back(arg);
            }

            // Expand macro
            expandMacro(word, args, expanded_code);
        } else {
            // Print non-macro lines directly to the expanded code
            expanded_code << line << std::endl;
        }
    }

    file.close();

    // Print the MNT, MDT, and APT
    printMNT();
    printMDT();
    printAPT();

    // Print the expanded code
    std::cout << expanded_code.str();
}

int main() {
    std::string filename = "assignment5.txt"; // Ensure the input file path is correct
    processProgram(filename);

    return 0;
}