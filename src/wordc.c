#include "wordc.h"
#include "utils.h"
#include <ctype.h>   // for isspace
#include <stdlib.h>  // for atol()
#include <string.h>  // for memcpy()
#include <alloca.h>

#include <sys/types.h> // for open(), read()
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>     // for EINVAL errno value

#include "read_file_lines.h"
#include "bdb.h"
#include "utils.h"

/**
 * Private, context-specific function.
 *
 * Parses a line, identified by *start* and *end*, returning th
 * address of the end-of-string and sets *value* to the occurance
 * count of that perticular word.
 */
void wcc_interpret_string_number(const char /*in*/ *start, const char /*in*/ *end,
                             const char /*out*/ **word_end, Freq /*out*/ *number)
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

/**
 * Open a word-count table
 */
Result wcc_open(WCC *wcc, const char *name, int create)
{
   Result result;
   DB *db;

   if (!wcc)
   {
      result = EINVAL;
      goto abandon_function;
   }
   
   if ((result = db_create(&db, NULL, 0)))
      goto abandon_function;

   DBFlags flags = create ? DB_CREATE|DB_OVERWRITE : DB_RDONLY;

   if ((result = db->open(db, NULL, name, NULL, DB_BTREE, flags, 0664)))
      goto abandon_db;

   wcc->db = db;
   // bypass db->close() since called needs to use the db handle:
   goto abandon_function;

  abandon_db:
   db->close(db, 0);

  abandon_function:
   return result;
}

/**
 */
bool wcc_add_word(const char *line, const char *end, void *closure)
{
   WCC *wcc = (WCC*)closure;
   DB *db = wcc->db;
   
   const char *end_word;
   Freq   vcount;
   wcc_interpret_string_number(line, end, &end_word, &vcount);

   Rank rank = wcc->static_rank;
   ++wcc->static_position;

   // Record ties as the same rank
   // (ie only increase rank for changed word frequency)
   if (vcount == wcc->static_last_count)
      rank = wcc->static_rank;
   else
   {
      wcc->static_last_count = vcount;
      rank = wcc->static_rank = wcc->static_position;
   }

   // Initialize the key DBT
   int word_len = end_word - line;
   DBT key;
   set_dbt(&key, (void *)line, word_len);

   // Initialize the value DBT
   // Prepare the new record:
   WCR wcr = { vcount, rank };
   DBT value;
   set_dbt(&value, &wcr, sizeof(WCR));

   Result result = db->put(db, NULL, &key, &value, 0);

   if (result)
   {
      db->err(db, result, "Error adding \"%.*s\"", word_len, line);
      return 0;
   }
   else
      return 1;
}


Result wcc_get_word(WCC *wcc, const char *word, Rank *rank, Freq *count)
{
   Result result = 1;

   DBT key;
   set_dbt(&key, (void*)word, strlen(word));
   DBT value;
   memset(&value, 0, sizeof(DBT));

   if ((result = wcc->db->get(wcc->db, NULL, &key, &value, 0)))
      goto abandon_function;

   WCR *wcr = (WCR*)value.data;
   *rank = wcr->rank;
   *count = wcr->count;

  abandon_function:
   return result;
}

void wcc_close(WCC *wcc)
{
   if (wcc)
      wcc->db->close(wcc->db, 0);
}

DataSize wcc_ranker(void *wcc, const char *word, int size)
{
   Rank rank;
   Freq count;

   Result result;

   // Make NULL-terminated copy if *word* is not NULL terminated:
   if (word[size] != '\0')
   {
      char *t = (char*)alloca(size+1);
      memcpy(t, word, size);
      t[size] = '\0';
      word = t;
   }

   if ((result = wcc_get_word((WCC*)wcc, word, &rank, &count)))
   {
      fprintf(stderr, "wcc_ranker failed with %s.\n", db_strerror(result));
      return 0;
   }
   else
      return rank;
}

#ifdef WORDC_MAIN

#include <readargs.h>

#include "bdb.c"
#include "utils.c"
#include "read_file_lines.c"

const char *import_file = "../files/count_1w.txt";
int        add_number_commas = 0;
int        flag_raw_output = 0;
int        flag_save_output = 0;
int        min_word_len = 0;
const char *lookup_word = NULL;

raAction actions[] = {
   {'h', "help", "This help display", &ra_show_help_agent},
   {'b', "buffer", "Buffer size", &ra_int_agent, &RFL_bufflen },
   {-1,  "*lookup", "Lookup word", &ra_string_agent, &lookup_word },
   {'i', "import", "Import file path", &ra_string_agent, &import_file },
   {'c', "commaize", "Add commas to large numbers", &ra_flag_agent, &add_number_commas },
   {'r', "raw-mode", "Output same input", &ra_flag_agent, &flag_raw_output },
   {'S', "save", "Save word and count to table", &ra_flag_agent, &flag_save_output },
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
   Freq value;
   wcc_interpret_string_number(start, end, &end_word, &value);

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
   wcc_interpret_string_number(start, end, &end_word, &value);

   printf("%.*s\t%lu\n", (int)(end_word - start), start, value);

   return 1;
}

/**
 * Echo out each line, exactly as it was read.
 *
 * This line user echos the input, mainly included for use
 * in comparing input with output.
 */
bool rfl_raw_user(const char *start, const char *end, void *closure)
{
   printf("%.*s\n", (int)(end-start), start);
   return 1;
}

void perform_word_lookup(const char *word)
{
   Result result;
   WCC wcc;
   memset(&wcc, 0, sizeof(WCC));
   if (!(result = wcc_open(&wcc, "wordc.db", 0)))
   {
      Rank rank;
      Freq freq;
      result = wcc_get_word(&wcc, word, &rank, &freq);

      if (result)
      {
         if (result == DB_NOTFOUND)
            printf("\"%s\" not found in corpus.\n", word);
         else
            wcc.db->err(wcc.db, result, "Error using wcc_get_word.");
      }
      else
      {
         printf("\"%s\" lookup results:\n", word);
         printf("  frequency in corpus: ");
         commaize_number(freq);
         printf("\n  rank in word list: ");
         commaize_number(rank);
         printf("\n");
      }

      wcc_close(&wcc);
   }

}

int main(int argc, const char **argv)
{
   ra_set_scene(argv, argc, actions, ACTS_COUNT(actions));

   if (ra_process_arguments())
   {
      if (lookup_word)
      {
         perform_word_lookup(lookup_word);
      }
      else if (flag_save_output)
      {
         Result result;
         WCC wcc;
         memset(&wcc, 0, sizeof(WCC));
         if (!(result = wcc_open(&wcc, "wordc.db", 1)))
         {
            result = read_file_lines(import_file, wcc_add_word, &wcc);
            wcc_close(&wcc);
         }
      }
      else
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

