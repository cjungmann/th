#include <string.h>
#include <stdarg.h>
#include "utils.h"

#include <readargs.h>      // access to main()'s argc for approximate start of stack
#include <sys/resource.h>  // for getrlimit() in display_stack_report()

/**
 * Invokes a callback with a concatenated string of the array of strings.
 *
 * The _**end_ argument should be a pointer to the end of the last element
 * in the array.  In other words, first_array_element + number_of_elements,
 * AKA &array[element_count];
 */
void strarr_builder(catstr_user user, void *data, const char **start, const char **end)
{
   // Count the characters needed to contain the string
   int str_len =  1;       // initialize with count for null-terminator
   const char **ptr = start;
   while (ptr < end)
   {
      str_len += strlen(*ptr);
      ++ptr;
   }

   // Allocate memory for concatented string
   char buffer[str_len];

   // Copy the strings to the concatentated string
   char *bptr = buffer;
   int cur_len;
   ptr = start;
   while (ptr < end)
   {
      cur_len = strlen(*ptr);
      memcpy(bptr, *ptr, cur_len);
      bptr += cur_len;

      ++ptr;
   }

   *bptr = '\0';

   // Send it back to the user:
   (*user)(buffer, data);
}

/**
 * Supplies strarr_builder() with appropriate string array
 * from a variable length argument list.
 */
void strargs_builder(catstr_user user, void *data, const char *str, ...)
{
   va_list args;

   // Scan string to count chars needed
   int count = 0;
   va_start(args, str);
   const char *ptr = str;
   while (ptr)
   {
      ++count;
      ptr = va_arg(args, const char*);
   }
   va_end(args);

   if (count)
   {
      // Prep string array to pass on to strarr_builder()
      const char *list[count];
      const char **lptr = list;

      // Rescan, saving pointers to *list*
      va_start(args, str);
      ptr = str;
      while (ptr)
      {
         *lptr++ = ptr;
         ptr = va_arg(args, const char*);
      }
      va_end(args);

      strarr_builder(user, data, list, lptr);
   }
   else
      (*user)(NULL, data);
}


/**
 * Shorthand function for preparing a DBT
 */
DBT* set_dbt(DBT *dbt, void *data, DataSize size)
{
   memset(dbt, 0, sizeof(DBT));
   if (size)
   {
      dbt->data = data;
      dbt->size = size;
   }
   return dbt;
}


/**
 * We assume that the cursor is already set to the first
 * matched record, so we use it before continuing.
 */
Result cycle_cursor(DBC *cursor, DBT *key, cc_user user, void *closure)
{
   DBT value;
   memset(&value, 0, sizeof(DBT));

   Result result;

   if ((result = cursor->get(cursor, key, &value, DB_CURRENT)))
      goto abandon_function;

   while ((*user)(key, &value, closure))
   {
      if ((result = cursor->get(cursor, key, &value, DB_NEXT)))
         goto abandon_function;
   }

  abandon_function:
   return result;
}

/**
 * Use this function to report errors without your code checking
 * for errors.  Use this by putting the db function call as the
 * first argument of the function:
 *
 * bdberr(rrt_add_link(db, 5, 11), stderr, "Adding a link");
 */
void bdberr(Result result, FILE *file, const char *context)
{
   if (result)
   {
      const char *errstr = db_strerror(result);
      fprintf(file, "Error \"\x1b[31;1m%s\x1b[m\"", errstr);
      if (context)
         fprintf(file, " (%s)", context);
      fprintf(file, "\n");
   }
}

void display_stack_report(int level)
{
   struct rlimit rlim;
   memset(&rlim, 0, sizeof(rlim));

   int result = getrlimit(RLIMIT_STACK, &rlim);

   unsigned long start = (unsigned long)g_scene.args;
   unsigned long stack_used;
   unsigned long end;

   end = (unsigned long)&end;

   if (end > start)
      stack_used = end - start;
   else
      stack_used = start - end;

   if (level > 2)
   {
      printf("stack used %lu bytes.\n", stack_used);
      printf("  starting at %lu\n", start);
      printf("  ending at %lu\n", (unsigned long)&result);
      printf("\n");
   }

   if (level > 1)
   {
      printf("Stack rlimit cur: %lu\n", rlim.rlim_cur);
      printf("Stack rlimit max: %lu\n", rlim.rlim_max);
      printf("\n");
   }

   commaize_number(stack_used);
   printf(" of ");
   commaize_number(rlim.rlim_cur);
   printf(" (%f%% of available stack).\n",
          (double)stack_used * 100.0 / (double)rlim.rlim_cur);
}

/**
 * Add commas to printout of integer values.
 */
void commaize_number(unsigned long num)
{
   if (num > 0)
   {
      // recurse to reverse order
      commaize_number(num / 1000);

      if (num > 1000)
         printf(",%03lu", num % 1000);
      else
         printf("%lu", num);
   }
}

/**
 * Doesn't do much, (erase and reuse line), but it's easier
 * to find and use by making it a simple function.
 */
void reuse_terminal_line(void)
{
   printf("\x1b[2K\x1b[1G");
}

#ifdef UTILS_MAIN

#include <stdio.h>

void cs_user(const char *catstr, void *data)
{
   if (catstr)
      printf("%s\n", catstr);
}

void demo_strarr_builder(int argc, const char **argv)
{
   // The best way to use string builder is to pass it
   // and array of char*.
   const char *list[] = {
      "Hi", " ", "Mom"
   };
   strarr_builder(cs_user, NULL, list, LIST_END(list));

   // Use the command-line arguments, if you want:
   strarr_builder(cs_user, NULL, argv+1, argv+argc);

   // or use a null-terminated list of char* arguments:
   strargs_builder(cs_user, NULL, "Hi", " ", "Mom", NULL);
}

int main(int argc, const char **argv)
{
   // Initialize g_scene, which is used by display_stack_report()
   ra_set_scene(argv, argc, NULL, 0);

   demo_strarr_builder(argc, argv);
   display_stack_report(2);
}

#endif


/* Local Variables: */
/* compile-command: "b=utils;  \*/
/*  cc -Wall -Werror -ggdb     \*/
/*  -std=c99 -pedantic         \*/
/*  -D${b^^}_MAIN -o $b ${b}.c \*/
/*  -lreadargs -ldb"           \*/
/* End: */
