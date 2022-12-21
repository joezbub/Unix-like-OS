#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

uint32_t char_to_32(char a, char b, char c, char d);

uint16_t char_to_16(char a, char b);

uint8_t char_to_8(char a);

bool is_zero(char *buf);

uint8_t get_msb(uint16_t val);

#endif // !UTIL_H