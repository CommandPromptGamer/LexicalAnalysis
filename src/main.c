#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "TokenList.h"
#include "SymbolTable.h"
#include "Decompose.h"
#include "Recompose/Recompose.h"

typedef struct {
    bool    punchCardExtention;
    char *  input;
    char *  output;
    int     mode;
    bool    yolo;
} options_t;

enum mode_t {
    DECOMPOSE,
    RECOMPOSE,
    ROUNDTRIP
};

int main( int argc, char *argv[] ) {
    options_t  options = { .punchCardExtention = false, .output = "a.tok", .mode = DECOMPOSE, .yolo = false };

    // Option gathering
    if ( argc >= 2 ) {
        options.input = argv[ 1 ];
    } else {
        fputs( "Please enter a filename or file path as the first argument.\n", stderr );
        fprintf( stderr, "%s\n", argv[ 0 ] );
        for ( unsigned int i = 0; i < strlen( argv[ 0 ] ) - 4; i++ ) {
            fputc( ' ', stderr );
        }
        fputs( "here ^", stderr );
        exit( 1 );
    }
    
    for ( int i = 2; i < argc; i++ ) {
        if ( !strcmp( argv[ i ], "--punch" ) ) {
            options.punchCardExtention = true;
        } else if ( !strcmp( argv[ i ], "-o" ) ) {
            if ( argc >= i ) {
                options.output = argv[ i + 1 ];
                i++;
            }
        } else if ( !strcmp( argv[ i ], "-r" ) ) {
            options.mode = RECOMPOSE;
        } else if ( !strcmp( argv[ i ], "-rt" ) ) {
            options.mode = ROUNDTRIP;
        } else if ( !strcmp( argv[ i ], "-yolo" ) ) {
            options.yolo = true;
        // More options may be added here if needed.
        } else {
            fprintf( stderr, "Warning: unrecognized argument ignored: \"%s\".", argv[ i ] );
        }
    }

    if ( options.mode == DECOMPOSE ) {
        tokenList_t    tokens;
        symbolTable_t  symbolTable;
        
        Decompose( options.input, options.punchCardExtention, &tokens, &symbolTable );
        ExportTokenFile( options.output, &tokens, &symbolTable );

        DestroySymbolTable( symbolTable );
        DestroyTokenList( tokens );
    } else if ( options.mode == RECOMPOSE ) {
        RecomposeFromFile( options.input, options.output, options.yolo );
    } else if ( options.mode == ROUNDTRIP ) {
        tokenList_t    tokens;
        symbolTable_t  symbolTable;
        
        // Decompose
        Decompose( options.input, options.punchCardExtention, &tokens, &symbolTable );

        // Turn symbol chart into symbol meaning.
        token_t hash;
        while ( ReadChart( symbolTable.chart, &hash ) ) {
            tokenMeaning[ hash ] = malloc( strlen( symbolTable.table[ hash ] ) + 1 );
            strcpy( tokenMeaning[ hash ], symbolTable.table[ hash ] );
        }

        // Symbol table is no longer needed after turning it into symbolMeaning.
        DestroySymbolTable( symbolTable );

        // Recompose the file and export it.
        FILE * outputFile = fopen( options.output, "wb" );
        Recompose( &tokens, outputFile );
        fclose( outputFile );

        DestroyTokenList( tokens );
        DestroyTokenMeaning();
    }

    return 0;
}