
#include <inc/types.h>

#define UINT8_MAX 255

#define CH_BTW(c, X, Y)  (X <= c && c <= Y)

uint8_t char_hex_val(char c);
bool read_hex_from_str(char *buf, uint32_t *hex);

