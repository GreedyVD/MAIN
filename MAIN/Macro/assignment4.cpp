#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

using namespace std;

// Data structure for MNT entry
struct MNTEntry
{
    string macro_name;
    int mdt_index;
    int pnt_index;
    int kpdt_index;
    int evnt_index;
    int ssnt_index;
    int num_pos;
    int num_key;
};

// Data structure for MDT entry
struct MDTEntry
{
    int index;
    string instruction;
};

// Data structure for parameter entry in PNTAB
struct PNTEntry
{
    string param_name;
    int param_index;
};

// Data structure for keyword parameter entry in KPD Table
struct KPDEntry
{
    string keyword_name;
    string default_value;
};

// Data structure for expansion-time variables
struct EVNEntry
{
    string var_name;
};

// Data structure for sequencing symbols
struct SSNEntry
{
    string symbol_name;
};

// Macro Processor class
class MacroProcessor
{
private:
    vector<MNTEntry> MNT;
    vector<MDTEntry> MDT;
    vector<PNTEntry> PNTAB; // Now using vector instead of unordered_map to maintain order
    vector<KPDEntry> KPD;
    vector<EVNEntry> EVNTAB;
    vector<SSNEntry> SSNTAB;
    int mdt_counter, pnt_counter, kpd_counter, evn_counter, ssn_counter;

public:
    MacroProcessor() : mdt_counter(0), pnt_counter(0), kpd_counter(0), evn_counter(0), ssn_counter(0) {}

    void processInput(string filename)
    {
        ifstream file(filename);
        if (!file.is_open())
        {
            cerr << "Error: Unable to open file!" << endl;
            return;
        }

        string line;
        bool inside_macro = false;
        string macro_name;
        int num_pos_params = 0, num_key_params = 0;

        // Do NOT clear PNTAB at the start of every macro
        while (getline(file, line))
        {
            istringstream iss(line);
            string token;
            iss >> token;

            if (token == "MACRO")
            {
                // Start of macro definition
                inside_macro = true;
                num_pos_params = num_key_params = 0; // Reset counts
            }
            else if (inside_macro)
            {
                macro_name = token;
                processParameters(iss, num_pos_params, num_key_params);

                // Add MNT entry
                addMNT(macro_name, num_pos_params, num_key_params);

                // Add macro lines to MDT until 'MEND' is found
                while (getline(file, line) && line != "MEND")
                {
                    if (line.find('#') != string::npos)
                    {
                        processEVN(line); // Handle expansion-time variables
                    }
                    if (line.find('$') != string::npos)
                    {
                        processSSN(line); // Handle sequencing symbols
                    }
                    addMDT(line);
                }

                // Add the MEND statement
                addMDT("MEND");
                inside_macro = false;
            }
        }

        file.close();
    }

    void processParameters(istringstream &iss, int &num_pos_params, int &num_key_params)
    {
        string param;
        while (iss >> param)
        {
            if (param[0] == '&')
            {                            // Positional parameter
                param = param.substr(1); // Remove '&' from parameter name
                addPNT(param);
                num_pos_params++;
            }
            else if (param.find('=') != string::npos)
            { // Keyword parameter with default
                string keyword_name = param.substr(0, param.find('='));
                string default_value = param.substr(param.find('=') + 1);
                addKPD(keyword_name, default_value);
                num_key_params++;
            }
        }
    }

    void processEVN(const string &line)
    {
        // Detect expansion-time variable prefixed with #
        size_t pos = line.find('#');
        if (pos != string::npos)
        {
            string var_name = line.substr(pos + 1); // Remove '#' from variable name
            addEVNTAB(var_name);
        }
    }

    void processSSN(const string &line)
    {
        // Detect sequencing symbol prefixed with $
        size_t pos = line.find('$');
        if (pos != string::npos)
        {
            string symbol_name = line.substr(pos + 1); // Remove '$' from symbol name
            addSSNTAB(symbol_name);
        }
    }

    void addMNT(string macro_name, int num_pos_params, int num_key_params)
    {
        MNT.push_back({macro_name, mdt_counter, pnt_counter, kpd_counter - num_key_params, evn_counter, ssn_counter, num_pos_params, num_key_params});
    }

    void addMDT(string instruction)
    {
        // Replace positional parameters (&) in the instruction
        for (const auto &pnt_entry : PNTAB)
        {
            size_t pos = instruction.find("&" + pnt_entry.param_name);
            if (pos != string::npos)
            {
                instruction.replace(pos, pnt_entry.param_name.length() + 1, "(P," + to_string(pnt_entry.param_index) + ")");
            }
        }

        // Replace expansion time variables (#) in the instruction
        for (size_t i = 0; i < EVNTAB.size(); ++i)
        {
            size_t pos = instruction.find("#" + EVNTAB[i].var_name);
            if (pos != string::npos)
            {
                instruction.replace(pos, EVNTAB[i].var_name.length() + 1, "(E," + to_string(i + 1) + ")");
            }
        }

        // Replace sequencing symbols ($) in the instruction
        for (size_t i = 0; i < SSNTAB.size(); ++i)
        {
            size_t pos = instruction.find("$" + SSNTAB[i].symbol_name);
            if (pos != string::npos)
            {
                instruction.replace(pos, SSNTAB[i].symbol_name.length() + 1, "(SS," + to_string(i + 1) + ")");
            }
        }

        // Add the modified instruction to MDT
        MDT.push_back({mdt_counter++, instruction});
    }

    // Add a new positional parameter to PNTAB
    void addPNT(string param_name)
    {
        // Check if the parameter already exists in PNTAB
        for (const auto &entry : PNTAB)
        {
            if (entry.param_name == param_name)
            {
                return; // If parameter already exists, do not add it again
            }
        }
        // Add new parameter if it doesn't exist
        PNTAB.push_back({param_name, ++pnt_counter});
    }

    void addKPD(string keyword_name, string default_value)
    {
        KPD.push_back({keyword_name, default_value});
        kpd_counter++;
    }

    void addEVNTAB(string var_name)
    {
        EVNTAB.push_back({var_name});
        evn_counter++;
    }

    void addSSNTAB(string symbol_name)
    {
        SSNTAB.push_back({symbol_name});
        ssn_counter++;
    }

    void printMNT()
    {
        cout << "\nMNT (Macro Name Table):\n";
        cout << "Index\tMacro Name\tMDT Index\tPNTAB Index\tKPDT Index\tEVNT Index\tSSNT Index\t#Pos\t#Key\n";
        for (size_t i = 0; i < MNT.size(); ++i)
        {
            cout << i + 1 << "\t" << MNT[i].macro_name << "\t\t" << MNT[i].mdt_index << "\t\t"
                 << MNT[i].pnt_index << "\t\t" << MNT[i].kpdt_index << "\t\t"
                 << MNT[i].evnt_index << "\t\t" << MNT[i].ssnt_index << "\t\t"
                 << MNT[i].num_pos << "\t" << MNT[i].num_key << endl;
        }
    }

    void printMDT()
    {
        cout << "\nMDT (Macro Definition Table):\n";
        cout << "Index\tInstruction\n";
        for (const auto &entry : MDT)
        {
            cout << entry.index << "\t" << entry.instruction << endl;
        }
    }

    void printPNTAB()
    {
        cout << "\nPNTAB (Parameter Name Table):\n";
        cout << "Index\tParameter Name\n";
        int idx = 0;
        for (const auto &entry : PNTAB)
        {
            cout << ++idx << "\t" << entry.param_name << endl;
        }
    }

    void printKPD()
    {
        cout << "\nKPD (Keyword Parameter Table):\n";
        cout << "Index\tKeyword Name\tDefault Value\n";
        for (size_t i = 0; i < KPD.size(); ++i)
        {
            cout << i + 1 << "\t" << KPD[i].keyword_name << "\t\t" << KPD[i].default_value << endl;
        }
    }

    void printEVNTAB()
    {
        cout << "\nEVNTAB (Expansion Time Variable Table):\n";
        cout << "Index\tVariable Name\n";
        for (size_t i = 0; i < EVNTAB.size(); ++i)
        {
            cout << i + 1 << "\t" << EVNTAB[i].var_name << endl;
        }
    }

    void printSSNTAB()
    {
        cout << "\nSSNTAB (Sequencing Symbol Name Table):\n";
        cout << "Index\tSymbol Name\n";
        for (size_t i = 0; i < SSNTAB.size(); ++i)
        {
            cout << i + 1 << "\t" << SSNTAB[i].symbol_name << endl;
        }
    }
};

int main()
{
   

    MacroProcessor mp;
    mp.processInput("assignment4.txt");
    mp.printMNT();
    mp.printMDT();
    mp.printPNTAB();
    mp.printKPD();
    mp.printEVNTAB();
    mp.printSSNTAB();

    return 0;
}
