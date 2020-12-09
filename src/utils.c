#include <string.h>
#include <stdarg.h>
#include "utils.h"

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
   demo_strarr_builder(argc, argv);
}

#endif


/* Local Variables: */
/* compile-command: "b=utils; \*/
/*  cc -Wall -Werror -ggdb        \*/
/*  -std=c99 -pedantic            \*/
/*  -D${b^^}_MAIN -o $b ${b}.c"   \*/
/* End: */
