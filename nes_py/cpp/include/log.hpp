#ifndef LOG_H
#define LOG_H
#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <cstring>

#define debug_disabled true

#define LOG(level) \
    if (debug_disabled) {} \
    else std::cerr

enum Level
{
    None,
    Error,
    Info,
    InfoVerbose,
    CpuTrace
};

#endif // LOG_H
