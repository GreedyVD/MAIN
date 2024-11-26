#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>
#include <algorithm>

using namespace std;

struct Literal
{
    string value;
    int address;
};

struct RelocationEntry
{
    string full_instruction;
    int translated_address;
    int linktime_address;
};

struct LinkEntry
{
    string symbol;
    string type;
    int translated_address;
};

struct NTABEntry
{
    string obj_module_or_variable;
    int linktime_address;
};

// Symbol tables, intermediate codes, relocation tables, and link tables for each file
vector<vector<pair<string, int>>> symbol_tables(3);
vector<vector<pair<int, string>>> intermediate_codes(3);
vector<vector<RelocationEntry>> relocation_tables(3);
vector<vector<LinkEntry>> link_tables(3);
vector<Literal> literal_table;
vector<int> pool_table;
vector<NTABEntry> NTAB; // New NTAB for object modules and public variables

// Hardcoded Machine OpCode Table (MOT)
map<string, pair<string, int>> mot = {
    {"START", {"AD", 1}},
    {"END", {"AD", 2}},
    {"ORIGIN", {"AD", 3}},
    {"LTORG", {"AD", 4}},
    {"EXTERN", {"AD", 5}},
    {"ENTRY", {"AD", 6}},
    {"MOVER", {"IS", 1}},
    {"ADD", {"IS", 2}},
    {"SUB", {"IS", 3}},
    {"STOP", {"IS", 4}},
    {"COMP", {"IS", 5}},
    {"JZ", {"IS", 6}},
    {"JMP", {"IS", 7}},
    {"JNZ", {"IS", 8}},
    {"INCR", {"IS", 9}},
    {"DECR", {"IS", 10}},
    {"STORE", {"IS", 11}},
    {"DC", {"DL", 1}},
    {"DS", {"DL", 2}}};

map<string, int> register_table = {
    {"AREG", 1},
    {"BREG", 2},
    {"CREG", 3},
    {"DREG", 4}};

int lc = 0;
int literal_index = 0;
int relocation_factor = 800;
int last_linktime_address = 0;
int link_origin = 0;

// Modified first pass to handle different files
void firstPass(const string &line, vector<pair<string, int>> &symbol_table)
{
    string tokens[3];
    int tokenIndex = 0;
    size_t start = 0;
    size_t end = line.find(' ');
    while (end != string::npos)
    {
        tokens[tokenIndex++] = line.substr(start, end - start);
        start = end + 1;
        end = line.find(' ', start);
    }
    tokens[tokenIndex] = line.substr(start, end);

    if (mot.find(tokens[0]) == mot.end() && !tokens[0].empty())
    {
        bool found = false;
        for (auto &sym : symbol_table)
        {
            if (sym.first == tokens[0])
            {
                found = true;
                if (sym.second == -1)
                {
                    sym.second = lc;
                }
                return;
            }
        }
        if (!found && !tokens[0].empty())
        {
            symbol_table.push_back({tokens[0], lc});
        }
        tokens[0] = tokens[1];
        tokens[1] = tokens[2];
        tokens[2] = "";
    }

    string mnemonic = tokens[0];
    if (mnemonic == "START")
    {
        lc = stoi(tokens[1]);
    }
    else if (mnemonic == "DS")
    {
        lc += stoi(tokens[1]);
    }
    else if (mnemonic == "DC")
    {
        lc++;
    }
    else if (mot.find(mnemonic) != mot.end())
    {
        if (mnemonic == "EXTERN" || mnemonic == "ENTRY")
        {
            return;
        }
        lc += (mot[mnemonic].first == "IS") ? 2 : 1;
    }
}

void updateRelocationFactorForNextFile(int fileIndex)
{
    if (!relocation_tables[fileIndex].empty())
    {
        last_linktime_address = relocation_tables[fileIndex].back().linktime_address;
        relocation_factor = last_linktime_address + 1;
    }
}

// Modified second pass to handle different files and add to relocation table
void secondPass(const string &line, vector<pair<string, int>> &symbol_table,
                vector<pair<int, string>> &intermediate_code,
                vector<RelocationEntry> &relocation_table,
                vector<LinkEntry> &link_table,
                const string &filename) // Pass the filename
{
    vector<string> tokens;
    size_t start = 0;
    size_t end;
    while ((end = line.find_first_of(" ,", start)) != string::npos)
    {
        if (end > start)
        {
            tokens.push_back(line.substr(start, end - start));
        }
        start = end + 1;
    }
    if (start < line.length())
    {
        tokens.push_back(line.substr(start));
    }

    if (mot.find(tokens[0]) == mot.end() && mot.find(tokens[1]) != mot.end())
    {
        tokens.erase(tokens.begin());
    }

    string mnemonic = tokens[0];
    if (mnemonic == "START")
    {
        lc = stoi(tokens[1]);
        intermediate_code.push_back({lc, "(AD,1) (C," + tokens[1] + ")"});

        // Add filename and its starting link-time address to NTAB based on filename
        if (filename == "project2_1.txt")
        {
            NTAB.push_back({filename, relocation_factor + 200});
        }
        else if (filename == "project2_2.txt")
        {
            NTAB.push_back({filename, relocation_factor + 210});
        }
        else if (filename == "project2_3.txt")
        {
            NTAB.push_back({filename, relocation_factor + 220});
        }
        else
        {
            // Default case, if filename does not match specific cases
            NTAB.push_back({filename, relocation_factor + 200});
        }
    }
    else if (mnemonic == "END" || mnemonic == "LTORG")
    {
        intermediate_code.push_back({lc, (mnemonic == "LTORG") ? "(AD,4)" : "(AD,2)"});
        while (literal_index < literal_table.size())
        {
            literal_table[literal_index].address = lc;
            lc++;
            literal_index++;
        }
        pool_table.push_back(literal_index);
    }
    else if (mnemonic == "ORIGIN")
    {
        lc = stoi(tokens[1]);
        intermediate_code.push_back({lc, "(AD,3) (C," + tokens[1] + ")"});
    }
    else if (mnemonic == "DS")
    {
        intermediate_code.push_back({lc, "(DL,2) (C," + tokens[1] + ")"});
        lc += stoi(tokens[1]);
    }
    else if (mnemonic == "DC")
    {
        intermediate_code.push_back({lc, "(DL,1) (C," + tokens[1] + ")"});
        lc++;
    }
    else if (mot.find(mnemonic) != mot.end())
    {
        if (mnemonic == "EXTERN" || mnemonic == "ENTRY")
        {
            LinkEntry entry;
            entry.symbol = tokens[1];
            entry.type = (mnemonic == "EXTERN") ? "external" : "public";
            entry.translated_address = lc;
            link_table.push_back(entry);

            intermediate_code.push_back({lc, (mnemonic == "EXTERN") ? "(AD,5)" : "(AD,6)"});

            if (mnemonic == "ENTRY")
            {
                // Add public variable and its link-time address to NTAB
                NTAB.push_back({tokens[1], lc + relocation_factor});
            }
            return;
        }

        int opcode = mot[mnemonic].second;
        stringstream icStream;
        icStream << "(IS," << opcode << ")";
        string operand1 = (tokens.size() > 1) ? tokens[1] : "";
        string operand2 = (tokens.size() > 2) ? tokens[2] : "";

        if (register_table.find(operand1) != register_table.end())
        {
            int reg_code1 = register_table[operand1];
            icStream << " (R," << reg_code1 << ")";
            if (!operand2.empty())
            {
                int symbol_index = find_if(symbol_table.begin(), symbol_table.end(),
                                           [&](const pair<string, int> &sym)
                                           {
                                               return sym.first == operand2;
                                           }) -
                                   symbol_table.begin() + 1;
                icStream << " (S," << symbol_index << ")";
            }
        }

        intermediate_code.push_back({lc, icStream.str()});

        if (mnemonic == "JMP" || mnemonic == "JZ" || mnemonic == "JNZ" ||
            mnemonic == "MOVER" || mnemonic == "MOVEM")
        {
            relocation_table.push_back({line, lc, lc + relocation_factor});
        }

        lc += 2;
    }
}

void displayNTAB()
{
    cout << "\nNTAB (Object Modules and Public Variables):\n";
    cout << setw(35) << "Obj Module & Public Variable"
         << setw(20) << "Linktime Address" << "\n";
    cout << string(52, '-') << "\n";

    for (const auto &entry : NTAB)
    {
        cout << setw(35) << entry.obj_module_or_variable
             << setw(20) << entry.linktime_address << "\n";
    }
    cout << "\n";
}

// Function to display the link table
void displayLinkTables()
{
    cout << "\nLink Tables:\n\n";
    for (int i = 0; i < 3; i++)
    {
        cout << "project2_" << (i + 1) << ":\n" << string(12, '-') << endl;
        cout << setw(15) << "Symbol"
             << setw(15) << "Type"
             << setw(20) << "Translated Address" << "\n";
        cout << string(50, '-') << "\n";

        for (const auto &entry : link_tables[i])
        {
            cout << setw(15) << entry.symbol
                 << setw(15) << entry.type
                 << setw(20) << entry.translated_address << "\n";
        }
        cout << "\n";
    }
}

// Function to display relocation table
void displayRelocationTables()
{
    cout << "\nRelocation Tables:\n\n";
    for (int i = 0; i < 3; i++)
    {
        cout << "project2_" << (i + 1) << ":\n" << string(12, '-') << endl;
        cout << setw(50) << "Instruction"
             << setw(30) << "Translated Address"
             << setw(30) << "Linktime Address" << "\n";
        cout << string(97, '-') << "\n";

        for (const auto &entry : relocation_tables[i])
        {
            cout << setw(50) << entry.full_instruction // Display full instruction
                 << setw(30) << entry.translated_address
                 << setw(30) << entry.linktime_address << "\n";
        }
        cout << "\n";
    }
}

// Function to read file and process passes
int last_final_lc = 0; // Store the ending lc after each file

void processFile(const string &filename, int fileIndex)
{
    ifstream inputFile(filename);
    if (!inputFile)
    {
        cerr << "Error: Could not open file " << filename << "!" << endl;
        return;
    }

    string line;
    pool_table.push_back(0);

    // First pass
    while (getline(inputFile, line))
    {
        firstPass(line, symbol_tables[fileIndex]);
    }

    // Reset file to the beginning for the second pass
    inputFile.clear();
    inputFile.seekg(0, ios::beg);

    // Second pass
    while (getline(inputFile, line))
    {
        secondPass(line, symbol_tables[fileIndex], intermediate_codes[fileIndex], relocation_tables[fileIndex], link_tables[fileIndex], filename);
    }

    inputFile.close();

    // Update the relocation factor for the next file based on the last instruction of this file
    last_final_lc = lc; // Update to reflect where the last instruction ended
    relocation_factor = last_final_lc + 1 + 800;
}

int main()
{
    cout << "Enter Link Origin: ";
    cin >> link_origin;

    string files[] = {"project2_1.txt", "project2_2.txt", "project2_3.txt"};
    for (int i = 0; i < 3; i++)
    {
        relocation_tables.push_back(vector<RelocationEntry>());
        processFile(files[i], i);
    }

    // Display headers for each file
    cout << "\nResults:\n";
    for (int i = 0; i < 3; i++)
    {
        cout << setw(25) << files[i] << " | ";
    }
    cout << "\n";

    // Display Symbol Tables horizontally
    cout << "\nSymbol Table:\n\n";
    for (int i = 0; i < 3; i++)
    {
        cout << setw(15) << "Symbol" << setw(10) << "Address" << " | ";
    }
    cout << "\n";

    // Display each file's symbol table in aligned columns
    size_t maxRows = max({symbol_tables[0].size(), symbol_tables[1].size(), symbol_tables[2].size()});
    for (size_t row = 0; row < maxRows; row++)
    {
        for (int i = 0; i < 3; i++)
        {
            if (row < symbol_tables[i].size())
            {
                cout << setw(15) << symbol_tables[i][row].first << setw(10) << symbol_tables[i][row].second << " | ";
            }
            else
            {
                cout << setw(25) << " " << " | "; // Empty space if the table has fewer rows
            }
        }
        cout << "\n";
    }

    // Display Intermediate Codes in an organized format
    cout << "\nIntermediate Code:" << endl;
    cout << string(116, '-') << "\n"; // Adjust the width based on your table's structure
    for (int i = 0; i < 3; i++)
    {
        cout << files[i] << setw(25) << " | ";
    }
    cout << endl;
    cout << string(116, '-') << "\n";

    // Print the header
    for (int i = 0; i < 3; i++)
    {
        cout << "Location" << setw(2) << " : " << "Code" << setw(24) << " | ";
    }
    cout << "\n";

    size_t maxICRows = max({intermediate_codes[0].size(), intermediate_codes[1].size(), intermediate_codes[2].size()});
    for (size_t row = 0; row < maxICRows; row++)
    {
        for (int i = 0; i < 3; i++)
        {
            if (row < intermediate_codes[i].size())
            {
                // Extract location and code
                string location = to_string(intermediate_codes[i][row].first);
                string code = intermediate_codes[i][row].second;

                // Output the current row with content
                cout << setw(8) << location << " : " << left << setw(25) << code << " | ";
            }
            else
            {
                // Output empty space with : and ensure vertical bar alignment
                cout << setw(8) << " " << " : " << setw(25) << " " << " | ";
            }
        }
        cout << "\n";
    }

    // Optional: Final line to close the table
    cout << string(116, '-') << "\n"; // Adjust the width based on your table's structure

    displayRelocationTables();

    displayLinkTables();

    displayNTAB();

    return 0;
}