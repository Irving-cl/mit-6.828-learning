
#include <inc/types.h>

uint8_t char_hex_val(char c)
{
}

bool read_hex_from_str(char *buf, uint32_t *hex)
{
    uint8_t  state = 0;
    uint32_t result = 0;

    for (uint8_t i = 0; buf[i] != 0; i++)
    {
        switch (state)
        {
            case 0:
                if (buf[i] == '0')
                {
                    state = 1;
                }
                else if (('1' <= buf[i] && buf[i] <= '9') || ('a' <= buf[i] && buf[i] <= 'f'))
                {
                    state = 3;
                    result 
                }
                break;
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
        }
    }

    *hex = result;

    return (state == 1) || (state == 3);
}
