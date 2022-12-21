#include "utils.h"


uint32_t char_to_32(char a, char b, char c, char d) {
    uint32_t res = (uint8_t)a + ((uint8_t)b << 8) + ((uint8_t)c << 16) + ((uint8_t)d << 24);
    return res;
}

uint16_t char_to_16(char a, char b) {
    uint16_t res = (uint8_t)a + ((uint8_t)b << 8);
    return res;
}

uint8_t char_to_8(char a) {
    uint8_t res = (uint8_t)a;
    return res;
}

bool is_zero(char* buf) {
    int i = 0;
    char curr = buf[i];
    while (curr != '\0') {
        if (curr != 0) {
            return false;
        }
        i = i + 1;
        curr = buf[i];
    }
    return true;
}