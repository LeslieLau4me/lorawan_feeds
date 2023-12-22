/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Base64 encoding & decoding library

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _BASE64_H_
#define _BASE64_H_

#include <iostream>
#include <string>

using namespace std;
class Base64
{
  public:
    string encode(const std::string &input);
    string decode(const std::string &input);

  private:
    static char    code_to_char(uint8_t x);
    static uint8_t char_to_code(char x);
    static int     bin_to_b64_nopad(const uint8_t *in, int size, char *out, int max_len);
    static int     b64_to_bin_nopad(const char *in, int size, uint8_t *out, int max_len);
    static int     bin_to_b64(const uint8_t *in, int size, char *out, int max_len);
    static int     b64_to_bin(const char *in, int size, uint8_t *out, int max_len);
};

#endif

/* --- EOF ------------------------------------------------------------------ */
