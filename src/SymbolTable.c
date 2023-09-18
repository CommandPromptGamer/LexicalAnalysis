#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "Hash.h"
#include "TokenList.h"

typedef char * symbol_t;

typedef struct _chartList_t {
    uint32_t               hash;
    struct _chartList_t *  next;
} chartStack_t;

typedef struct {
    symbol_t *      table;
    chartStack_t *  chart;
    chartStack_t *  chartTail;
} symbolTable_t;


symbolTable_t InitializeSymbolTable() {
/*
====================
=
= InitializeSymbolTable
=
= Initializes the symbolTable_t data structure.
=
====================
*/

    #define SYMBOL_TABLE_SIZE 4819
    
    symbolTable_t  symbolTable;

    // The table field is the table itself.
    if ( ( symbolTable.table = calloc( SYMBOL_TABLE_SIZE, sizeof( symbol_t ) ) ) == NULL ) {
        fputs( "Out of memory.\n", stderr );
        exit( 1 );
    }

    // The chart field is a stack containing all symbols, useful when serializing the table.
    symbolTable.chart = NULL;
    symbolTable.chartTail = NULL;

    return symbolTable;
}

token_t PushSymbol( symbolTable_t * symbolTable, symbol_t symbol, size_t length ) {
    token_t  hash = IdentifierHash( symbol, length );
    bool     cycled = false;

    // Circular probing
    while ( symbolTable->table[ hash ] != NULL ) {
        // First check if the table is full
        if ( hash == SYMBOL_TABLE_SIZE ) {
            if ( cycled ) {
                fputs( "Maximum number of identifiers reached.\n", stderr );
                exit( 1 );
            // Cycle
            } else {
                hash = 747;
                cycled = true;
            }
        }
        
        if ( symbolTable->table[ hash ] == NULL ) {
            // Position found
            break;
        } else {
            // If the token is already on the table return its hash
            if ( !memcmp( symbol, symbolTable->table[ hash ], length ) ) {
                return hash;
            }
        }
        
        hash++;
    }

    // Push the symbol if it is new
        if ( ( symbolTable->table[ hash ] = malloc( ( strlen( symbol ) + 1 ) * sizeof( char ) ) ) == NULL ) {
            fputs( "Out of memory.\n", stderr );
            exit( 1 );
        }
        memcpy( symbolTable->table[ hash ], symbol, length );
        symbolTable->table[ hash ][ length ] = '\0';

    // Push to the chart
    if ( symbolTable->chart == NULL ) {
        if ( ( symbolTable->chart = malloc( sizeof( chartStack_t ) ) ) == NULL ) {
            fputs( "Out of memory.\n", stderr );
            exit( 1 );
        }
        symbolTable->chartTail = symbolTable->chart;
    } else {
        if ( ( ( symbolTable->chartTail )->next = malloc( sizeof( chartStack_t ) ) ) == NULL ) {
            fputs( "Out of memory.\n", stderr );
            exit( 1 );
        }
        symbolTable->chartTail = ( symbolTable->chartTail )->next;
    }
    
    ( symbolTable->chartTail )->hash = hash;
    ( symbolTable->chartTail )->next = NULL;

    return hash;
}

bool PushSymbolToHash( symbolTable_t * symbolTable, symbol_t symbol, size_t length, token_t hash ) {
    bool override = false;
    
    if ( symbolTable->table[ hash ] == NULL ) {
        free( symbolTable->table[ hash ] );    
        override = true;
    }  
    
    symbolTable->table[ hash ] = malloc( ( length + 1 ) * sizeof( char ) );
    strcpy( symbolTable->table[ hash ], symbol );

    return override;
}

bool ReadChart( chartStack_t * chart, token_t * hash ) {
/*
====================
=
= ReadChart
=
= Reads symbols from the chart successively, the hash pointer is filled with the hash read.
=
= The first call must be with a valid chart value, subsequent calls may have any value for chart.
=
= The function returns true if as symbol was read and false when a symbol could not be read because the end was reached.
=
====================
*/
    
    #define NOPE ( ( void * )1 )
    
    static chartStack_t * tail = NOPE;

    if ( tail == NOPE ) {
        tail = chart;
    }
    
    if ( tail == NULL ) {
        tail = NOPE;
        return false;
    } else {
        *hash = tail->hash;
        tail = tail->next;

        return true;
    }

    #undef NOPE
}

void DestroySymbolTable( symbolTable_t symbolTable ) {
    chartStack_t *  tracer = symbolTable.chart;
    chartStack_t *  next;


    // Free the chart linked list
    while ( tracer != NULL ) {
        next = tracer->next;
        free( symbolTable.table[ tracer->hash ] );
        free( tracer );
        tracer = next;
    }
    
    // Free the table
    free( symbolTable.table );
}