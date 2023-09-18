#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "TokenList.h"
#include "SymbolTable.h"
#include "File.h"
#include "HandleCharacters.h"

void Decompose( char * inputFilename, bool punchCardExtension, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= Decompose
=
= Decomposes a C source file into a series of tokens (lexical analysis).
=
= This function also does the translation phases 1 and 2 of C, which handle multibyte characters (implicitly in this
= case) and removes backslashes followed by newlines, respectively.
=
= The tokens are stored in the tokens structure. tokens is initialized by the function, so it must passed uninitialized
= or empty to the function.
=
= The symbol table is stored in the symbolTable structure. symbolTable is also initialized by the function and has the
= same constraints as tokens.
=
= The caller is responsible for destroying tokens and symbolTable.
=
====================
*/
    
    // Open the file and convert it into a more manageable string.
    int        length;
    char *     sourceString;

    sourceString = ReadFileIntoBuffer( inputFilename, &length );
    
    // Translation phase 1 occurs locally when handling string literals and character constants.
    
    // If the punch card extention is enabled, remove all the DEL characters before parsing the file.
    if ( punchCardExtension ) {
        RemoveDel( sourceString, &length );
    }

    // Translation phase 2. (Remove backslashes followed by newlines).
    RemoveBackslashNewline( sourceString, &length );

    // Translation phase 3 (Lexical Analysis).
    *tokens = InitializeTokenList();
    *symbolTable = InitializeSymbolTable();
    char * slice = sourceString;
    
    while ( *slice != '\0' && slice - sourceString <= length - 1 ) {
        slice = characterFunctions[ ( unsigned char ) ( *slice ) ]( slice, tokens, symbolTable );
    }
    
    free( sourceString );
}

void ExportTokenFile( char * outputFilename, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= ExportTokenFile
=
= Exports a series of tokens and a symbol table to a tokens file, as specified in Appendix 2 of "The Tokens" document.
=
====================
*/
    
    FILE * output = fopen( outputFilename, "wb" );

    // Signature (%TOK-001)
    if ( fwrite( "\x25\x54\x4F\x4B\x2D\x30\x30\x31", 1, 8, output ) < 8 ) {
        fputs( "Error writing to output file.\n", stderr );
        fclose( output );
        exit( 1 );
    }

    // The amount of tokens
    if ( fwrite( &( tokens->size ), sizeof( token_t ), 1, output ) < 1 ) {
        fputs( "Error writing to output file.\n", stderr );
        fclose( output );
        exit( 1 );
    }

    // The tokens
    tokenNode_t * tracer = tokens->head;

    while ( tracer != NULL ) {       
        if ( fwrite( &( tracer->token ), sizeof( token_t ), 1, output ) < 1 ) {
            fputs( "Error writing to output file.\n", stderr );
            fclose( output );
            exit( 1 );
        }
        
        tracer = tracer->next;
    }

    // Symbol table
    token_t hash;

    while ( ReadChart( symbolTable->chart, &hash ) ) {
        // Hash
        if ( fwrite( &hash, 4, 1, output ) < 1 ) {
            fputs( "Error writing to output file.\n", stderr );
            fclose( output );
            exit( 1 );
        }

        // Name
        if ( fwrite( symbolTable->table[ hash ], sizeof( char ), strlen( symbolTable->table[ hash ] ) + 1, output ) < strlen( symbolTable->table[ hash ] ) + 1 ) {
            fputs( "Error writing to output file.\n", stderr );
            fclose( output );
            exit( 1 );
        }
    }

    fclose( output );
}