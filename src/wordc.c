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

#include "bdb.c"
#include "utils.c"
#include "read_file_lines.c"

void test_open(void)
{
   Result result;
   WCC wcc;
   memset(&wcc, 0, sizeof(WCC));
   if (!(result = wcc_open(&wcc, "wordc.db", 1)))
   {
      printf("Created the database.\n");
   }
   else
      printf("Failed to create the database.\n");
}

bool read_file_lines_user(const char *start, const char *end, void *closure)
{
   const char *end_word;
   uint64_t   value;
   interpret_string_number(start, end, &end_word, &value);

   printf("\"%.*s\"  ", (int)(end_word - start), start);
   commaize_number(value);
   printf(" times.\n");

   return 1;
}

void test_read_file_lines(void)
{
   const char *source = "../files/count_1w2.txt";
   /* const char *source = "wordc.txt"; */
   read_file_lines(source, read_file_lines_user, NULL);
}

int main(int argc, const char **argv)
{
   /* test_open(); */
   test_read_file_lines();

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
