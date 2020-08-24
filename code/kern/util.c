
#include <kern/util.h>

uint8_t char_hex_val(uint8_t c)
{
    if (CH_BTW(c, 'a', 'f'))
    {
        return c - 'a' + 10;
    }
    else if (CH_BTW(c, 'A', 'F')
    {
        return c - 'A' + 10;
    }
    else if (CH_BTW(c, '0', '9')
    {
        return c - '0';
    }

    return UINT8_MAX;
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
                else if (char_hex_val(buf[i]) != UINT8_MAX)
                {
                    state = 3;
                    result = char_hex_val(buf[i]);
                }
                else
                {
                    goto exit;
                }
                break;
            case 1:
                if (buf[i] == 'x')
                {
                    state = 2;
                }
                else if (char_hex_val(buf[i]) != UINT8_MAX)
                {
                    state = 3;
                    result = (result << 1) | char_hex_val(buf[i]);
                }
                else
                {
                    goto exit;
                }
                break;
            case 2:
                if (char_hex_val(buf[i]) != UINT8_MAX)
                {
                    state = 3;
                    result = (result << 1) | char_hex_val(buf[i]);
                }
                else
                {
                    goto exit;
                }
                break;
            case 3:
                if (char_hex_val(buf[i]) != UINT8_MAX)
                {
                    result = (result << 1) | char_hex_val(buf[i]);
                }
                else
                {
                    goto exit;
                }
                break;
        }
    }

    *hex = result;

exit:
    return (state == 1) || (state == 3);
}

