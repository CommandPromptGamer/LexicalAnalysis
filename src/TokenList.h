#ifndef TOKEN_ARRAY_H
#define TOKEN_ARRAY_H
#include <stdlib.h>
#include <stdint.h>

typedef uint32_t token_t;

typedef struct _tokenNode_t {
    token_t                token;
    struct _tokenNode_t *  next;
} tokenNode_t;

typedef struct _tokenList_t {
    tokenNode_t *  head;
    tokenNode_t *  tail;
    size_t         size;
} tokenList_t;

tokenList_t InitializeTokenList();
tokenNode_t * PushToken( tokenList_t * tokens, token_t token );
tokenNode_t * PushData( tokenList_t * tokens, void * data, size_t size );
int ReadTokens( tokenNode_t * head, size_t count, void * buffer );
void DestroyTokenList( tokenList_t tokenList );
#endif