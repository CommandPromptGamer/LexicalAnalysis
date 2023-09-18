#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

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

tokenList_t InitializeTokenList() {
/*
====================
=
= InitializeTokenList
=
= Initializes the tokenArray_t data structure.
=
====================
*/

    tokenList_t tokenList;

    tokenList.head = NULL;
    tokenList.tail = NULL;
    tokenList.size = 0;

    return tokenList;
}

tokenNode_t * PushToken( tokenList_t * tokens, token_t token ) {
/*
====================
=
= PushToken
=
= Pushes a token to a tokenList_t structure.
=
= Returns the tail.
=
====================
*/
    
    // No elements on the list
    if ( tokens->tail == NULL ) {
        if ( ( tokens->head = malloc( sizeof( tokenNode_t ) ) ) == NULL ) {
            fputs( "Out of memory.\n", stderr );
            exit( 1 );
        }
        tokens->tail = tokens->head;
    // Elements already present
    } else {
        if ( ( ( tokens->tail )->next = malloc( sizeof( tokenNode_t ) ) ) == NULL ) {
            fputs( "Out of memory.\n", stderr );
            exit( 1 );
        }
        tokens->tail = ( tokens->tail )->next;
    }

    ( tokens->size )++;

    ( tokens->tail )->token = token;
    ( tokens->tail )->next = NULL;

    return tokens->tail;
}

tokenNode_t * PushData( tokenList_t * tokens, void * data, size_t size ) {
/*
====================
=
= PushData
=
= Pushes arbitrary data to a tokenList_t structure.
=
= Returns the tail.
=
====================
*/

    tokenNode_t * tail;

    // Push whole 4 byte segments
    while ( size >= 4 ) {
        tail = PushToken( tokens, *( uint32_t * )data );
        data = ( char * )data + 4;
        size -= 4;
    }
    
    // Push remaining bytes
    if ( size > 0 ) {
        tail = PushToken( tokens, 0x00000000 );
        void * position = &( tail->token );
    
        // This assert should never fail.
        static_assert( sizeof( char ) == 1, "A char is not a byte." );
        while ( size > 0 ) {
            *( char * )position = *( char * )data;
            position = ( char * )position + 1;
            size -= 1;
        }
    }

    return tail;
}

int ReadTokens( tokenNode_t * head, size_t count, void * buffer ) {
/*
====================
=
= ReadTokens
=
= Reads a certain number of tokens from a tokenNode_t structure into a buffer.
=
= Successive calls with NULL or the same head value read successive tokens from the list.
=
= Passing a different head value will reset the current position on the list.
=
= Returns the number of tokens successfully read.
=
====================
*/
    
    static tokenNode_t *  tracer = NULL;
    static tokenNode_t *  list = NULL;
    size_t                read = 0;

    // Begin reading a different list if it is not NULL or the same list.
    if ( head != NULL && head != list ) {
        list = head;
        tracer = head;
    }

    while ( tracer != NULL && read < count ) {
        ( ( token_t * )buffer )[ read ] = tracer->token;
        tracer = tracer->next;
        read++;
    }

    if ( read < count ) {
        list = NULL;
    }

    return read;
}

void DestroyTokenList( tokenList_t tokenList ) {
    tokenNode_t *  tracer = tokenList.head;
    tokenNode_t *  next;

    // Free the linkedList
    while ( tracer != NULL ) {
        next = tracer->next;
        free( tracer );
        tracer = next;
    }
}