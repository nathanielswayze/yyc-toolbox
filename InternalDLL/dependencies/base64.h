#pragma once
#ifndef _BASE64_H_
#define _BASE64_H_

#include <vector>
#include <string>
typedef unsigned char BYTE;

std::string base64_encode(unsigned char const* bytes_to_encode, size_t in_len);
std::string base64_decode(std::string const& encoded_string);

#endif