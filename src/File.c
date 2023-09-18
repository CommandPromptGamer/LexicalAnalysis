#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void * ReadFileIntoBuffer( char * filename, int * fileLength );
int RemoveBackslashNewline( char * string, int * size );

void * ReadFileIntoBuffer( char * filename, int * fileLength ) {
/*
====================
=
= ReadFileIntoBuffer
=
= Takes a filename and reads the file into a buffer that is allocated dynamically and returned.
=
= This buffer must be freed by the caller after use to avoid a leak.
=
= The fileLength pointer is filled with the length of the buffer if not NULL.
=
====================
*/
    
    char  *buffer = NULL;
    FILE  *fp = fopen( filename, "r" );
    int   bufferSize;
    int   length = 0;
    
    if ( fp != NULL ) {
        if ( fseek( fp, 0, SEEK_END ) == 0 ) { // Go to the end of the file.
            bufferSize = ftell( fp ); // Get the size of the file.
            
            if ( bufferSize == -1 ) {
                fputs( "Error reading file.", stderr );
                exit( 1 );
            }
            
            // Allocate memory for the file plus the string terminator null and 64 characters extra to avoid out of bounds accesses in the file handling.
            if ( ( buffer = malloc( ( bufferSize + 65 ) * sizeof( char ) ) ) == NULL ) {
                fputs( "Out of memory.\n", stderr );
                exit( 1 );
            }
            
            fseek( fp, 0, SEEK_SET ); // Go to the start of the file.

            length = fread( buffer, sizeof( char ), bufferSize, fp ); // Read the file into memory.
            if ( ferror( fp ) != 0 ) {
                fputs( "Error reading file.", stderr );
                exit( 1 );
            } else {
                buffer[ length ] = '\0'; // Null at the end of the file string.
            }
        }
        
        fclose( fp );
    } else {
        perror( filename );
        exit( 1 );
    }
    
    if ( fileLength != NULL ) {
        *fileLength = length;
    }
    
    return buffer;
}

int RemoveBackslashNewline( char * string, int * size ) {
/*
====================
=
= RemoveBackslashNewline
=
= Removes the backslashes followed by newlines in the inputted string (C translation phase 2).
=
= The size pointer must point to the location of an int containing the number of characters the string has, this
= location will be updated with the number of characters after removing the backslash-newlines.
=
====================
*/
    
    char *  seeker = string;
    int     charsRemaining = *size;

    while ( *seeker != '\0' ) {
        if ( *seeker == '\\' && *( seeker + 1 ) == '\n' ) {
            memcpy( seeker, seeker + 2, --charsRemaining );
            ( *size ) -= 2;
        } else {
            seeker++;
            charsRemaining--;
        }
    }

    return *size;
}

int RemoveDel( char * string, int * size ) {
    char *  seeker = string;
    int     charsRemaining = *size;

    while ( *seeker != '\0' ) {
        if ( *seeker == 0x7F ) {
            memcpy( seeker, seeker + 1, charsRemaining );
            ( *size ) -= 1;
        } else {
            seeker++;
            charsRemaining--;
        }
    }

    return *size;
}