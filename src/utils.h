#ifndef UTILS_H
#define UTILS_H

#define LIST_END(a) (a + (sizeof((a)) / sizeof(a[0])))
typedef void (*catstr_user)(const char *catstr, void *data);
void strarr_builder(catstr_user user, void *data, const char **start, const char **end);
void strargs_builder(catstr_user user, void *data, const char *str, ...);


#endif
