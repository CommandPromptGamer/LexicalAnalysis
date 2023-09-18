#include "TokenList.h"
#include "SymbolTable.h"

void Decompose( char * inputFilename, bool punchCardExtension, tokenList_t * tokens, symbolTable_t * symbolTable );
void ExportTokenFile( char * outputFilename, tokenList_t * tokens, symbolTable_t * symbolTable );