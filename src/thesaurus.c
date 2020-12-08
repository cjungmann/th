#include "bdb.h"
#include "ivtable.h"
#include "thesaurus.h"

#include <string.h>   // for memset()


int save_thesaurus_word(const char *str, int size, int newline, void *data)
{
   Result result;
   DB *db = NULL;    // in case we need to report an error

   IVTable *ivt = (IVTable*)data;

   DBT key;
   set_dbt(&key, (void*)str, size);
   DBT value;
   memset(&value, 0, sizeof(DBT));

   RecID recid;

   int bufflen = 0;
   TREC *trec = NULL;

   if ((result = ivt->get_record_by_key(ivt, &recid, &key, &value)))
   {
      if (result == DB_NOTFOUND)
      {
         // Allocate memory for the record
         bufflen = size + sizeof(TREC);
         char buffer[bufflen];
         memset(&buffer, 0, bufflen);
         trec = (TREC*)buffer;

         // Set the record values
         ++trec->count;
         if (newline)
            ++trec->is_root;
         memcpy(trec->value, str, size);

         value.data = buffer;
         value.size = bufflen;

         if ((result = ivt->add_record_raw(ivt, &recid, &key, &value)))
         {
            db = ivt->get_records_db(ivt);
            db->err(db, result, "Unexpected error adding a record for \"%*.s\".", size, str);
            return 0;
         }
      }
      else
      {
         db = ivt->get_records_db(ivt);
         db->err(db, result, "Unexpected error searching for \"%*.s\".", size, str);
         return 0;
      }
   }
   else
   {
      // Update the existing record value, mainly
      // incrementing the count and updaing is_root
      // if appropriate
      trec = (TREC*)value.data;
      ++trec->count;
      if (newline)
         ++trec->is_root;

      if ((result = ivt->update_record(ivt, recid, &value)))
      {
         db = ivt->get_records_db(ivt);
         db->err(db, result, "Unexpected error updating the record for \"%*.s\".", size, str);
         return 0;
      }
   }

   // Errors will have exited by now.
   // Update inter-word indexes
   if (!result)
   {
      if (newline)
      {
         // Update static root-word recid,
         // but wait to link following words.
         ;
      }
      else
      {
         // Using the static root-word recid, 
         // Add root-to-word link
         // Add word-to-root link
         ;
      }
   }

   return 1;
}

bool thesaurus_dumpster(DBT *key, DBT *value, void *data)
{
   TREC *trec = (TREC*)value->data;
   int word_size = value->size - sizeof(TREC);
   const char *color = trec->is_root ? "\x1b[32;1m" : "";
   printf("%s%5u %.*s\x1b[m\n", color, trec->count, word_size, trec->value);
   return 1;
}
