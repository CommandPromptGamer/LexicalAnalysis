#include "TokenList.h"

token_t TokenHash( char * keyword, size_t length ) {
/*
====================
=
= TokenHash
=
= The token hash function behaves differently when the input string is less than 2 characters long.
=
= If it is at least 2 characters long the pre-hash is the value formed by taking the second character in the token and
= setting it as the most significant byte, the first character as the second most significant byte, the last character
= as the third most significant byte, and finally the second-last character as the fourth most significant byte.
= The reason for this weird byte positioning is that we are loading each set of 2 characters as a 16-bit unsigned
= integer, which is little-endian in a functioning CPU architecture for this hash function, while the strings are not.
=
= When the token length is less than 2, we set all bytes of the pre-hash as the value of the character, except the least
= significant byte, which is set to the value of the character plus 12.
=
= The final hash is equal to the modulo of the pre-hash with 619 plus 128.
= We add 128 so that all hashes are at least 128, so that no collisions with ASCII characters can occur.
=
= As a result of these operations, the maximum value for a hash is 746, and the minimum 128, so a look-up table may have
= a size of at least 746 with the values 0–127 unused, or a size of 618 with an always-applied offset of -128 for
= minimal memory savings at the cost of marginal read performance.
=
====================
*/
    
    token_t hash;

    if ( length >= 2 ) {
        hash = ( ( *( uint16_t * )( keyword ) ) << 16 );
        hash |= *( uint16_t * )( &( keyword[ length - 2 ] ) );
    } else {
        ( ( unsigned char * )&hash )[ 0 ] = *keyword + 12;
        ( ( unsigned char * )&hash )[ 1 ] = *keyword;
        ( ( unsigned char * )&hash )[ 2 ] = *keyword;
        ( ( unsigned char * )&hash )[ 3 ] = *keyword;
    }

    return hash % 619 + 128;
}

token_t IdentifierHash( char * identifier, size_t length ) {
/*
====================
=
= IdentifierHash
=
= The identifier hash function is the remainder of the division of the sum of the characters in the identifier with
= 4073, plus 747.
=
= 4073 is the first prime greater than 4095, which is the minumum number of unique identifiers required by the spec.
=
= 747 is the minumum value for the identifier hash so that it won't collide with the token hashes.
=
====================
*/
    
    token_t  hash = 0;
    
    for ( unsigned int i = 0; i < length; i++ ) {
        hash += *( identifier + i );
    }

    return hash % 4073 + 747;
}