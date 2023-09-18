#include <uchar.h>
#include <assert.h>
#include <limits.h>
#include "q-char-sequence.h"

#if __STDC_VERSION_UCHAR_H__ < 202311L
static_assert( CHAR_BIT == 8, "A char is not 8 bits." );
typedef unsigned char char8_t;
#endif

unsigned char  character         =  'a';
char8_t        UTF_8character    =  'a';
wchar_t        wchar_tCharacter  = L'ç';
char16_t       UTF_16Character   = u'特';
char32_t       UTF_32Character   = U'😳';

unsigned char  characterString[]  =  "character string literal";
char8_t        UTF_8string[]      =  "UTF-8 string literal 🎉";
wchar_t        wchar_tString[]    = L"wchar_t strìng litêrál";
char16_t       UTF_16String[]     = u"UTF-16 字符串文字";
char32_t       UTF_32String[]     = U"UTF-32 string literal 👍";

unsigned char  escapeChars[] = { '\'', '\"', '\?', '\\', '\a', '\b', '\f', '\n', '\r', '\t', '\v', '\123', '\xff', '\uabcd', '\Uabcdef01' };
unsigned char  escapeString[] = { "\' \" \? \\ \a \b \f \n \r \t \v \123 \xff \uabcd \Uabcdef01" };

int                 intConstant               = 12345;
long                longConstant              = 012345L;
long long           longLongConstant          = 0x12345LL;
unsigned int        unsignedIntConstant       = 0b1010U;
unsigned long       unsignedLongConstant      = 12345UL;
unsigned long long  unsignedLongLongConstant  = 12345ULL;
float               floatConstant             = 123.4f;
double              doubleConstant            = 123.4;
long double         longDoubleConstant        = 123.4l;

int  \u90A1\u7528\u5B57\u7B26\u540D\u7A31\u6A19\u8B58\u7B26;
int  \u0024\u0040\u0060;

int main() {
    unsignedLongLongConstant = 9'223'372'036'854'775'808;

    return 0;
}

// slash-slash comment

/* slash-asterisk comment */

void functionDeclaration( int functionParameter ) {
    // Function body

    int variable;

    while ( variable < functionParameter ) {
        /* while loop */
    }

    // Function call
    functionDeclaration( 0 );

    return;
}

#define MACRO 1