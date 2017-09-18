#ifndef CC_UTILS_H
#define CC_UTILS_H


/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>
#include "config.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/

#define STR(x)  #x

// concatenate version numbers
#define VERSION2_STR(a,b)   STR(a) "." STR(b)
#define VERSION3_STR(a,b,c) STR(a) "." STR(b) "." STR(c)


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/

#if !defined(CC_STRING_NOT_SUPPORTED) && CC_MAX_OPTIONS_ITEMS > 0
#define OPTIONS_MAX_ITEMS   CC_MAX_OPTIONS_ITEMS
#endif


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

typedef struct cstr_t {
    uint8_t size;
    const char *text;
} cstr_t;

typedef struct str16_t {
    uint8_t size;
    char text[17];
} str16_t;

typedef struct version_t {
    uint8_t major, minor, micro;
} version_t;

typedef struct option_t {
    str16_t label;
    float value;
} option_t;


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

/*
 http://stackoverflow.com/a/15171925/1283578
 8-bit CRC with polynomial x^8+x^6+x^3+x^2+1, 0x14D.
 Chosen based on Koopman, et al. (0xA6 in his notation = 0x14D >> 1):
 http://www.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf
*/
uint8_t crc8(const uint8_t *data, uint32_t len);

int cstr_create(const char *str, cstr_t *dest);
int cstr_serialize(const cstr_t *str, uint8_t *buffer);

int str16_create(const char *str, str16_t *dest);
int str16_serialize(const str16_t *str, uint8_t *buffer);
int str16_deserialize(const uint8_t *data, str16_t *str);

int bytes_to_float(const uint8_t *array, float *pvar);

option_t **options_list_create(uint8_t items_count);
void options_list_destroy(option_t **list);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/


#endif
