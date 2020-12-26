#include "wordc.h"
#include "utils.h"
#include <ctype.h>   // for isspace
#include <stdlib.h>  // for atol()
#include <string.h>  // for memcpy()

#include <sys/types.h> // for open(), read()
#include <sys/stat.h>
#include <fcntl.h>

#include "read_file_lines.h"

Result wcc_open(WCC *wcc, const char *name, int create)
{
   Result result;
   DB *db;

   if ((result = db_create(&db, NULL, 0)))
      goto abandon_function;

   db->set_re_len(db, sizeof(uint64_t));
   db->set_re_pad(db, 0);

   DBFlags flags = create ? DB_CREATE|DB_OVERWRITE : DB_RDONLY;

   if ((result = db->open(db, NULL, name, NULL, DB_BTREE, flags, 0667)))
      goto abandon_db;

   wcc->db = db;
   return result;

  abandon_db:
   db->close(db, 0);

  abandon_function:
   return result;
}

void interpret_string_number(const char /*in*/ *start, const char /*in*/ *end,
                             const char /*out*/ **word_end, uint64_t /*out*/ *number)
{
   const char *ptr = end;

   while (isdigit(*--ptr))
      ;

   *number = atol(ptr);

   while (isspace(*ptr))
      --ptr;

   // The word-ending space ends the word
   *word_end = ptr+1;
}

Result wcc_add_word(WCC *wcc, const char *line, const char *end)
{
   const char *end_word;
   uint64_t   value;
   interpret_string_number(line, end, &end_word, &value);

   printf("\"%.*s\"", (int)(end_word - line), line);
   commaize_number(value);
   printf(".\n");

   return 0;
}


#ifdef WORDC_MAIN

#include <readargs.h>

#include "bdb.c"
#include "utils.c"
#include "read_file_lines.c"

const char *import_file = "../files/count_1w.txt";
int        add_number_commas = 0;
int        flag_raw_output = 0;
int        min_word_len = 0;

raAction actions[] = {
   {'h', "help", "This help display", &ra_show_help_agent},
   {'b', "buffer", "Buffer size", &ra_int_agent, &RFL_bufflen },
   {'i', "import", "Import file path", &ra_string_agent, &import_file },
   {'c', "commaize", "Add commas to large numbers", &ra_flag_agent, &add_number_commas },
   {'r', "raw-mode", "Output same input", &ra_flag_agent, &flag_raw_output },
   {'w', "width", "Minimum word length", &ra_int_agent, &min_word_len },
   {'s', "show", "Show settings", &ra_show_values_agent}
};


void print_word(const char *start, const char *end, int min_width)
{
   int word_len = end - start;

   printf("%.*s", word_len, start);

   if (word_len < min_width)
      printf("%*.s", (int)(min_width - word_len), "");
}

/**
 * tweak output for readability, adding commas to numbers, and
 * optionally through the -w option, adding spaces to words to
 * align the numbers column.
 */
bool rfl_commaize_user(const char *start, const char *end, void *closure)
{
   const char *end_word;
   uint64_t   value;
   interpret_string_number(start, end, &end_word, &value);

   print_word(start, end_word, min_word_len);
   fputc(' ', stdout);
   commaize_number(value);
   fputc('\n', stdout);

   return 1;
}

/**
 * For the word count list, this output should be the same
 * as the raw user, but this function parses the line into
 * a word and a number before printing, so it tests both the
 * line-reading ability, but also the line parsing ability.
 */
bool rfl_comparable_user(const char *start, const char *end, void *closure)
{
   const char *end_word;
   uint64_t   value;
   interpret_string_number(start, end, &end_word, &value);

   printf("%.*s\t%lu\n", (int)(end_word - start), start, value);

   return 1;
}

/**
 * Echo out each line, exactly as it was read.
 */
bool rfl_raw_user(const char *start, const char *end, void *closure)
{
   printf("%.*s\n", (int)(end-start), start);
   return 1;
}

int main(int argc, const char **argv)
{
   ra_set_scene(argv, argc, actions, ACTS_COUNT(actions));

   if (ra_process_arguments())
   {
      line_user_f line_user = rfl_comparable_user;
      if (flag_raw_output)
         line_user = rfl_raw_user;
      else if (add_number_commas)
         line_user = rfl_commaize_user;

      int result = read_file_lines(import_file, line_user, NULL);
      if (result)
         printf("There was a failure of read_file_lines (%s).\n", db_strerror(result));
   }

   return 0;
}

#endif


/* Local Variables: */
/* compile-command: "b=wordc; \*/
/*  cc -Wall -Werror -ggdb        \*/
/*  -std=c99 -pedantic            \*/
/*  -ldb  -lreadargs              \*/
/*  -D${b^^}_MAIN -o $b ${b}.c"   \*/
/* End: */
