#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <stdbool.h>
#include "TokenList.h"

#define SYMBOL_TABLE_SIZE 4819

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

symbolTable_t InitializeSymbolTable();
token_t PushSymbol( symbolTable_t * table, symbol_t symbol, size_t length );
bool PushSymbolToHash( symbolTable_t * symbolTable, symbol_t symbol, size_t length, token_t hash );
bool ReadChart( chartStack_t * chart, token_t * hash );
void DestroySymbolTable( symbolTable_t table );
#endif