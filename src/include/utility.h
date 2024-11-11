#ifndef UTILITY_H
#define UTILITY_H

#include <vector>
#include <string>
#include <getopt.h>

std::vector<std::string> parse_arguments(int argc, char** argv, const option long_options[], const char* short_options);

#endif //UTILITY_H
