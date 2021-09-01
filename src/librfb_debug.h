#ifndef LIBRFB_DEBUG_H
#define LIBRFB_DEBUG_H

//#define LIBRFB_DEBUG

#ifdef LIBRFB_DEBUG
#include <stdio.h>
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#endif