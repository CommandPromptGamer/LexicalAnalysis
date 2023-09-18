#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "TokenList.h"
#include "SymbolTable.h"
#include "Hash.h"
#include "Tokens.h"
#include "IdentifierCharacters.h"
#include "CharacterConstants.h"

/*
This function-like macro compares 2 strings up to 8 characters (excluding null) about five times faster than memcmp.
The ref string, if less than 8 characters, must be preceded by padding nulls e.g. "while" should be "\0\0\0while".
Note that this macro doesn't check the string terminator, unless the size is increased by one, causing the terminator to
be part of the comparison and limiting the string size to 7.
The macro works by loading the first 8 bytes of the strings into registers then shifting left the cmp string by a number
of bits so that it will be preceeded by padding zeroes, causing it to be equal to the ref string, if they are equal.
Depending on the compiler this macro won't use any branching as it will potentially use a set if equal instruction to
set the receiving variable, greatly speeding up the comparison.
If there is no uint64_t memcmp is used instead and STR_8_EQUAL_MEMCMP is defined.
*/
#ifndef UINT64_MAX
#define STR_8_EQUAL( cmp, ref, size ) ( !memcmp( cmp, ref + ( 8 - size ), size ) )
#define STR_8_EQUAL_MEMCMP
#else
#define STR_8_EQUAL( cmp, ref, size ) ( *( uint64_t * )ref == ( *( uint64_t * )cmp << ( ( 8 - size ) * 8 ) ) )
#endif

/*
This macro does the same as the STR_8_EQUAL, but for strings up to 16 characters.
It is about 2.7 times faster than memcmp.
*/
#ifndef UINT64_MAX
#define STR_16_EQUAL( cmp, ref, size ) ( !memcmp( cmp, ref + ( 16 - size ), size ) )
#define STR_16_EQUAL_MEMCMP
#else
#define STR_16_EQUAL( cmp, ref, size ) ( *( uint64_t * )ref == ( *( uint64_t * )cmp << ( ( 8 - ( size - 8 ) ) * 8 ) ) && *( uint64_t * )&ref[ 8 ] == ( *( uint64_t * )&cmp[ size - 8 ] ) )
#endif

/*
==================
Invalid characters
==================
*/

char * _HandleInvalid( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleInvalid
=
= Handles invalid characters that shouldn't be in the source file outsize of certain literals.
=
====================
*/
    
    fputs( "Invalid character located.", stderr );
    exit( 1 );
    
    // Avoid warnings about unused variables
    ( void )slice;
    ( void )tokens;
    ( void )symbolTable;
}

/*
======================
White-space characters
======================
*/

// 9, 11, 12, 32 (horizontal tab, vertical tab, newline, space)
char * _HandleWhiteSpace( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleWhiteSpace
=
= Handles white-space characters.
=
= The white-space characters should be pushed to the tokens directly.
=
====================
*/

    PushToken( tokens, *slice );
    
    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 13 (carriage return)
char * _HandleCarriageReturn( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleCarriageReturn
=
= Handles the carriage return character.
=
= The carriage return character appears before the newline in text files generated in certain operating systems and acts
= as the newline in others.
=
= If there is a newline following it, the carriage return is ignored and the newline is pushed to the tokens.
=
= If there is no newline after the carriage return, the carriage return is used to mark new lines, in this case a
= newline is pushed to the tokens to mark the end of line.
=
====================
*/
    
    PushToken( tokens, '\n' );

    if ( *( slice + 1 ) == '\n' ) {
        // CRLF line delimiter
        return slice + 2;
    } else {
        // CR line delimiter
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

/*
=================================================================================
Characters 33 through 126 except 36, 64, 96 and 48 through 57 ($, @, ` and 0 – 9)
=================================================================================
*/

// 33 !
char * _HandleExclamationMark( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleExclamationMark
=
= Handles the exclamation mark character.
=
= The exclamation mark appears in the logical negation (!) operator and the inequality operator (!=).
=
====================
*/

    // Inequality (!=).
    if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // Logical negation (!).
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 34 "
char * _HandleDoubleQuotes( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleDoubleQuotes
=
= Handles double quotes.
=
= Double quotes not preceded by u8, L, u or U indicates a character string literal. The character string literal is
= added to the tokens.
=
====================
*/   
    int            length;
    tokenNode_t *  lengthNode;
    
    PushToken( tokens, CHARACTER_STRING_LITERAL_TOKEN );
    lengthNode = PushToken( tokens, 0 );
    slice = HandleStringLiteral( slice + 1, &length, tokens );

    lengthNode->token = length;

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 35 #
char * _HandleHash( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleHash
=
= Handles hashes (#).
=
= Hashes indicate preprocessor directives.
=
====================
*/

    // ##
    if ( *( slice + 1 ) == '#' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    } else if ( *( slice + 1 ) == '\n' ) {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Skip spaces after the #
    while ( *( ++slice ) == ' ' );
    
    // #ifdef
    if ( STR_8_EQUAL( slice, "\0\0\0ifdef", 5 ) ) {
        PushToken( tokens, IFDEF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 5;
    // #ifndef
    } else if ( STR_8_EQUAL( slice, "\0\0ifndef", 6 ) ) {
        PushToken( tokens, IFNDEF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 6;
    // #if
    } else if ( STR_8_EQUAL( slice, "\0\0\0\0\0\0if", 2 ) ) {
        PushToken( tokens, IF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 2;
    // #elif
    } else if ( STR_8_EQUAL( slice, "elifndef", 8 ) ) {
        PushToken( tokens, ELIFNDEF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 8;
    // elifdef
    } else if ( STR_8_EQUAL( slice, "\0elifdef", 7 ) ) {
        PushToken( tokens, ELIFDEF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 7;
    // #elif
    } else if ( STR_8_EQUAL( slice, "\0\0\0\0elif", 4 ) ) {
        PushToken( tokens, ELIFNDEF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 4;
    // #else
    } else if ( STR_8_EQUAL( slice, "\0\0\0\0else", 4 ) ) {
        PushToken( tokens, ELSE_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 4;
    // #endif
    } else if ( STR_8_EQUAL( slice, "\0\0\0endif", 5 ) ) {
        PushToken( tokens, ENDIF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 5;
    // #include and #embed
    } else if ( STR_8_EQUAL( slice, "\0include", 7 ) || STR_8_EQUAL( slice, "\0\0\0embed", 5 ) ) {
        int       headerCharSequenceType;
        uint32_t  character;
        
        enum headerCharSequence_t {
            Q_CHAR_SEQUENCE,
            H_CHAR_SEQUENCE,
        };
        
        // #include token
        if ( STR_8_EQUAL( slice, "\0include", 7 ) ) {
            PushToken( tokens, INCLUDE_PREPROCESSING_DIRECTIVE_TOKEN );
            slice += 7;
        // #embed header
        } else {
            PushToken( tokens, EMBED_PREPROCESSING_DIRECTIVE_TOKEN );
            slice += 5;
        }

        // Push whitespace character before header name
        while ( *slice != '<' && *slice != '\"' ) {
            PushToken( tokens, *slice );
            slice++;
        }

        if ( *slice == '\"' ) {
            headerCharSequenceType = Q_CHAR_SEQUENCE;
            PushToken( tokens, HEADER_NAME_QUOTES_TOKEN );
        } else {
            headerCharSequenceType = H_CHAR_SEQUENCE;
            PushToken( tokens, HEADER_NAME_LESS_GREATER_TOKEN );
        }
        
        slice++;

        tokenNode_t *  lengthNode = PushToken( tokens, 0x00000000 );
        size_t         length = 0;

        while ( headerCharSequenceType == Q_CHAR_SEQUENCE ? *slice != '\"' : *slice != '>' ) {
            slice = HandleUTF8Character( slice, &character );
            PushToken( tokens, character );
            length++;
        }

        lengthNode->token = length;
        
    // #define
    } else if ( STR_8_EQUAL( slice, "\0\0define", 6 ) ) {
        PushToken( tokens, DEFINE_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 6;
    // #undef
    } else if ( STR_8_EQUAL( slice, "\0\0\0undef", 5 ) ) {
        PushToken( tokens, UNDEF_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 5;
    // #line
    } else if ( STR_8_EQUAL( slice, "\0\0\0\0line", 4 ) ) {
        PushToken( tokens, LINE_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 4;
    // #error
    } else if ( STR_8_EQUAL( slice, "\0\0\0error", 5 ) ) {
        PushToken( tokens, ERROR_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 5;
    // #warning
    } else if ( STR_8_EQUAL( slice, "\0warning", 7 ) ) {
        PushToken( tokens, WARNING_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 7;
    // #pragma
    } else if ( STR_8_EQUAL( slice, "\0\0pragma", 6 ) ) {
        PushToken( tokens, PRAGMA_PREPROCESSING_DIRECTIVE_TOKEN );
        return slice + 6;
    } else {
        int length = 1; // length is initialized at 1 so the # doesn't get caught as an invalid identifier character.

        while ( validIdentifierCharacter[ ( unsigned char )slice[ length ] ] ) {
            length++;
        }
        
        fprintf( stderr, "Unrecognized preprocessing directive: %.*s is among them.", length, slice );
        exit( 1 );
    }

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 37 %
char * _HandlePercent( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandlePercent
=
= Handles the percent sign character.
=
= The percent sign character is mainly used in remainder (%) and the remainder assignment (%=).
=
= The percent sign may also be used as part of %: that acts as a replacement for #.
=
====================
*/
    
    // Remainder assignment (%=)
    if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // %: as a replacement for #
    } else if ( *( slice + 1 ) == ':' ) {
        // %:%:
        if ( *( slice + 2 ) == '%' && *( slice + 3 ) == ':' ) {
            PushToken( tokens, HASH_HASH_PUNCTUATOR_TOKEN );
            return slice + 4;
        // %:
        } else {
            *( slice + 1 ) = '#';
            return _HandleHash( slice + 1, tokens, symbolTable );
        }
        
    // Remainder operation (%)
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 38 &
char * _HandleAmpersand( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleAmpersand
=
= Handles the ampersand character.
=
= The ampersand character is used in the bitwise AND operator (&), the logical AND (&&) and the bitwise AND assignment
= operator (&=).
=
====================
*/
    
    if ( *( slice + 1 ) == '&' || *( slice + 1 ) == '=' ) {
        // Logical and (&&) and bitwise AND assignment (&=).
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    } else {
        // Bitwise AND (&).
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 39 '
char * _HandleApostrophe( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleApostrophe
=
= Handles the apostrophe character (').
=
= The apostrophe character is used to mark the beginning of a character constant.
=
====================
*/

    char *   next;
    token_t  character;

    next = HandleCharacterConstant( ++slice, &character );

    PushToken( tokens, CHARACTER_CONSTANT_TOKEN );
    PushToken( tokens, character );

    return next + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 40 (
char * _HandleOpeningParenthesis( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleOpeningParenthesis
=
= Handles the opening parenthesis character ("(").
=
= The opening parenthesis character is only used in the opening parenthesis punctuator ("(").
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 41 )
char * _HandleClosingParenthesis( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleClosingParenthesis
=
= Handles the closing parenthesis character (")").
=
= The closing parenthesis character is only used in the closing parenthesis punctuator (")").
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 42 *
char * _HandleAsterisk( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleAsterisk
=
= Handles the asterisk character.
=
= The asterisk appears in the multiplication (*) operator and the multiplication assignment operator (*=).
=
====================
*/

    // multiplication assignment (*=).
    if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // Multiplication (*).
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;

}

// 43 +
char * _HandlePlus( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandlePlus
=
= Handles the plus sign character.
=
= The plus sign appears in the increment operators (++), addition operator (+), unary plus operator (+) and plus assign
= operator (+=).
=
====================
*/

    // ++ and +=
    if ( *( slice + 1 ) == '+' || *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // +
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 44 ,
char * _HandleComma( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleComma
=
= Handles the comma character.
=
= The comma is only used in the comma punctuator (,).
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 45 -
char * _HandleMinus( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleMinus
=
= Handles the plus sign character.
=
= The minus sign appears in the decrement operators (--), subtraction operator (-), unary minus operator (-), minus
= assign operator (-=) and arrow operator (->).
=
====================
*/

    // ->, -= and --
    if ( *( slice + 1 ) == '>' || *( slice + 1 ) == '=' || *( slice + 1 ) == '-' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // -
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 46 .
char * _HandleDot( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleDot
=
= Handles the dot character.
=
= The dot is used in the structure/union member operator (.) and the ellipsis punctuator (...).
=
====================
*/
    
    if ( *( slice + 1 ) == '.' ) {
        PushToken( tokens, TokenHash( slice, 3 ) );
        return slice + 3;
    } else {
        PushToken( tokens, TokenHash( slice, 1 ));
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 47 /
char * _HandleSlash( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSlash
=
= Handles the slash character.
=
= The slash character is used in the division operator (/), division assignment operator (/=) and in comment delimiters (// and / *).
=
= Comments as replaced with a space as required by the spec.
=
====================
*/
    // // comments
    if ( *( slice + 1 ) == '/' ) {
        // Push the comment space
        PushToken( tokens, ' ' );

        // Find and return the position of the newline.
        slice += 2;

        while ( *slice != '\n' ) {
            slice++;
        }

        return slice;
    // /* */ comments
    } else if ( *( slice + 1 ) == '*' ) {
        // Push the comment space
        PushToken( tokens, ' ' );

        // Find and return the position after the end of the comment.
        slice += 2;
        
        while ( !( *slice == '*' && *( slice + 1 ) == '/' ) ) {
            slice++;
        }

        return slice + 2;
    // /=
    } else if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // /
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 58 :
char * _HandleColon( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleColon
=
= Handles the colon character.
=
= The colon character is used in the colon punctuator (:), colon-colon punctuator (::) and the alternative spelling of
= the closing bracket punctuator (]).
=
====================
*/
    
    // ::
    if ( *( slice + 1 ) == ':' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // :>
    } else if ( *( slice + 1 ) == '>' ) {
        PushToken( tokens, CLOSING_BRACKET_PUNCTUATOR_TOKEN );
        return slice + 2;

    // :
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 59 ;
char * _HandleSemicolon( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSemicolon
=
= Handles the semicolon character (;).
=
= The semicolon character is only used in the semicolon punctuator (;).
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 60 <
char * _HandleLess( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleLess
=
= Handles the less-than sign character (<).
=
= The less-than character is used in the left shift operator (<<), less-than operator (<), less-than-or-equal-to
= operator (<=), left-shift assignment operator (<<=), the alternative spelling of the opening bracket punctuator (<:)
= and the alternative spelling of the opening brace punctuator (<%).
=
====================
*/

    if ( *( slice + 1 ) == '<' ) {
        // <<=
        if ( *( slice + 2 ) == '=' ) {
            PushToken( tokens, TokenHash( slice, 3 ) );
            return slice + 3;
        // <<
        } else {
            PushToken( tokens, TokenHash( slice, 2 ) );
            return slice + 2;
        }
    // <=
    } else if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // <:
    } else if ( *( slice + 1 ) == ':' ) {
        PushToken( tokens, OPENING_BRACKET_PUNCTUATOR_TOKEN );
        return slice + 2;
    // <%
    } else if ( *( slice + 1 ) == '%' ) {
        PushToken( tokens, OPENING_BRACE_PUNCTUATOR_TOKEN );
        return slice + 2;
    // <
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 61 =
char * _HandleEqual( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleEqual
=
= Handles the equal sign character.
=
= The equal sign character is used in the simple assignment operator (=) and the equality operator (==).
=
====================
*/
    
    // ==
    if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // =
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 62 >
char * _HandleGreater( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleGreater
=
= Handles the greater-than sign character.
=
= The greater-than sign character is used in the greater than operator (>), right-shift operator (>>) right-shift
= assignment operator and greater-than-or-equal-to operator (>=).
=
====================
*/
    
    if ( *( slice + 1 ) == '>' ) {
        // >>=
        if ( *( slice + 2 ) == '=' ) {
            PushToken( tokens, TokenHash( slice, 3 ) );
            return slice + 3;
        // >>
        } else {
            PushToken( tokens, TokenHash( slice, 2 ) );
            return slice + 2;
        }
    // >=
    } else if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // >
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 63 ?
char * _HandleQuestionMark( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleQuestionMark
=
= Handles the question mark character (;).
=
= The question mark character is only used in the question mark punctuator (?) as part of the ternary conditional
= operator (?:).
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

char * _HandleIdentifier( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable );

// 76 L
char * _HandleCapitalL( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleCapitalL
=
= Handles the capital L character (L).
=
= The capital L character is used as a prefix for wchar_t character constants and string literals and in identifiers.
=
====================
*/
    
    token_t  character;
    char *   next;
    
    // wchar_t character constants
    if ( *( slice + 1 ) == '\'' ) {
        next = HandleCharacterConstant( slice + 2, &character );

        PushToken( tokens, WCHAR_UNDERSCORE_T_CHARACTER_CONSTANT_TOKEN );
        PushToken( tokens, character );

        return next + 1;
    } else if ( *( slice + 1 ) == '\"' ) {
        int            length;
        tokenNode_t *  lengthNode;
        
        PushToken( tokens, WCHAR_UNDERSCORE_T_STRING_LITERAL_TOKEN );
        lengthNode = PushToken( tokens, 0 );
        next = HandleStringLiteral( slice + 2, &length, tokens );

        lengthNode->token = length;

        return next + 1;
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 85 U
char * _HandleCapitalU( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleCapitalU
=
= Handles the capital L character (U).
=
= The capital U character is used as a prefix for UTF-32 character constants and string literals and in identifiers.
=
====================
*/
    
    token_t  character;
    char *   next;
    
    // wchar_t character constants
    if ( *( slice + 1 ) == '\'' ) {
        next = HandleCharacterConstant( slice + 2, &character );

        PushToken( tokens, UTF_32_CHARACTER_CONSTANT_TOKEN );
        PushToken( tokens, character );

        return next + 1;
    } else if ( *( slice + 1 ) == '\"' ) {
        int            length;
        tokenNode_t *  lengthNode;
        
        PushToken( tokens, UTF_32_STRING_LITERAL_TOKEN );
        lengthNode = PushToken( tokens, 0 );
        slice = HandleStringLiteral( slice + 2, &length, tokens );

        lengthNode->token = length;

        return slice + 1;
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}


// 91 [
char * _HandleOpeningBrackets( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleOpeningBrackets
=
= Handles the opening brackets character ([).
=
= The opening brackets character is only used in the array subscript operator.
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 93 ]
char * _HandleClosingBrackets( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleClosingBrackets
=
= Handles the closing brackets character ([).
=
= The opening closing character is only used in the array subscript operator.
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 94 ^
char * _HandleCaret( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleCaret
=
= Handles the caret character (^).
=
= The carat character is used in the bitwise exclusive OR operator and the bitwise exclusive OR assignment operator (^=).
=
====================
*/
    
    // ^=
    if ( *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // ^
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

char * _HandleIdentifier( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable );

// 95 _
char * _HandleUnderscore( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleUnderscore
=
= Handles the underscore character (_).
=
= The underscore character is used in the _Atomic, _BitInt, _Complex, _Decimal128, _Decimal32, _Decimal64, _Generic,
= _Imaginary and _Noreturn keywords along with the _Alignas, _Alignof, _Bool, _Static_assert, _Thread_local keywords,
= which are alternative spellings of alignas, bool, static_assert and thread_local respectively.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 5 ] ] ) {
        // _Bool
        if ( STR_8_EQUAL( slice, "\0\0\0_Bool", 5 ) ) {
            // _Bool is an alternative spelling of the bool keyword from C23+.
            PushToken( tokens, BOOL_KEYWORD_TOKEN );
            return slice + 5;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 7 ] ] ) {
        // _Atomic and _BitInt
        if ( STR_8_EQUAL( slice, "\0_Atomic", 7 ) || STR_8_EQUAL( slice, "\0_BitInt", 7 ) ) {
            PushToken( tokens, TokenHash( slice, 7 ) );
            return slice + 7;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 8 ] ] ) {
        // _Generic
        if ( STR_8_EQUAL( slice, "_Generic", 8 ) ) {
            PushToken( tokens, TokenHash( slice, 8 ) );
            return slice + 8;
        } else if ( STR_8_EQUAL( slice, "_Alignas", 8 ) ) {
            // _Alignas is an alternative spelling of the alignas keyword.
            PushToken( tokens, ALIGNAS_KEYWORD_TOKEN );
            return slice + 8;
        } else if ( STR_8_EQUAL( slice, "_Alignof", 8 ) ) {
            // _Alignof is an alternative spelling of the alignof keyword.
            PushToken( tokens, ALIGNOF_KEYWORD_TOKEN );
            return slice + 8;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 9 ] ] ) {
        // _Noreturn
        if ( STR_16_EQUAL( slice, "\0\0\0\0\0\0\0_Noreturn", 9 ) ) {
            PushToken( tokens, TokenHash( slice, 9 ) );
            return slice + 9;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 10 ] ] ) {
        // _Imaginary, _Decimal32 and _Decimal64
        if ( STR_16_EQUAL( slice, "\0\0\0\0\0\0_Imaginary", 10 ) || STR_16_EQUAL( slice, "\0\0\0\0\0\0_Decimal32", 10 ) || STR_16_EQUAL( slice, "\0\0\0\0\0\0_Decimal64", 10 ) ) {
            PushToken( tokens, TokenHash( slice, 10 ) );
            return slice + 10;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 11 ] ] ) {
        // _Decimal128
        if ( STR_16_EQUAL( slice, "\0\0\0\0\0_Decimal128", 11 ) ) {
            PushToken( tokens, TokenHash( slice, 11 ) );
            return slice + 11;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 13 ] ] ) {
        // _Thread_local
        if ( STR_16_EQUAL( slice, "\0\0\0_Thread_local", 13 ) ) {
            // _Thread_local is an alternative spelling of thread_local
            PushToken( tokens, THREAD_UNDERSCORE_LOCAL_KEYWORD_TOKEN );
            return slice + 13;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 14 ] ] ) {
        // _Static_assert
        if ( STR_16_EQUAL( slice, "\0\0_Static_assert", 14 ) ) {
            // _Static_assert is an alternative spelling of static_assert
            PushToken( tokens, STATIC_UNDERSCORE_ASSERT_KEYWORD_TOKEN );
            return slice + 14;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 97 a
char * _HandleSmallA( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallA
=
= Handles small latin letter A character.
=
= The small latin letter A character is used in the alignas alingof and auto keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // auto
        if ( STR_8_EQUAL( slice, "\0\0\0\0auto", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } if ( !validIdentifierCharacter[ ( unsigned char )slice[ 7 ] ] ) {
        // alignas and alignof
        if ( STR_8_EQUAL( slice, "\0alignas", 7 ) || STR_8_EQUAL( slice, "\0alignof", 7 ) ) {
            PushToken( tokens, TokenHash( slice, 7 ) );
            return slice + 7;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 98 b
char * _HandleSmallB( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallB
=
= Handles small latin letter B character.
=
= The small latin letter B character is used in the bool and break keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // bool
        if ( STR_8_EQUAL( slice, "\0\0\0\0bool", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 5 ] ] ) {
        // break
        if ( STR_8_EQUAL( slice, "\0\0\0break", 5 ) ) {
            PushToken( tokens, TokenHash( slice, 5 ) );
            return slice + 5;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 99 c
char * _HandleSmallC( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallC
=
= Handles small latin letter C character.
=
= The small latin letter C character is used in the case, char, const and constexpr keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // case and char
        if ( STR_8_EQUAL( slice, "\0\0\0\0case", 4 ) || STR_8_EQUAL( slice, "\0\0\0\0char", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 5 ] ] ) {
        // const
        if ( STR_8_EQUAL( slice, "\0\0\0const", 5 ) ) {
            PushToken( tokens, TokenHash( slice, 5 ) );
            return slice + 5;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 8 ] ] ) {
        // continue
        if ( STR_8_EQUAL( slice, "continue", 8 ) ) {
            PushToken( tokens, TokenHash( slice, 8 ) );
            return slice + 8;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 9 ] ] ) {
        // constexpr
        if ( STR_16_EQUAL( slice, "\0\0\0\0\0\0\0constexpr", 9 ) ) {
            PushToken( tokens, TokenHash( slice, 9 ) );
            return slice + 9;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 100 d
char * _HandleSmallD( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallD
=
= Handles small latin letter D character.
=
= The small latin letter D character is used in the do, default and double keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 2 ] ] ) {
        // do
        if ( STR_8_EQUAL( slice, "\0\0\0\0\0\0do", 2 ) ) {
            PushToken( tokens, TokenHash( slice, 2 ) );
            return slice + 2;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 6 ] ] ) {
        // double
        if ( STR_8_EQUAL( slice, "\0\0double", 6 ) ) {
            PushToken( tokens, TokenHash( slice, 6 ) );
            return slice + 6;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 7 ] ] ) {
        // default
        if ( STR_8_EQUAL( slice, "\0default", 7 ) ) {
            PushToken( tokens, TokenHash( slice, 7 ) );
            return slice + 8;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 101 e
char * _HandleSmallE( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallE
=
= Handles small latin letter E character.
=
= The small latin letter E character is used in the else, enum and extern keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // else and enum
        if ( STR_8_EQUAL( slice, "\0\0\0\0else", 4 ) || STR_8_EQUAL( slice, "\0\0\0\0enum", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 6 ] ] ) {
        // extern
        if ( STR_8_EQUAL( slice, "\0\0extern", 6 ) ) {
            PushToken( tokens, TokenHash( slice, 6 ) );
            return slice + 6;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 102 f
char * _HandleSmallF( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallF
=
= Handles small latin letter F character.
=
= The small latin letter F character is used in the for, false and float keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 3 ] ] ) {
        // for
        if ( STR_8_EQUAL( slice, "\0\0\0\0\0for", 3 ) ) {
            PushToken( tokens, TokenHash( slice, 3 ) );
            return slice + 3;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 5 ] ] ) {
        // false and float
        if ( STR_8_EQUAL( slice, "\0\0\0float", 5 ) || STR_8_EQUAL( slice, "\0\0\0false", 5 ) ) {
            PushToken( tokens, TokenHash( slice, 5 ) );
            return slice + 5;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 103 g
char * _HandleSmallG( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallG
=
= Handles small latin letter G character.
=
= The small latin letter G character is only used in the goto keyword.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // goto
        if ( STR_8_EQUAL( slice, "\0\0\0\0goto", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 105 i
char * _HandleSmallI( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallI
=
= Handles small latin letter I character.
=
= The small latin letter I character is in the if, int and inline keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 2 ] ] ) {
        // if
        if ( STR_8_EQUAL( slice, "\0\0\0\0\0\0if", 2 ) ) {
            PushToken( tokens, TokenHash( slice, 2 ) );
            return slice + 2;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 3 ] ] ) {
        // int
        if ( STR_8_EQUAL( slice, "\0\0\0\0\0int", 3 ) ) {
            PushToken( tokens, TokenHash( slice, 3 ) );
            return slice + 3;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 6 ] ] ) {
        // inline
        if ( STR_8_EQUAL( slice, "\0\0inline", 6 ) ) {
            PushToken( tokens, TokenHash( slice, 6 ) );
            return slice + 6;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 108 l
char * _HandleSmallL( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallL
=
= Handles small latin letter L character.
=
= The small latin letter L character is only used in the long keyword.
=
====================
*/

    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // long
        if ( STR_8_EQUAL( slice, "\0\0\0\0long", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 110 n
char * _HandleSmallN( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallN
=
= Handles small latin letter N character.
=
= The small latin letter N character is only used in the nullptr keyword.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 7 ] ] ) {
        // nullptr
        if ( STR_8_EQUAL( slice, "\0nullptr", 7 ) ) {
            PushToken( tokens, TokenHash( slice, 7 ) );
            return slice + 7;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 114 r
char * _HandleSmallR( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallR
=
= Handles small latin letter R character.
=
= The small latin letter R character is used in the return, register and restrict keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 6 ] ] ) {
        // return
        if ( STR_8_EQUAL( slice, "\0\0return", 6 ) ) {
            PushToken( tokens, TokenHash( slice, 6 ) );
            return slice + 6;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 8 ] ] ) {
        // register and restrict
        if ( STR_8_EQUAL( slice, "register", 8 ) || STR_8_EQUAL( slice, "restrict", 8 ) ) {
            PushToken( tokens, TokenHash( slice, 8 ) );
            return slice + 8;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 115 s
char * _HandleSmallS( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallS
=
= Handles small latin letter S character.
=
= The small latin letter S character is used in the short, signed, sizeof, static, struct, switch and static_assert
= keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 5 ] ] ) {
        // short
        if ( STR_8_EQUAL( slice, "\0\0\0short", 5 ) ) {
            PushToken( tokens, TokenHash( slice, 5 ) );
            return slice + 5;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 6 ] ] ) {
        // signed, sizeof, static, struct and switch
        if ( STR_8_EQUAL( slice, "\0\0sizeof", 6 ) || STR_8_EQUAL( slice, "\0\0struct", 6 ) || STR_8_EQUAL( slice, "\0\0switch", 6 ) || STR_8_EQUAL( slice, "\0\0static", 6 ) || STR_8_EQUAL( slice, "\0\0signed", 6 ) ) {
            PushToken( tokens, TokenHash( slice, 6 ) );
            return slice + 6;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 13 ] ] ) {
        // static_assert
        if ( STR_16_EQUAL( slice, "\0\0\0static_assert", 13 ) ) {
            PushToken( tokens, TokenHash( slice, 13 ) );
            return slice + 13;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 116 t
char * _HandleSmallT( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallT
=
= Handles small latin letter T character.
=
= The small latin letter T character is used in the true, typeof, thread_local and typeof_unqual keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // true
        if ( STR_8_EQUAL( slice, "\0\0\0\0true", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 6 ] ] ) {
        // typeof
        if ( STR_8_EQUAL( slice, "\0\0typeof", 6 ) ) {
            PushToken( tokens, TokenHash( slice, 6 ) );
            return slice + 6;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 7 ] ] ) {
        // typedef
        if ( STR_8_EQUAL( slice, "\0typedef", 7 ) ) {
            PushToken( tokens, TokenHash( slice, 7 ) );
            return slice + 7;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 12 ] ] ) {
        // thread_local
        if ( STR_16_EQUAL( slice, "\0\0\0\0thread_local", 12 ) ) {
            PushToken( tokens, TokenHash( slice, 12 ) );
            return slice + 12;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 13 ] ] ) {
        // typeof_unqual
        if ( STR_16_EQUAL( slice, "\0\0\0typeof_unqual", 13 ) ) {
            PushToken( tokens, TokenHash( slice, 13 ) );
            return slice + 13;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 117 u
char * _HandleSmallU( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallU
=
= Handles small latin letter U character.
=
= The small latin letter U character is used in the union and unsigned keywords.
=
====================
*/
    
    token_t  character;
    char *   next;
    
    // UTF-16 character constants
    if ( *( slice + 1 ) == '\'' ) {
        next = HandleCharacterConstant( slice + 2, &character );
        PushToken( tokens, UTF_16_CHARACTER_CONSTANT_TOKEN );
        PushToken( tokens, character );

        return next + 1;
    // UTF-16 string literals
    } else if ( *( slice + 1 ) == '\"' ) {
        int            length;
        tokenNode_t *  lengthNode;
        
        PushToken( tokens, UTF_16_STRING_LITERAL_TOKEN );
        lengthNode = PushToken( tokens, 0 );
        next = HandleStringLiteral( slice + 2, &length, tokens );

        lengthNode->token = length;

        return next + 1;
    // UTF-8
    } else if ( *( slice + 1 ) == '8' ) {
        // UTF-8 character constants
        if ( *( slice + 2 ) == '\'' ) {
            next = HandleCharacterConstant( slice + 3, &character );
            PushToken( tokens, UTF_8_CHARACTER_CONSTANT_TOKEN );
            PushToken( tokens, character );
            
            return next + 1;
        // UTF-8 string literals
        } else if ( *( slice + 2 ) == '\"' ) {
            int            length;
            tokenNode_t *  lengthNode;
            
            PushToken( tokens, CHARACTER_STRING_LITERAL_TOKEN );
            lengthNode = PushToken( tokens, 0 );
            next = HandleStringLiteral( slice + 3, &length, tokens );

            lengthNode->token = length;

            return next + 1;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 5 ] ] ) {
        // union
        if ( STR_8_EQUAL( slice, "\0\0\0union", 5 ) ) {
            PushToken( tokens, TokenHash( slice, 5 ) );
            return slice + 5;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 8 ] ] ) {
        // unsigned
        if ( STR_8_EQUAL( slice, "unsigned", 8 ) ) {
            PushToken( tokens, TokenHash( slice, 8 ) );
            return slice + 8;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 118 v
char * _HandleSmallV( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallV
=
= Handles small latin letter V character.
=
= The small latin letter V character is used in the void and volatile keywords.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 4 ] ] ) {
        // void
        if ( STR_8_EQUAL( slice, "\0\0\0\0void", 4 ) ) {
            PushToken( tokens, TokenHash( slice, 4 ) );
            return slice + 4;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else if ( !validIdentifierCharacter[ ( unsigned char )slice[ 8 ] ] ) {
        // volatile
        if ( STR_8_EQUAL( slice, "volatile", 8 ) ) {
            PushToken( tokens, TokenHash( slice, 8 ) );
            return slice + 8;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 119 w
char * _HandleSmallW( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleSmallW
=
= Handles small latin letter W character.
=
= The small latin letter W character is only used in the while keyword.
=
====================
*/
    
    if ( !validIdentifierCharacter[ ( unsigned char )slice[ 5 ] ] ) {
        // while
        if ( STR_8_EQUAL( slice, "\0\0\0while", 5 ) ) {
            PushToken( tokens, TokenHash( slice, 5 ) );
            return slice + 5;
        } else {
            return _HandleIdentifier( slice, tokens, symbolTable );
        }
    } else {
        return _HandleIdentifier( slice, tokens, symbolTable );
    }
}

// 123 {
char * _HandleOpeningBraces( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleOpeningBraces
=
= Handles the opening braces character ({).
=
= The opening braces character is only used in the compound-literal operator ({).
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 124 |
char * _HandleVerticalLine( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleVerticalLine
=
= Handles the vertical line character (|).
=
= The vertical line character is used in the bitwise inclusive OR operator (|), logical OR operator (||) and bitwise
= inclusive OR assignment operator (|=).
=
====================
*/
    
    // || and |=
    if ( *( slice + 1 ) == '|' || *( slice + 1 ) == '=' ) {
        PushToken( tokens, TokenHash( slice, 2 ) );
        return slice + 2;
    // |
    } else {
        PushToken( tokens, TokenHash( slice, 1 ) );
        return slice + 1;
    }

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 125 }
char * _HandleClosingBraces( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleClosingBraces
=
= Handles the closing braces character (}).
=
= The closing braces character is only used in the compound-literal operator (}).
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

// 126 ~
char * _HandleTilde( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleTilde
=
= Handles the tilde character (~).
=
= The tilde character is only used in the bitwise complement operator (~).
=
====================
*/
    
    PushToken( tokens, TokenHash( slice, 1 ));

    return slice + 1;

    // Avoid unused variable warning
    ( void )symbolTable;
}

/*
=====
Other
=====
*/

char * _HandleConstant( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
    char *       tracer = slice;
    bool         isFloat = false;
    char         number[ 64 ] = { 0 };
    int          characters = 0;
    long double  ldConstant;
    uint64_t     ullConstant;
    char *       end;
    bool         decimal = false;
    
    // Hexadecimal constants.    
    if ( *slice == '0' && ( *( slice + 1 ) == 'x' || *( slice + 1 ) == 'X' ) ) {
        tracer = slice + 2;
        
        number[ 0 ] = '0';
        number[ 1 ] = 'x';
        characters = 2;
        
        while ( ( *tracer >= '0' && *tracer <= '9' ) || *tracer == '.' || *tracer == '\'' || *tracer == 'e' || *tracer == 'E' || *tracer == 'p' || *tracer == 'P' ) {
            // Tests if the constant is a float.
            if ( !isFloat && ( *tracer == '.' || *tracer == 'e' || *tracer == 'E' || *tracer == 'p' || *tracer == 'P' ) ) {
                isFloat = true;
            }

            // Copies the constant without the apostrophes since the string to something functions can't handle
            // apostrophes.
            if ( *tracer != '\'' ) {
                number[ characters ] = *tracer;
                characters++;
            }

            tracer++;
        }

        end = tracer;
    // Octal constants (floats can only be decimal or hexadecimal, so octal constants are always integer constants)
    } else if ( *slice == '0' && !( *( slice + 1 ) == 'x' || *( slice + 1 ) == 'X' || *( slice + 1 ) == 'b' || *( slice + 1 ) == 'B' ) ) {
        tracer = slice + 1;

        number[ 0 ] = '0';
        characters = 1;

        while ( ( *tracer >= '0' && *tracer <= '9' ) || *tracer == '\'' ) {
            if ( *tracer != '\'' ) {
                number[ characters ] = *tracer;
                characters++;
            }

            tracer++;
        }

        end = tracer;
    // Binary constants
    } else if ( *slice == '0' && ( *( slice + 1 ) == 'b' || *( slice + 1 ) == 'B' ) ) {
        tracer = slice + 2;
        
        number[ 0 ] = '0';
        number[ 1 ] = 'b';
        characters = 2;

        while ( ( *tracer >= '0' && *tracer <= '9' ) || *tracer == '\'' ) {
            if ( *tracer != '\'' ) {
                number[ characters ] = *tracer;
                characters++;
            }

            tracer++;
        }

        end = tracer;
    // Constants without a prefix (decimal).
    } else {
        decimal = true;
        
        while ( ( *tracer >= '0' && *tracer <= '9' ) || *tracer == '.' || *tracer == '\'' || *tracer == 'e' || *tracer == 'E' || *tracer == 'p' || *tracer == 'P' || ( *tracer == '+' && ( *( tracer - 1 ) == 'e' || *( tracer - 1 ) == 'E' ) ) ) {
            if ( !isFloat && ( *tracer == '.' || *tracer == 'e' || *tracer == 'E' || *tracer == 'p' || *tracer == 'P' ) ) {
                isFloat = true;
            }

            if ( *tracer != '\'' ) {
                number[ characters ] = *tracer;
                characters++;
            }

            tracer++;
        }

        number[ characters ] = '\0';

        end = tracer;
    }

    if ( isFloat ) {
        ldConstant = strtold( number, NULL );
    } else {
        static_assert( sizeof( unsigned long long ) >= 8, "A 64-bit value does not fit in an unsigned long long" );
        // Arbitrary-length integers may be needed if the target platform does not have native 64-bit integer types.
        // C23's _BitInt(64) may work, if available.

        // strtoull doesn't work with binary constants as they were added in C2x and this program is not made in that
        // future version, only for it.
        if ( number[ 0 ] == '0' && number[ 1 ] == 'b' ) {
            ullConstant = strtoull( number + 2, NULL, 2 );
        } else {
            ullConstant = strtoull( number, NULL, 0 );
        }
    }

    if ( !isFloat ) {
        if ( *end == 'u' || *end == 'U' ) {
            if ( *( end + 1 ) == 'l' || *( end + 1 ) == 'L' ) {
                // unsigned long long int
                if ( *( end + 2 ) == 'l' || *( end + 2 ) == 'L' ) {
                    PushToken( tokens, UNSIGNED_LONG_LONG_INT_CONSTANT_TOKEN );
                    
                    PushData( tokens, &ullConstant, 8 );

                    return end + 3;
                // unsigned long int
                } else {
                    uint32_t ulConstant = ullConstant;
                    
                    PushToken( tokens, UNSIGNED_LONG_INT_CONSTANT_TOKEN );
                    
                    PushData( tokens, &ulConstant, 4 );

                    return end + 2;
                }
            // unsigned unspecified size
            } else {
                // unsigned long int
                if ( ullConstant <= UINT32_MAX ) {
                    uint32_t uiConstant = ullConstant;
                
                    PushToken( tokens, UNSIGNED_INT_CONSTANT_TOKEN );
                    
                    PushData( tokens, &uiConstant, 4 );

                    return end + 1;
                // unsigned long long int
                } else {
                    PushToken( tokens, UNSIGNED_LONG_LONG_INT_CONSTANT_TOKEN );
                
                    PushData( tokens, &ullConstant, 8 );
                    
                    return end + 1;
                }
            }
        } else if ( *end == 'l' || *end == 'L' ) {
            // long long
            if ( *( end + 1 ) == 'l' || *( end + 1 ) == 'L' ) {
                // The spec requires integers that won't fit in its suffix type to be promoted to its unsigned version if they are not decimal.
                ullConstant <= INT64_MAX || decimal ? PushToken( tokens, LONG_LONG_INT_CONSTANT_TOKEN ) : PushToken( tokens, UNSIGNED_LONG_LONG_INT_CONSTANT_TOKEN );
                
                PushData( tokens, &ullConstant, 8 );

                return end + 2;
            // long
            } else {
                int32_t lConstant = ullConstant;

                ullConstant <= INT32_MAX || decimal ? PushToken( tokens, LONG_INT_CONSTANT_TOKEN ) : PushToken( tokens, UNSIGNED_LONG_INT_CONSTANT_TOKEN );
                
                PushData( tokens, &lConstant, 4 );

                return end + 1;
            }
        // Find the smallest when unspecified
        } else {
            // int
            if ( ullConstant <= INT32_MAX ) {
                int32_t iConstant = ullConstant;
                
                PushToken( tokens, INT_CONSTANT_TOKEN );
                
                PushData( tokens, &iConstant, 4 );

                return end;
            // unsigned int
            } else if ( ullConstant <= UINT32_MAX && !decimal ) {
                uint32_t uiConstant = ullConstant;
                
                PushToken( tokens, UNSIGNED_INT_CONSTANT_TOKEN );
                
                PushData( tokens, &uiConstant, 4 );

                return end;
            // long long
            } else if ( ullConstant <= INT64_MAX ) {
                PushToken( tokens, LONG_LONG_INT_CONSTANT_TOKEN );
                
                PushData( tokens, &ullConstant, 8 );

                return end;
            // unsigned long long
            } else {
                PushToken( tokens, UNSIGNED_LONG_LONG_INT_CONSTANT_TOKEN );
                
                PushData( tokens, &ullConstant, 8 );
                
                return end;
            }
        }
    } else {
        if ( *end == 'f' || *end == 'F' || *end == 'l' || *end == 'L' || *end == 'd' || *end == 'D' ) {
            // float
            if ( *end == 'f' || *end == 'F' ) {
                float  fConstant = ldConstant;
                
                PushToken( tokens, FLOAT_CONSTANT_TOKEN );
                static_assert( sizeof( float ) == 4, "The size of a float is not 4 bytes." );
                PushData( tokens, &fConstant, 4 );

                return end + 1;
            // long double
            } else if ( *end == 'l' || *end == 'L' ) {
                PushToken( tokens, LONG_DOUBLE_CONSTANT_TOKEN );

                // IntelliSense will fail the static assert for long double even when the mode is set to gcc-x64.
                // To avoid error squiggles in the IDE the assert is compiled-out.
                #ifndef __INTELLISENSE__
                static_assert( sizeof( long double ) == 16, "The size of a long double is not 16 bytes." );
                #endif
                PushData( tokens, &ldConstant, 16 );

                return end + 1;
            // Decimal floats
            } else if ( *end == 'd' || *end == 'D' ) {                
                // There isn't much support for decimal floats so, for the time being this section will be compiled-out in most builds.
                #if __STDC_IEC_60559_DFP__ >= 202311L
                _Decimal128  d128Constant = strtod128( number, NULL );
                
                // _Decimal32
                if ( *( end + 1 ) == 'f' || *( end + 1 ) == 'F' ) {
                    _Decimal32  d32Constant = d128Constant;
                    
                    PushToken( tokens, UNDERSCORE_DECIMAL32_CONSTANT_TOKEN );
                    
                    static_assert( sizeof( _Decimal32 ) == 4, "The size of a _Decimal32 is not 4 bytes." );
                    PushData( tokens, &d32Constant, 4 );

                    return end + 2;
                // _Decimal64
                } else if ( *( end + 1 ) == 'd' || *( end + 1 ) == 'D' ) {
                    _Decimal64  d64Constant = d128Constant;
                    
                    PushToken( tokens, UNDERSCORE_DECIMAL64_CONSTANT_TOKEN );
                    
                    static_assert( sizeof( _Decimal64 ) == 8, "The size of a _Decimal64 is not 8 bytes." );
                    PushData( tokens, &d64Constant, 8 );

                    return end + 2;
                // _Decimal128
                } else if ( *( end + 1 ) == 'l' || *( end + 1 ) == 'L' ) {               
                    PushToken( tokens, UNDERSCORE_DECIMAL128_CONSTANT_TOKEN );
                    
                    static_assert( sizeof( _Decimal128 ) == 16, "The size of a _Decimal128 is not 16 bytes." );
                    PushData( tokens, &d128Constant, 16 );

                    return end + 2;
                }
                #else
                fputs( "_DecimalN floats are currently unsupported.\n", stderr );
                exit( 1 );
                #endif
            }
        // Unsuffixed float constants have type double
        } else {
            double  doubleConstant = ldConstant;
            
            PushToken( tokens, DOUBLE_CONSTANT_TOKEN );
            PushData( tokens, &doubleConstant, sizeof( double ) );

            return end;
        }
    }

    fputs( "Invalid constant detected.\n", stderr );
    exit( 1 );

    // Avoid unused variable warning
    ( void )symbolTable;
}

size_t IdentifierLength( char * identifier ) {
/*
====================
=
= IdentifierLength
=
= Returns the length of an identifier pointed by identifier.
=
====================
*/
    
    size_t length = 0;

    while ( validIdentifierCharacter[ *( unsigned char * )identifier ] ) {
        identifier++;
        length++;
    }

    return length;
}

char * _HandleIdentifier( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleIdentifier
=
= Pushes the identifier to the symbol table and its hash to the tokens.
=
====================
*/
    
    size_t    length = IdentifierLength( slice );
    char      floating;
    char *    pos;
    uint32_t  ucnValue;
    int       ucnExpectedLength;
    char *    end;

    // Check if universal character names are legal
    for ( unsigned int i = 0; i < length; i++ ) {
        if ( slice[ i ] == '\\' ) {
            if ( slice[ i + 1 ] == 'u' ) {
                floating = slice[ i + 6 ];
                pos = &( slice[ i + 6 ] );
                *pos = '\0';
                ucnExpectedLength = 4;
            } else if ( slice[ i + 1 ] == 'U' ) {
                floating = slice[ i + 10 ];
                pos = &( slice[ i + 10 ] );
                *pos = '\0';
                ucnExpectedLength = 8;
            } else {
                fprintf( stderr, "Invalid identifier located: %.*s\n", ( int )length, slice );
                exit( 1 );
            }
            
            ucnValue = strtol( &( slice[ i + 2 ] ), &end, 16 );
            *pos = floating;
            
            if ( end < pos ) {
                fprintf( stderr, "Invalid universal character name in identifier %.*s (%.*s): universal character name is too short at %d characters, %d are expected.\n", ( int )length, slice, ( int )( end - &( slice[ i ] ) ), &( slice[ i ] ), ( int )( end - &( slice[ i ] ) ) - 2, ucnExpectedLength );
                exit( 1 );
            }
            
            if ( ucnValue == 0 ) {
                fprintf( stderr, "Invalid universal character name in identifier %.*s: %.*s.\n", ( int )length, slice, ( int )( pos - &( slice[ i ] ) ), &( slice[ i ] ) );
                exit( 1 );
            }
            
            if ( ucnValue < 0x00A0 && !( ucnValue == 0x0024 || ucnValue == 0x0040 || ucnValue == 0x0060 ) ) {
                fprintf( stderr, "Invalid universal character name in identifier %.*s (%.*s): universal character names in identifiers below 0x00A0, other than 0x0024 ($), 0x0040 (@) and 0x0060 (`), are not allowed.\n", ( int )length, slice, ( int )( pos - &( slice[ i ] ) ), &( slice[ i ] ) );
                exit( 1 );
            } else if ( ucnValue >= 0xD800 && ucnValue <= 0xDFFF ) {
                fprintf( stderr, "Invalid universal character name in identifier %.*s (%.*s): universal character names in identifiers with values in range 0xD800 to 0xDFFF inclusive are not allowed.\n", ( int )length, slice, ( int )( pos - &( slice[ i ] ) ), &( slice[ i ] ) );
                exit( 1 );
            } else if ( ucnValue > 0x10FFFF ) {
                fprintf( stderr, "Invalid universal character name in identifier %.*s (%.*s): universal character names in identifiers with values greater than 0x10FFFF are not allowed.\n", ( int )length, slice, ( int )( pos - &( slice[ i ] ) ), &( slice[ i ] ) );
                exit( 1 );
            }

            i += pos - &( slice[ i ] ) - 1;
        }
    }
    
    // Push the entire identifier if all the universal character names are legal
    PushToken( tokens, PushSymbol( symbolTable, slice, length ) );

    return slice + length;
}

/*
==================================
Character 127 ([DEL]). (Not used.)
==================================
*/

char * _HandleDel( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) {
/*
====================
=
= _HandleDel
=
= Handles the [DEL] character.
=
= The [DEL] character is a special character that is meant to be ignored, this function does that.
=
= This function is currently unused as it was noticed that DEL characters must be removed before parsing rather than
= while parsing.
=
= The [DEL] character works by being the representation of a punch card character with all holes punched, so if a
= mistake is made while punching it, it can be rectified by punching the remaining holes and continuing the proper
= punching in the next character.
=
====================
*/
    
    return slice + 1;

    // Avoid unused variable warning
    ( void )slice;
    ( void )tokens;
    ( void )symbolTable;
}

// The characterFunctions definition. The functions 45 through 126 have not yet been added.
char *  ( * const characterFunctions[ 128 ] )( char * slice, tokenList_t * tokens, symbolTable_t * symbolTable ) = { _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleWhiteSpace, _HandleWhiteSpace, _HandleWhiteSpace, _HandleInvalid, _HandleCarriageReturn, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleInvalid, _HandleWhiteSpace, _HandleExclamationMark, _HandleDoubleQuotes, _HandleHash, _HandleInvalid, _HandlePercent, _HandleAmpersand, _HandleApostrophe, _HandleOpeningParenthesis, _HandleClosingParenthesis, _HandleAsterisk, _HandlePlus, _HandleComma, _HandleMinus, _HandleDot, _HandleSlash, _HandleConstant, _HandleConstant, _HandleConstant, _HandleConstant, _HandleConstant, _HandleConstant, _HandleConstant, _HandleConstant, _HandleConstant, _HandleConstant, _HandleColon, _HandleSemicolon, _HandleLess, _HandleEqual, _HandleGreater, _HandleQuestionMark, _HandleInvalid, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleCapitalL, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleCapitalU, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleOpeningBrackets, _HandleIdentifier, _HandleClosingBrackets, _HandleCaret, _HandleUnderscore, _HandleInvalid, _HandleSmallA, _HandleSmallB, _HandleSmallC, _HandleSmallD, _HandleSmallE, _HandleSmallF, _HandleSmallG, _HandleIdentifier, _HandleSmallI, _HandleIdentifier, _HandleIdentifier, _HandleSmallL, _HandleIdentifier, _HandleSmallN, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleSmallR, _HandleSmallS, _HandleSmallT, _HandleSmallU, _HandleSmallV, _HandleSmallW, _HandleIdentifier, _HandleIdentifier, _HandleIdentifier, _HandleOpeningBraces, _HandleVerticalLine, _HandleClosingBraces, _HandleTilde, _HandleInvalid };
                                                                                                                       // NULL             SOH             STX             ETX             EOT             ENQ             ACK             BEL             BS               HT                 LF                  VT               FF                 CR                  SO              SI              DLE             DC1             DC2             DC3             DC4             NAK             SYN             ETB             CAN             EM              SUB             ESC             FS              GS              RS              US              space                  !                      "                #              $               %               &                  '                      (                          )                     *              +             ,             -           .            /               0                1                2                3                4                5                6                7                8                9              :               ;               <            =               >                  ?                 @                A                  B                  C                  D                  E                  F                  G                  H                  I                  J                  K                 L                 M                  N                  O                  P                  Q                  R                  S                  T                 U                 V                  W                  X                  Y                  Z                    [                     \                    ]                  ^                _                 `              a              b              c              d              e              f              g                h                i                j                  k               l                m                n                o                  p                  q                r              s              t              u              v              w                x                  y                  z                   {                     |                    }                 ~              DEL