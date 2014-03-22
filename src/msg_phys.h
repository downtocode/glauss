#ifndef PHYSENGINE_MSG
#define PHYSENGINE_MSG

#define PRI_ESSENTIAL 1
#define PRI_VERYHIGH 2
#define PRI_HIGH 3
#define PRI_HIGHMED 4
#define PRI_MEDIUM 5
#define PRI_LOWMED 6
#define PRI_LOW 7
#define PRI_VERYLOW 8
#define PRI_SPAM 9

void pprintf(unsigned int priority, const char *format, ...);

#endif
