#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>
#include "Characters.h"
#include "../TokenList.h"
#include "../Tokens.h"

char * tokenMeaning[ 4819 ] = { "", "", "", "", "", "", "", "", "", "\t", "\n", "\v", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_", "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "", "", "", "", "", "", "->", "\xFF", "\xFF", "", "", "", "", "", "false", "!", "", "|=", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "_Decimal64", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "\xFF", "", "while", "", "", "", "##", "", "", "", "", "", "", "", "", "", "", "", "#", "", "", "", "", "", "", "", "", "enum", "", "+=", "", "", "", "", "", "_BitInt", "#if", "#ifdef", "#ifndef", "#elif", "#elifdef", "#elifndef", "#else", "#endif", "#include", "#embed", "#define", "#undef", "#line", "#error", "#warning", "#pragma", "", "", "", "", "%", "", "", "", "constexpr", "", "", "&&", "", "", "", "", "", "", "", "", "", "", "", "&", "", "", "", "", "", "", "", "", "", "", "", "return", "", "", "", "", "_Decimal32", "", "", "", "", "", "", "alignof", "", "", "", "nullptr", "", "", "*=", "", "", "", "", "", "", "(", "", "", "", "", "<<=", "", "", "", "", "", "", "", "", "", "", "", "", "", ")", "", "", "", "inline", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "*", "", "", "else", "", "", "", "++", "", "thread_local", "", "", "", "", "", "", "", "", "", "+", "_Atomic", "", "unsigned", "", "", "", "", "", "", "!=", "", "", "", "float", "", "", "", "", ",", "", "", "", "", "", "", "--", "", "volatile", "_Imaginary", "", "", "", "", "", "", "", "", "-", "", "", "", "", "case", "", "...", "", "", "", "", "", "", "goto", "", "", "", "", ".", "", "", "", "", "", "default", "", "", "", "", "", "", "", "typedef", "", "", "", "", "/", "", "", "", "", "typeof", "", "", "long", "", "", "", "", "", "", "", "int", ">>=", "", "", "", "union", "", "", "", "_Complex", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "_Noreturn", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "alignas", "", "", "", "", "", "", "", "", "break", "", "", "", "", "", "", "", "", "", "", "", "", "/=", "", "", "", "", "", "", "", "", "auto", "", "", "", "", "", "static", "", "", "", "", "", "", "", "", "double", "", "", "", "struct", "", "restrict", "", "", "", "", "", "", "", "", "", "", "", "", "static_assert", "", "", "", "", "", "_Decimal128", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "sizeof", "&=", "", "", "", "", "", "", "", "", ">=", "", "", "", "", "", "if", "", "", "", "", "", "^=", "", "", "", "", "do", "", "", "::", "for", "", "short", "", "", "_Generic", "", "continue", "{", "", "", ":", "", "", "bool", "||", "", "", "", "[", "", "", "", "", "", "", "", "|", "", "", ";", "", "", "", "", "register", "", "<<", "", "", "", "", "", "", "", "", "}", "%=", "", "<", "-=", "", "", "", "", "", "==", "]", "true", "", "", "", "", "", "", "~", "signed", "", "=", "", "", "", "", "", "", ">>", "^", "", "", "", "", "", "", "", "", "", "switch", ">", "", "", "", "typeof_unqual", "", "extern", "", "", "", "", "", "", "", "", "char", "", "", "", "?", "", "", "", "", "", "", "", "void", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "const", "", "<=", "" };

void DestroyTokenMeaning();

void SpecialCases( uint32_t token, FILE * outputFile, unsigned int * iterator ) {
/*
====================
=
= SpecialCases
=
= Handles special tokens from the token list.
=
= Special cases are mostly compound tokens where the next tokens must be read and pushed in a certain way to the
= recomposed source file.
=
= The value pointed by iterator is incremented the number of additional tokens read.
=
====================
*/
    
    uint32_t     stringLength;
    int32_t      iConstant;
    uint32_t     uiConstant;
    int32_t      lConstant;
    uint32_t     ulConstant;
    int64_t      llConstant;
    uint64_t     ullConstant;
    float        fConstant;
    double       dConstant;
    long double  ldConstant;

    // These integer static asserts can be fixed by replacing the format specifiers and casts in the fprintf functions.
    // A fully portable implementation would have all the fprintf functions with either %ull or %ll, without side-effects.
    static_assert( sizeof( int ) >= 4, "A 32-bit integer does not fit into an int" );
    static_assert( sizeof( unsigned int ) >= 4, "A 32-bit integer does not fit into an int" );
    static_assert( sizeof( long ) >= 4, "A 32-bit integer does not fit into a long" );
    static_assert( sizeof( unsigned long ) >= 4, "A 32-bit integer does not fit into a long" );
    static_assert( sizeof( long long ) >= 8, "A 64-bit integer does not fit into a long long" );
    static_assert( sizeof( unsigned long long ) >= 8, "A 64-bit integer does not fit into a long long" );
    
    switch ( token ) {
        /*
        ===============
        String literals
        ===============
        */
        
        // Character string literals
        case CHARACTER_STRING_LITERAL_TOKEN:
            fputc( '\"', outputFile );
            
            ReadTokens( NULL, 1, &stringLength );
            ( *iterator )++;

            for ( unsigned int i = 0; i < stringLength; i++ ) {
                ReadTokens( NULL, 1, &token );
                PushCharacter( token, outputFile );

                ( *iterator )++;
            }
            
            fputc( '\"', outputFile );
            break;
        
        // UTF-8 string literals
        case UTF_8_STRING_LITERAL_TOKEN:
            fputs( "u8\"", outputFile );
            
            ReadTokens( NULL, 1, &stringLength );
            ( *iterator )++;

            for ( unsigned int i = 0; i < stringLength; i++ ) {
                ReadTokens( NULL, 1, &stringLength );
                PushCharacter( token, outputFile );

                ( *iterator )++;
            }
            
            fputc( '\"', outputFile );
            break;
        
        // wchar_t string literals
        case WCHAR_UNDERSCORE_T_STRING_LITERAL_TOKEN:
            fputs( "L\"", outputFile );
            
            ReadTokens( NULL, 1, &stringLength );
            ( *iterator )++;

            for ( unsigned int i = 0; i < stringLength; i++ ) {
                ReadTokens( NULL, 1, &token );
                PushCharacter( token, outputFile );

                ( *iterator )++;
            }
            
            fputc( '\"', outputFile );
            break;
        
        // UTF-16 string literals
        case UTF_16_STRING_LITERAL_TOKEN:
            fputs( "u\"", outputFile );
            
            ReadTokens( NULL, 1, &stringLength );
            ( *iterator )++;

            for ( unsigned int i = 0; i < stringLength; i++ ) {
                ReadTokens( NULL, 1, &token );
                PushCharacter( token, outputFile );

                ( *iterator )++;
            }
            
            fputc( '\"', outputFile );
            break;
        
        // UTF-32 string literals
        case UTF_32_STRING_LITERAL_TOKEN:
            fputs( "U\"", outputFile );
            
            ReadTokens( NULL, 1, &stringLength );
            ( *iterator )++;

            for ( unsigned int i = 0; i < stringLength; i++ ) {
                ReadTokens( NULL, 1, &token );
                PushCharacter( token, outputFile );

                ( *iterator )++;
            }
            
            fputc( '\"', outputFile );
            break;

        /*
        ============
        Header names
        ============
        */

        // H-char headers (<header.h>)
        case HEADER_NAME_LESS_GREATER_TOKEN:
            fputc( '<', outputFile );
            
            ReadTokens( NULL, 1, &stringLength );
            ( *iterator )++;

            for ( unsigned int i = 0; i < stringLength; i++ ) {
                ReadTokens( NULL, 1, &token );
                PushUTF8CharactersFromUTF32( token, outputFile );

                ( *iterator )++;
            }

            fputc( '>', outputFile );
            break;

        // Q-char headers ("header.h")
        case HEADER_NAME_QUOTES_TOKEN:
            fputc( '\"', outputFile );
            
            ReadTokens( NULL, 1, &stringLength );
            ( *iterator )++;

            for ( unsigned int i = 0; i < stringLength; i++ ) {
                ReadTokens( NULL, 1, &token );
                PushUTF8CharactersFromUTF32( token, outputFile );

                ( *iterator )++;
            }

            fputc( '\"', outputFile );
            break;

        /*
        ===================
        Character constants
        ===================
        */
        
        // Character constants
        case CHARACTER_CONSTANT_TOKEN:
            fputc( '\'', outputFile );
            
            ReadTokens( NULL, 1, &token );
            ( *iterator )++;
            
            PushCharacter( token, outputFile );
            
            //Closing apostrophe
            fputc( '\'', outputFile );
            break;
        
        // UTF-8 character constant
        case UTF_8_CHARACTER_CONSTANT_TOKEN:
            fputs( "u8\'", outputFile );
            
            ReadTokens( NULL, 1, &token );
            ( *iterator )++;
            
            PushCharacter( token, outputFile );
            
            //Closing apostrophe
            fputc( '\'', outputFile );
            break;
        
        // wchar_t character constant
        case WCHAR_UNDERSCORE_T_CHARACTER_CONSTANT_TOKEN:
            fputs( "L\'", outputFile );
            
            ReadTokens( NULL, 1, &token );
            ( *iterator )++;
            
            PushCharacter( token, outputFile );
            
            //Closing apostrophe
            fputc( '\'', outputFile );
            break;
        
        // UTF-16 character constant
        case UTF_16_CHARACTER_CONSTANT_TOKEN:
            fputs( "u\'", outputFile );
            
            ReadTokens( NULL, 1, &token );
            ( *iterator )++;
            
            PushCharacter( token, outputFile );
            
            //Closing apostrophe
            fputc( '\'', outputFile );
            break;
        
        // UTF-32 character constant
        case UTF_32_CHARACTER_CONSTANT_TOKEN:
            fputs( "U\'", outputFile );
            
            ReadTokens( NULL, 1, &token );
            ( *iterator )++;
            
            PushCharacter( token, outputFile );
            
            //Closing apostrophe
            fputc( '\'', outputFile );
            break;
        
        /*
        =================
        Integer constants
        =================
        */
        
        // int constants
        case INT_CONSTANT_TOKEN:
            ReadTokens( NULL, 1, &iConstant );
            fprintf( outputFile, "%d", ( int )iConstant );
            ( *iterator )++;
            break;
        
        // unsigned int constants
        case UNSIGNED_INT_CONSTANT_TOKEN:
            ReadTokens( NULL, 1, &uiConstant );
            fprintf( outputFile, "%u", ( unsigned int )uiConstant );
            fputc( 'u', outputFile );
            ( *iterator )++;
            break;
        
        // long constants
        case LONG_INT_CONSTANT_TOKEN:
            ReadTokens( NULL, 1, &lConstant );
            fprintf( outputFile, "%ld", ( long )lConstant );
            fputc( 'l', outputFile );
            ( *iterator )++;
            break;
        
        // unsigned long constants
        case UNSIGNED_LONG_INT_CONSTANT_TOKEN:
            ReadTokens( NULL, 1, &ulConstant );
            fprintf( outputFile, "%lu", ( unsigned long )ulConstant );
            fputs( "ul", outputFile );
            ( *iterator )++;
            break;
        
        // long long constants
        case LONG_LONG_INT_CONSTANT_TOKEN:
            ReadTokens( NULL, 2, &llConstant );
            fprintf( outputFile, "%lld", ( long long )llConstant );
            fputs( "ll", outputFile );
            ( *iterator ) += 2;
            break;
        
        // unsigned long long constants
        case UNSIGNED_LONG_LONG_INT_CONSTANT_TOKEN:
            ReadTokens( NULL, 2, &ullConstant );
            fprintf( outputFile, "%llu", ( unsigned long long )ullConstant );
            fputs( "ull", outputFile );
            ( *iterator ) += 2;
            break;
        
        /*
        ========================
        Floating-point constants
        ========================
        */
        
        // Float constants
        case FLOAT_CONSTANT_TOKEN:
            static_assert( sizeof( float ) == sizeof( token_t ), "A float is not 4 bytes." );
            ReadTokens( NULL, 1, &fConstant );
            fprintf( outputFile, "%f", fConstant );
            fputc( 'f', outputFile );
            ( *iterator )++;
            break;
        
        // Double constants
        case DOUBLE_CONSTANT_TOKEN:
            static_assert( sizeof( double ) == 2 * sizeof( token_t ), "A double is not 8 bytes." );
            ReadTokens( NULL, 2, &dConstant );
            fprintf( outputFile, "%lf", dConstant );
            ( *iterator ) += 2;
            break;
        
        // long double constants
        case LONG_DOUBLE_CONSTANT_TOKEN:
            // IntelliSense will fail the static assert for long double even when the mode is set to gcc-x64.
            // To avoid error squiggles in the IDE the assert is compiled-out.
            #ifndef __INTELLISENSE__
            static_assert( sizeof( long double ) == 4 * sizeof( token_t ), "A long double is not 16 bytes." );
            #endif
            ReadTokens( NULL, 4, &ldConstant );
            fprintf( outputFile, "%Lf", ldConstant );
            fputc( 'l', outputFile );
            ( *iterator ) += 4;
            break;
    }
}

void Recompose( tokenList_t * tokens, FILE * outputFile ) {
/*
====================
=
= Recompose
=
= Recomposes a series of tokens into a C source file.
=
====================
*/
    
    token_t token;

    for ( unsigned int i = 0; i < tokens->size; i++ ) {
        ReadTokens( tokens->head, 1, &token );

        // Special cases
        if ( *tokenMeaning[ token ] == '\xFF' ) {
            SpecialCases( token, outputFile, &i );
        // Normal tokens
        } else {
            fputs( tokenMeaning[ token ], outputFile );
        }
    }
}

void RecomposeFromFile( char * inputFilename, char * outputFilename, bool yolo ) {
    FILE * inputFile = fopen( inputFilename, "rb" );

    // Error opening file
    if ( inputFile == NULL ) {
        perror( inputFilename );
        exit( 1 );
    }

    // Signature check
    char signature[ 9 ];
    signature[ 8 ] = '\0';

    if ( fread( signature, 1, 8, inputFile ) < 8 ) {
        fprintf( stderr, "%s: Error reading file.", inputFilename );
        exit( 1 );
    }

    // Check signature prefix ("%TOK-")
    if ( memcmp( signature, "\x25\x54\x4F\x4B\x2D", 5 ) ) {
        if ( yolo ) {
            fprintf( stderr, "%s: Signature check failed: expect instability from YOLO mode.\n", inputFilename );
        } else {
            fprintf( stderr, "%s: File signature mismatch. File potentially corrupted.\n" 
                             "Rerun with --yolo to ignore all checks.\n", inputFilename );
            exit( 1 );
        }
        
    }
    
    // Check revision number
    int revision = strtol( signature + 5, NULL, 10 );

    if ( revision > 1 ) {
        if ( yolo ) {
            fprintf( stderr, "%s: File revision check failed (got %d, maximum supported is 1): expect instability from YOLO mode.\n", inputFilename, revision );
        } else {
            fprintf( stderr, "%s: Unsupported file revision \"%d\", maximum supported revision is 1.\n"
                            "Rerun with --yolo to ignore all checks.\n", inputFilename, revision );
            exit( 1 );
        }
    }

    // Token count
    unsigned int tokenCount;
    if ( fread( &tokenCount, 4, 1, inputFile ) < 1 ) {
        fprintf( stderr, "%s: Error reading file.", inputFilename );
        exit( 1 );
    }

    tokenList_t  tokens = InitializeTokenList();
    token_t      token;

    for ( unsigned int i = 0; i < tokenCount; i++ ) {
        if ( fread( &token, 4, 1, inputFile ) < 1 ) {
            fprintf( stderr, "%s: Error reading file.", inputFilename );
        }
        
        PushToken( &tokens, token );
    }

    // Go to the symbol table
    fseek( inputFile, tokenCount * 4 + 12, SEEK_SET );

    // Load symbol table
    uint32_t  symbol;
    char      name[ 64 ];
    size_t    length;

    while( fread( &symbol, 4, 1, inputFile ) ) {
        length = 0;

        // Read symbol name
        do {
            if ( fread( &name[ length ], 1, 1, inputFile ) < 1 ) {
                fprintf( stderr, "%s: Error reading file.", inputFilename );
            }

            length++;
        } while ( name[ length - 1 ] != '\0' );

        if ( symbol > 4819 ) {
            fprintf( stderr, "Malformed file \"%s\": Symbol \"%s\" has value %u, above upper limit 4819 for file revision 1.\n", inputFilename, name, symbol );
            exit( 1 );
        } else if ( symbol < 128 ) {
            fprintf( stderr, "Malformed file \"%s\": Symbol \"%s\" has value %u, bellow lower limit 128 for file revision 1.\n", inputFilename, name, symbol );
            exit( 1 );
        }
        
        tokenMeaning[ symbol ] = malloc( ( strlen( name ) + 1 ) * sizeof( char ) );
        strcpy( tokenMeaning[ symbol ], name );
    }
    
    fclose( inputFile );

    FILE * outputFile = fopen( outputFilename, "w" );

    if ( outputFile == NULL ) {
        perror( inputFilename );
        exit( 1 );
    }

    Recompose( &tokens, outputFile );

    DestroyTokenMeaning();
    fclose( outputFile );
}

void DestroyTokenMeaning() {
/*
====================
=
= DestroyTokenMeaning
=
= Destroys the alloc'ed strings in tokenMeaning.
=
====================
*/
    
    for ( int i = 747; i < 4819; i++ ) {
        free( tokenMeaning[ i ] );
        tokenMeaning[ i ] = NULL;
    }
}