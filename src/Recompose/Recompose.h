#include <stdio.h>

extern char * tokenMeaning[];

void Recompose( tokenList_t * tokens, FILE * outputFile );
void RecomposeFromFile( char * inputFilename, char * outputFilename, bool yolo );
void DestroyTokenMeaning();