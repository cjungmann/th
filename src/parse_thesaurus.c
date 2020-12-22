// -*- compile-command: "cc -ggdb -Wall -Werror -std=c99 -pedantic -o parse parse_thesaurus.c -DPARSE_MAIN -ldb" -*-

#include <stdio.h>
#include <string.h>  // memset, memcpy

#include "parse_thesaurus.h"

bool printing_word_user(const char *str, int size, int newline, void *data)
{
   const char *col = newline ? "[32;1m" : "";
   printf("%s%.*s[m\n", col, size, str);
   return 1;
}

void read_thesaurus_file(FILE *f, bool verbose, word_user_f use_word, void *data)
{
   char buffer[1024];
   char *buffend = buffer + sizeof(buffer);

   char *str, *ptr;
   int slen;
   int bread;
   int initial_word;
   int length_of_partial_word;

   int words_read = 0;

   // Hand-set for first time through:
   initial_word = 1;
   str = ptr = buffer;

   if (verbose)
      printf("Importing thesaurus words.\n");

   while ((bread = fread(ptr, 1, buffend-ptr, f)) > 0)
   {
      // set new end to prevent overrun
      if (bread < buffend-ptr)
         buffend = ptr + bread;

      // New reads always results in word starting the buffer
      str = buffer;

      while (ptr < buffend)
      {
         // read next chunk
         while (ptr < buffend && *ptr != ',' && *ptr != '\n')
            ++ptr;

         if (ptr < buffend)
         {
            // Back-off any spaces
            char *endptr = ptr;
            if (*(endptr-1)=='\r')
               --endptr;

            slen = (int)(endptr - str);

            if (verbose)
            {
               if ((++words_read % 250) == 0)
               {
                  /* printf("\x1b[2K\x1b[1G");   // erase line, then move to column 1 of current line */
                  printf("\x1b[1G");   // erase line, then move to column 1 of current line
                  commaize_number(words_read);
                  printf(" %.*s\x1b[K", slen, str);
               }
            }

            // Exit loop if use_word function returns 0
            if (!(*use_word)(str, slen, initial_word, data))
               break;

            // Newline terminator indicates next word is a line-leading word:
            initial_word = *ptr == '\n';

            str = ++ptr;
         }
      }

      length_of_partial_word = ptr - str;
      memmove(buffer, str, length_of_partial_word);
      ptr = buffer + length_of_partial_word;
   }

   if (verbose)
      printf("\n");
}

#ifdef PARSE_MAIN

#include "bdb.c"
#include "istringt.c"

int wordlist_word_user(const char *str, int size, int newline, void *data)
{
   /* IStringT *ist = (IStringT*)data; */
   if (newline)
      printf("%.*s\n", size, str);

   return 1;
}


void persist_wordlist(FILE *f)
{
   int result;

   IStringT ist;
   init_istringt(&ist);

   if ((result = ist.open(&ist, "thesaurus")))
   {
      printf("Failed to open thesaurus.\n");
      goto abandon_function;
   }

   read_thesaurus_file(f, wordlist_word_user, &ist);

   ist.close(&ist);


  abandon_function:
   ;
}

int main(int argc, const char **argv)
{
   FILE *f = fopen("../files/mthesaur.txt", "r");


   /* read_thesaurus_file(f, printing_word_user, NULL); */
   persist_wordlist(f);

   fclose(f);
   return 0;
}

#endif
