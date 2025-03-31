#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#ifdef __arm__
    #define GBA
#endif

typedef enum Keys : unsigned int
{
    KeyLeft = 1,
    KeyRight = 2,
    KeyUp = 4,
    KeyDown = 8
} Keys;


#endif // COMMON_H
