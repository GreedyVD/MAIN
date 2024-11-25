#include <iostream>
#include <fstream>
#include <cctype>
#include <unordered_map>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

using namespace std;

// Define token types
enum TokenType
{
    KEYWORD,
    DELIMITER,
    OPERATOR,
    IDENTIFIER,
    LITERAL,
    ERROR
};

// List of Java keywords
const vector<string> javaKeywords = {
    "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char", "class",
    "const", "continue", "default", "do", "double", "else", "enum", "extends", "final",
    "finally", "float", "for", "goto", "if", "implements", "import", "instanceof",
    "int", "interface", "long", "native", "new", "package", "private", "protected",
    "public", "return", "short", "static", "strictfp", "super", "switch", "synchronized",
    "this", "throw", "throws", "transient", "try", "void", "volatile", "while", "true",
    "false", "null"};

// List of Java operators
const vector<string> javaOperators = {
    "+", "-", "*", "/", "%", "++", "--", "==", "!=", ">", "<", ">=", "<=", "&&", "||",
    "!", "&", "|", "^", "~", "<<", ">>", ">>>", "=", "+=", "-=", "*=", "/=", "%=", "&=",
    "|=", "^=", "<<=", ">>=", ">>>="};

// List of Java delimiters
const vector<char> javaDelimiters = {'(', ')', '{', '}', '[', ']', ',', ';', '.', ':'};

// Symbol table for identifiers
unordered_map<string, int> symbolTable;
vector<string> identifierOrder; // Vector to maintain insertion order of identifiers

// Function to check if a string is a keyword
bool isKeyword(const string &str)
{
    return find(javaKeywords.begin(), javaKeywords.end(), str) != javaKeywords.end();
}

// Function to check if a string is an operator
bool isOperator(const string &str)
{
    return find(javaOperators.begin(), javaOperators.end(), str) != javaOperators.end();
}

// Function to check if a character is a delimiter
bool isDelimiter(char ch)
{
    return find(javaDelimiters.begin(), javaDelimiters.end(), ch) != javaDelimiters.end();
}

// Function to display token information
void displayToken(int lineNo, const string &lexeme, TokenType tokenType, int tokenValue)
{
    string tokenTypeStr;
    switch (tokenType)
    {
    case KEYWORD:
        tokenTypeStr = "KEYWORD";
        break;
    case DELIMITER:
        tokenTypeStr = "DELIMITER";
        break;
    case OPERATOR:
        tokenTypeStr = "OPERATOR";
        break;
    case IDENTIFIER:
        tokenTypeStr = "IDENTIFIER";
        break;
    case LITERAL:
        tokenTypeStr = "LITERAL";
        break;
    case ERROR:
        tokenTypeStr = "ERROR";
        break;
    }
    cout << left << setw(10) << lineNo << setw(20) << lexeme
         << setw(15) << tokenTypeStr << setw(10) << tokenValue << endl;
}

// Function to display the symbol table with identifiers
void displaySymbolTable()
{
    cout << "\nSymbol Table for Identifiers:\n";
    cout << left << setw(10) << "Index" << "Identifier" << endl;
    cout << "-----------------------------" << endl;
    int index = 1;
    for (const auto &identifier : identifierOrder)
    {
        cout << left << setw(10) << index++ << identifier << endl;
    }
}

// Lexical analyzer function
void lexicalAnalyzer(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error opening file!" << endl;
        return;
    }

    string line;
    int lineNo = 0;

    cout << left << setw(10) << "Line No." << setw(20) << "Lexeme"
         << setw(15) << "Token" << setw(10) << "Token Value" << endl;
    cout << "--------------------------------------------------------------" << endl;

    while (getline(file, line))
    {
        lineNo++;
        string lexeme;
        for (size_t i = 0; i < line.length(); i++)
        {
            char ch = line[i];

            // Skip whitespace
            if (isspace(ch))
                continue;

            // Check if character is a delimiter
            if (isDelimiter(ch))
            {
                string delimiter(1, ch);
                int tokenValue = distance(javaDelimiters.begin(), find(javaDelimiters.begin(), javaDelimiters.end(), ch));
                displayToken(lineNo, delimiter, DELIMITER, tokenValue);
                continue;
            }

            // Check if character is an operator
            string op(1, ch);
            if (isOperator(op))
            {
                displayToken(lineNo, op, OPERATOR, distance(javaOperators.begin(), find(javaOperators.begin(), javaOperators.end(), op)));
                continue;
            }

            // Check if it's a numeric literal
            if (isdigit(ch))
            {
                lexeme = "";
                while (i < line.length() && (isdigit(line[i]) || line[i] == '.'))
                {
                    lexeme += line[i++];
                }
                i--; // Adjust index after the loop
                displayToken(lineNo, lexeme, LITERAL, 0);
                continue;
            }

            // Check if it's an identifier or keyword
            if (isalpha(ch) || ch == '_')
            {
                lexeme = "";
                while (i < line.length() && (isalnum(line[i]) || line[i] == '_'))
                {
                    lexeme += line[i++];
                }
                i--; // Adjust the index after extra increment

                if (isKeyword(lexeme))
                {
                    displayToken(lineNo, lexeme, KEYWORD, distance(javaKeywords.begin(), find(javaKeywords.begin(), javaKeywords.end(), lexeme)));
                }
                else
                {
                    // Add to symbol table if it's an identifier
                    if (symbolTable.find(lexeme) == symbolTable.end())
                    {
                        symbolTable[lexeme] = symbolTable.size() + 1; // Start indexing from 1
                        identifierOrder.push_back(lexeme);            // Maintain the insertion order
                    }
                    displayToken(lineNo, lexeme, IDENTIFIER, symbolTable[lexeme]);
                }
                continue;
            }

            // If none of the above, it's an error
            displayToken(lineNo, string(1, ch), ERROR, -1);
        }
    }

    file.close();

    // Display the symbol table after lexical analysis
    displaySymbolTable();
}

int main()
{

    lexicalAnalyzer("LexicalJava.txt");

    return 0;
}
