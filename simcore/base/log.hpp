#pragma once
#include <cstdio>

#ifndef NDEBUG
  #define MINERVA_LOG(...) std::printf("[Minerva] " __VA_ARGS__)
#else
  #define MINERVA_LOG(...) do {} while(0)
#endif
