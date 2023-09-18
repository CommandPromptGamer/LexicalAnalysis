#include <stdio.h>
#include <inttypes.h>
void PushUniversalCharacterName( uint32_t character, FILE * outputFile );
void PushCharacter( uint32_t character, FILE * outputFile );
void PushUTF8CharactersFromUTF32( uint32_t character, FILE * outputFile );