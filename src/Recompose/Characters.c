#include <stdio.h>
#include <inttypes.h>

void PushUniversalCharacterName( uint32_t character, FILE * outputFile ) {
/*
====================
=
= PushUniversalCharacterName
=
= Pushes a universal character name to the output file.
=
====================
*/
    
    int significantBytes;
    
    if ( *( ( uint8_t * )&character + 3 ) ) {
        significantBytes = 4;
    } else if ( *( ( uint8_t * )&character + 2 ) ) {
        significantBytes = 3;
    } else if ( *( ( uint8_t * )&character + 1 ) ) {
        significantBytes = 2;
    } else {
        significantBytes = 1;
    }

    significantBytes > 2 ? fprintf( outputFile, "\\U%08X", character ) : fprintf( outputFile, "\\u%04X", character );
}

void PushCharacter( uint32_t character, FILE * outputFile ) {
/*
====================
=
= PushCharacter
=
= Pushes a character to the output file, either as itself or an escape sequence.
=
====================
*/
    
    // Escape sequences
    switch ( character ) {
        case '\'':
            fputs( "\\\'", outputFile );
            break;
        case '\"':
            fputs( "\\\"", outputFile );
            break;
        case '\?':
            fputs( "\\\?", outputFile );
            break;
        case '\\':
            fputs( "\\\\", outputFile );
            break;
        case '\a':
            fputs( "\\a", outputFile );
            break;
        case '\b':
            fputs( "\\b", outputFile );
            break;
        case '\f':
            fputs( "\\f", outputFile );
            break;
        case '\n':
            fputs( "\\n", outputFile );
            break;
        case '\r':
            fputs( "\\r", outputFile );
            break;
        case '\t':
            fputs( "\\t", outputFile );
            break;
        case '\v':
            fputs( "\\v", outputFile );
            break;
        
        // Normal characters
        default:
            // Other control characters are represented on octal.
            if ( character < 32 ) {
                fprintf( outputFile, "\\%o", character );
            // Non-ASCII characters and DEL are represented as a universal character name.
            } else if ( character > 126 ) {
                PushUniversalCharacterName( character, outputFile );
            // Printable ASCII characters are represented as themselves.
            } else {
                fputc( character, outputFile );
            }
            break;
    }
}

void PushUTF8CharactersFromUTF32( uint32_t character, FILE * outputFile ) {
/*
====================
=
= PushUTF8CharactersFromUTF32
=
= Pushes UTF-8 characters to the output file from a UTF-32 character.
=
====================
*/
    int             bytes;
    const uint32_t  byteMask = 0xBF;
	const uint32_t  byteMark = 0x80; 
    uint32_t        charTowrite;
    uint8_t *       target;
    const uint8_t   firstByteMark[] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0 };

    if ( character < 0x80 ) { 
        bytes  = 1;
	} else if ( character < 0x800 ) {
        bytes = 2;
	} else if (character < 0x10000) {
        bytes = 3;
	} else {
        bytes = 4;
	}

    target = ( ( uint8_t * )&charTowrite ) + bytes;

    switch ( bytes ) {
	    case 4:
            *--target = ( ( character | byteMark ) & byteMask );
            character >>= 6;
            __attribute__ ( ( fallthrough ) );
	    case 3:
            *--target = ( ( character | byteMark ) & byteMask );
            character >>= 6;
            __attribute__ ( ( fallthrough ) );
	    case 2:
            *--target = ( ( character | byteMark ) & byteMask );
            character >>= 6;
            __attribute__ ( ( fallthrough ) );
	    case 1:
            *--target = ( character | firstByteMark[ bytes ] );
	}

    fwrite( &charTowrite, 1, bytes, outputFile );
}