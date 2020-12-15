#ifndef UTILS_H
#define UTILS_H

#include "bdb.h"

#define LIST_END(a) (a + (sizeof((a)) / sizeof(a[0])))

/** Set of functions and typedef that will combine a set
 * of strings into a stack-based concatenation and return
 * the concatenation to the callback function.
 */
typedef void (*catstr_user)(const char *catstr, void *data);
void strarr_builder(catstr_user user, void *data, const char **start, const char **end);
void strargs_builder(catstr_user user, void *data, const char *str, ...);



DBT *set_dbt(DBT *dbt, void *data, DataSize size);

/** 
 * The function cycle_cursor, and the preceding function pointer
 * typedef, work together to repeated call the cursor until the
 * return of the callback function returns "FALSE", or until the
 * cursor reaches the end of the database.
 */
typedef bool (*cc_user)(DBT *key, DBT *value, void *closure);
Result cycle_cursor(DBC *cursor, DBT *key, cc_user user, void *closure);

void bdberr(Result result, FILE *file, const char *context);

void display_stack_report(int level);

void commaize_number(unsigned long num);
void reuse_terminal_line(void);
  

#endif
