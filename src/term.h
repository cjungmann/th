#ifndef TERM_H
#define TERM_H

#include <stdint.h>

typedef uint32_t ScrSize;

#define SS_WIDE(x) *(uint16_t*)&(((uint16_t*)&(x))[0])
#define SS_HIGH(x) *(uint16_t*)&(((uint16_t*)&(x))[1])

void show_words(const char **words, int count, void *closure);

const char *cycle_color(void);



#endif
