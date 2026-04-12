#pragma once
// Thin Tracy wrapper. Include this instead of <tracy/Tracy.hpp> directly.
// When INFERNUS_TRACY is not defined, all macros compile to nothing.

#ifdef INFERNUS_TRACY
    #include <tracy/Tracy.hpp>
    #define INFERNUS_ZONE      ZoneScoped
    #define INFERNUS_ZONE_N(n) ZoneScopedN(n)
    #define INFERNUS_FRAME     FrameMark
#else
    #define INFERNUS_ZONE      (void)0
    #define INFERNUS_ZONE_N(n) (void)0
    #define INFERNUS_FRAME     (void)0
#endif
