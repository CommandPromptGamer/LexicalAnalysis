#include "TokenList.h"

char * HandleUTF8Character( char * string, uint32_t * character );
char * HandleCharacterConstant( char * string, token_t * character );
char * HandleStringLiteral( char * string, int * length, tokenList_t * tokens );