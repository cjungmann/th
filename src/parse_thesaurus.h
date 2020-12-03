#ifndef PARSE_THESAURUS_H
#define PARSE_THESAURUS_H

typedef int (*word_user_f)(const char *str, int size, int newline, void *data);
void read_thesaurus_file(FILE *f, word_user_f use_word, void *data);



#endif
