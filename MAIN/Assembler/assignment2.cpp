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

vector<pair<string, int>> symbol_table;
vector<Literal> literal_table;
vector<int> pool_table;
vector<pair<int, string>> intermediate_code;
vector<string> symbol_list;
map<string, bool> label_resolved;

map<string, pair<string, int>> mot = {
    {"START", {"AD", 1}},
    {"END", {"AD", 2}},
    {"ORIGIN", {"AD", 3}},
    {"LTORG", {"AD", 4}},
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
    {"MULT", {"IS", 11}},
    {"DIV", {"IS", 12}},
    {"DC", {"DL", 1}},
    {"DS", {"DL", 2}}};

map<string, int> register_table = {
    {"AREG", 1},
    {"BREG", 2},
    {"CREG", 3},
    {"DREG", 4}};

int lc = 0;
int literal_index = 0;

void firstPass(const string &line)
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

    // Check if the first token is a mnemonic or label
    if (mot.find(tokens[0]) == mot.end() && !tokens[0].empty())
    {
        bool found = false;

        // Check if the symbol is a literal
        if (tokens[0].find("='") == 0)
        {
            // If it's a literal, skip adding to the symbol table
            Literal literal;
            literal.value = tokens[0];
            literal.address = -1;

            // Check if the literal is already in the literal table
            auto it = find_if(literal_table.begin(), literal_table.end(),
                              [&](const Literal &l)
                              { return l.value == literal.value; });

            if (it == literal_table.end())
            {
                literal_table.push_back(literal);
            }

            return; // Exit since it's a literal and should not go to symbol table
        }

        // If not a literal, proceed to add to symbol table
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

    // Handle mnemonics and literals
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
        if (mot[mnemonic].first == "IS")
        {
            lc += 2;
        }
        else
        {
            lc++;
        }

        // Check for literals in the operands
        if (tokens[2].find("='") == 0)
        {
            Literal literal;
            literal.value = tokens[2];
            literal.address = -1;

            auto it = find_if(literal_table.begin(), literal_table.end(),
                              [&](const Literal &l)
                              { return l.value == literal.value; });

            if (it == literal_table.end())
            {
                literal_table.push_back(literal);
            }
        }
    }
}

void secondPass(const string &line)
{
    vector<string> tokens;
    size_t start = 0;
    size_t end;

    // Tokenize by space and comma
    while ((end = line.find_first_of(" ,", start)) != string::npos)
    {
        if (end > start)
        {
            tokens.push_back(line.substr(start, end - start)); // Capture the token
        }
        start = end + 1;
    }
    if (start < line.length())
    {
        tokens.push_back(line.substr(start)); // Capture the last token
    }

    // Check for label (when the first token is not an opcode)
    if (mot.find(tokens[0]) == mot.end() && mot.find(tokens[1]) != mot.end())
    {
        tokens.erase(tokens.begin()); // Remove the label
    }

    string mnemonic = tokens[0];
    bool new_literals_added = false; // Flag to track if new literals are added since the last LTORG/END

    if (mnemonic == "START")
    {
        lc = stoi(tokens[1]);
        intermediate_code.push_back({lc, "(AD,1) (C," + tokens[1] + ")"});
    }
    else if (mnemonic == "END" || mnemonic == "LTORG")
    {
        if (mnemonic == "LTORG")
        {
            intermediate_code.push_back({lc, "(AD,4)"});
        }
        else
        {
            intermediate_code.push_back({lc, "(AD,2)"});
        }

        // Process literals in the current pool
        while (literal_index < literal_table.size())
        {
            literal_table[literal_index].address = lc;
            lc++;
            literal_index++;
        }

        // Only update the pool table if new literals were added in the current segment
        if (new_literals_added)
        {
            pool_table.push_back(literal_index); // Push the index where the literals start
            new_literals_added = false;          // Reset flag after updating pool table
        }
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
        string opClass = mot[mnemonic].first;
        int opcode = mot[mnemonic].second;

        string operand1 = tokens.size() > 1 ? tokens[1] : "";
        string operand2 = tokens.size() > 2 ? tokens[2] : "";

        stringstream icStream;
        icStream << "(IS," << opcode << ")";

        if (register_table.find(operand1) != register_table.end())
        {
            int reg_code1 = register_table[operand1];
            icStream << " (R," << reg_code1 << ")";

            if (!operand2.empty())
            {
                if (register_table.find(operand2) != register_table.end())
                {
                    int reg_code2 = register_table[operand2];
                    icStream << " (R," << reg_code2 << ")";
                }
                else if (operand2.find("='") == 0)
                {
                    auto it = find_if(literal_table.begin(), literal_table.end(),
                                      [&](const Literal &l)
                                      { return l.value == operand2; });

                    if (it == literal_table.end())
                    {
                        Literal literal = {operand2, -1};
                        literal_table.push_back(literal);
                        new_literals_added = true; // New literal added
                    }

                    int literal_index = distance(literal_table.begin(), it) + 1;
                    icStream << " (L," << literal_index << ")";
                }
                else
                {
                    bool symbol_found = false;
                    int symbol_index = 0;
                    for (size_t i = 0; i < symbol_table.size(); ++i)
                    {
                        if (symbol_table[i].first == operand2)
                        {
                            symbol_index = i + 1;
                            symbol_found = true;
                            break;
                        }
                    }
                    if (!symbol_found)
                    {
                        symbol_index = symbol_table.size() + 1;
                        symbol_table.push_back({operand2, -1});
                    }

                    icStream << " (S," << symbol_index << ")";
                }
            }
        }
        else
        {
            bool symbol_found = false;
            int symbol_index = 0;

            for (size_t i = 0; i < symbol_table.size(); ++i)
            {
                if (symbol_table[i].first == operand1)
                {
                    symbol_index = i + 1;
                    symbol_found = true;
                    break;
                }
            }

            if (!symbol_found)
            {
                if (!operand1.empty())
                {
                    symbol_table.push_back({operand1, lc});
                }
                symbol_index = symbol_table.size();
            }

            icStream << " (S," << symbol_index << ")";
        }

        intermediate_code.push_back({lc, icStream.str()});
        lc += 2;
    }
}

void generateMachineCode()
{
    cout << left << setw(8) << "LC" << setw(10) << "OPCODE" << setw(6) << "OP1" << "OP2" << endl;
    cout << string(30, '-') << endl; // Divider line

    for (const auto &entry : intermediate_code)
    {
        int location_counter = entry.first;
        string instruction = entry.second;

        stringstream ss(instruction);
        string opcode, operand1, operand2;

        ss >> opcode;

        int op_num = stoi(opcode.substr(4, opcode.length() - 5));

        // Initialize operand values
        string reg1 = "00";
        string op2 = "00";

        // Check for operands
        if (ss >> operand1)
        {
            if (operand1.find("(R,") != string::npos)
            {
                // Operand1 is a register
                reg1 = operand1.substr(3, operand1.length() - 4);

                // Check if there is a second operand
                if (ss >> operand2)
                {
                    if (operand2.find("(S,") != string::npos)
                    {
                        // Operand2 is a symbol, look it up in the symbol table
                        int symbol_index = stoi(operand2.substr(3, operand2.length() - 4));
                        int address = symbol_table[symbol_index - 1].second;
                        op2 = to_string(address); // Get address from symbol table
                    }
                    else if (operand2.find("(L,") != string::npos)
                    {
                        // Operand2 is a literal, look it up in the literal table
                        int literal_index = stoi(operand2.substr(3, operand2.length() - 4));
                        int address = literal_table[literal_index - 1].address;
                        op2 = to_string(address); // Get address from literal table
                    }
                    else if (operand2.find("(R,") != string::npos)
                    {
                        // Operand2 is also a register
                        op2 = operand2.substr(3, operand2.length() - 4);
                    }
                }
            }
        }

        // Output the machine code line with proper formatting
        cout << left << setw(8) << location_counter
             << setw(10) << op_num
             << setw(6) << reg1
             << op2 << endl;
    }
}

int main()
{
    ifstream inputFile("assignment1.txt");

    if (!inputFile)
    {
        cerr << "Error: Could not open input file!" << endl;
        return 1;
    }

    string line;
    pool_table.push_back(0);

    while (getline(inputFile, line))
    {
        firstPass(line);
    }

    inputFile.clear();
    inputFile.seekg(0, ios::beg);

    while (getline(inputFile, line))
    {
        secondPass(line);
    }

    inputFile.close();

    // Output Symbol Table
    cout << "\nSymbol Table:\n";
    cout << left << setw(15) << "Symbol" << setw(10) << "Address" << endl;
    cout << string(25, '-') << endl;

    for (const auto &entry : symbol_table)
    {

        cout << left << setw(15) << entry.first << setw(10) << entry.second << endl;
    }

    // Output Literal Table
    cout << "\nLiteral Table:\n";
    cout << "Index\tLiteral\tAddress\n";
    for (size_t i = 0; i < literal_table.size(); i++)
    {
        cout << i << "\t" << literal_table[i].value << "\t" << literal_table[i].address << endl;
    }

    // Output Pool Table
    cout << "\nPool Table:\n";
    for (int index : pool_table)
    {
        cout << index << endl;
    }

    // Output Intermediate Code
    cout << "\nIntermediate Code:\n";
    cout << left << setw(8) << "LC" << "IC" << endl;
    cout << string(30, '-') << endl;

    for (const auto &ic : intermediate_code)
    {
        cout << left << setw(8) << ic.first << ic.second << endl;
    }

    cout << endl;
    cout << "Machine Code" << endl;
    generateMachineCode();

    return 0;
}