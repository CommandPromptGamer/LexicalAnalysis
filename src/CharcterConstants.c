#include <stdio.h>
#include <assert.h>
#include "TokenList.h"

char * HandleUTF8Character( char * string, uint32_t * character ) {
/*
====================
=
= HandleUTF8Character
=
= Handles UTF-8 characters.
=
= Reads UTF-8 characters and fills the character pointer with the UTF-32 value of the character.
=
= Returns the position of the next character.
=
====================
*/

    static_assert( sizeof( char ) == 1, "A char is not a byte." );
    int             extraBytes;
    const uint32_t  offsets[ 4 ] = { 0x00000000, 0x00003080, 0x000E2080, 0x03C82080 };

    if ( ( *string & 0b10000000 ) == 0b00000000 ) {
        extraBytes = 0;
    } else if ( ( *string & 0b11100000 ) == 0b11000000 ) {
        extraBytes = 1;
    } else if ( ( *string & 0b11110000 ) == 0b11100000 ) {
        extraBytes = 2;
    } else {
        extraBytes = 3;
    }

    *character = 0;

    switch ( extraBytes ) {
        case 3:
            *character += *( unsigned char * )string++;
            *character <<= 6;
            __attribute__ ( ( fallthrough ) );
        case 2:
            *character += *( unsigned char * )string++;
            *character <<= 6;
            __attribute__ ( ( fallthrough ) );
        case 1:
            *character += *( unsigned char * )string++;
            *character <<= 6;
            __attribute__ ( ( fallthrough ) );
        case 0:
            *character += *( unsigned char * )string++;
    }

    *character -= offsets[ extraBytes ];

    return string;
}

char * HandleCharacterConstant( char * string, token_t * character ) {
/*
====================
=
= HandleCharacterConstant
=
= Handles character constants and characters in string literals.
=
= Characters may be a simple character or an escape sequence. This function handles both cases.
=
= The spec suggests converting escape sequences to their appropriate characters only in translation phase 5, while
= lexical analysis is phase 3. This function will convert escape sequences in phase 3 to simplify the intermediate
= representation. This is allowed by the spec as translation phases are only conceptual models not having to occur in a
= particular implementation.
=
= The string input is the pointer to the beginning of the character.
=
= The character pointer will be filled with the UTF-32 representation of the character.
=
= The returned value is the postion of next character or closing ' or ".
=
====================
*/
    
    char *  end;
    char    floating;
    char *  pos;

    // Escape sequence
    if ( *string == '\\' ) {
        switch ( *( string + 1 ) ) {
            // Simple escape sequence.
            case '\'':
                *character = '\'';
                return string + 2;
                break;
            case '\"':
                *character = '\"';
                return string + 2;
                break;
            case '\?':
                *character = '\?';
                return string + 2;
                break;
            case '\\':
                *character = '\\';
                return string + 2;
                break;
            case 'a':
                *character = '\a';
                return string + 2;
                break;
            case 'b':
                *character = '\b';
                return string + 2;
                break;
            case 'e': // Common non-standard escape sequence, should be disabled in a future pedantic option, if added.
                *character = '\e';
                return string + 2;
                break;
            case 'f':
                *character = '\f';
                return string + 2;
                break;
            case 'n':
                *character = '\n';
                return string + 2;
                break;
            case 'r':
                *character = '\r';
                return string + 2;
                break;
            case 't':
                *character = '\t';
                return string + 2;
                break;
            case 'v':
                *character = '\v';
                return string + 2;
                break;
            // Octal escape sequences
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                floating = *( string + 4 );
                pos = string + 4;
                *pos = '\0';
                
                *character = strtoul( string + 1, &end, 8 );
                *pos = floating;

                return end;
                break;
            // 32-bit universal character names
            case 'U':
                floating = *( string + 10 );
                pos = string + 10;
                *pos = '\0';
                
                *character = strtoul( string + 2, &end, 16 );
                *pos = floating;

                return end;
                break;
            // 16-bit universal character names
            case 'u':
                floating = *( string + 10 );
                pos = string + 10;
                *pos = '\0';
                
                *character = strtoul( string + 2, &end, 16 );
                *pos = floating;

                return end;
                break;
            // Hexadecimal escape sequences
            case 'x':
                *character = strtoul( string + 2, &end, 16 );

                return end;
                break;
            default:
                fprintf( stderr, "Unsupported escape sequence: \"\\%c\".", *( character + 1 ) );
                exit( 1 );
        }
    // Simple character
    } else {
        return HandleUTF8Character( string, character );
    }
}

char * HandleStringLiteral( char * string, int * length, tokenList_t * tokens ) {
/*
====================
=
= HandleStringLiteral
=
= Handles string literals.
=
= Each character of a string literal is pushed to the token and the position of the closing " is returned.
=
= Escape sequences are not resolved as they are supposed to be resolved in translation phase 5 and lexical analysis is
= translation phase 3.
=
====================
*/
    static_assert( sizeof( char ) == 1, "A char is not a byte." );
    uint32_t character;

    *length = 0;
    
    while ( *string != '\"' || *( string - 1 ) == '\\' ) {
        string = HandleCharacterConstant( string, &character );
        PushToken( tokens, character );
        
        ( *length )++;
    }

    return string;
}