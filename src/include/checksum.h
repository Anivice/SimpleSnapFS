#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <array>
#include <cstdint>

std::array<char, 64> sha512sum(const char* _data, uint64_t _dt_len);

#endif //CHECKSUM_H
