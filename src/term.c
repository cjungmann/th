#include <stdio.h>    // for perror() and fileno()
#include <string.h>   // for strlen()

#include <sys/ioctl.h>

#include <termios.h>  // for tcgetattr()
#include <unistd.h>   // for tcgetattr(), STDOUT_FILENO
#include <errno.h>

#include "term.h"

ScrSize get_screen_size(void)
{
   // Refer to *man* pages 'ioctl' and 'ioctl_tty'
   struct winsize ws;
   ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

   ScrSize rval;
   SS_WIDE(rval) = ws.ws_col;
   SS_HIGH(rval) = ws.ws_row;

   return rval;
}

/**
 * Returns a keypress without waiting for ENTER.
 *
 * Stores the keypress in a buffer because many keypresses
 * return several characters, starting with "ESC[".
 *
 * Pass your own buffer if you need to preserve the value,
 * or just call the function without arguments to use
 * the built-in buffer.
 */
const char* get_keyp(char *buff, int bufflen)
{
   static char sbuff[40];
   static const int sbufflen = 10;

   // Use static buffer if no buffer provided:
   if (!buff)
   {
      buff = sbuff;
      bufflen = sbufflen;
   }

   struct termios term_old, term_new;
   if (tcgetattr(0, &term_old) < 0)
      perror("tcsetattr()");

   term_new = term_old;

   term_new.c_lflag &= ~(ICANON|ECHO);
   term_new.c_cc[VMIN] = 1;
   term_new.c_cc[VTIME] = 0;
   if (tcsetattr(0, TCSANOW, &term_new) < 0)
      perror("tcsetattr ICANON");

   // char c;
   // while((c=read(0,&c,1))!=EOF)
   //    ;

   int readlen;
   if ((readlen=read(0, buff, bufflen-1)) < 0)
      perror ("read()");
   else
      buff[readlen] = '\0';

   if (tcsetattr(0, TCSADRAIN, &term_old) < 0)
      perror ("tcsetattr ~ICANON");

   return (buff);
}

int get_max_length(const char **words, int count)
{
   int cur, max = 0;
   const char **end = words + count;
   while (words < end)
   {
      cur = strlen(*words);
      if (cur > max)
         max = cur;

      ++words;
   }

   return max;
}

void show_words(const char **words, int count, void *closure)
{
   ScrSize ssize = get_screen_size();
   int maxword = get_max_length(words,count);
   int wide = SS_WIDE(ssize);
   int wordcell = maxword + 4;

   char format[20];
   sprintf(format, " %%-%ds   ", maxword);

   int col;
   const char **ptr = words;
   const char **end = words + count;

   while (ptr < end)
   {
      for (col = 0;
           ptr < end && col+wordcell < wide;
           col += wordcell)
      {
         printf(format, *ptr);
         ++ptr;
      }
      printf("\n");
   }
}


/**
 * Returns a string that, when submitted to stdin of terminal,
 * will change the foreground color.
 *
 * It is a "cycle" because subsequent calls return different
 * colors, so consecutive texts can be distinguished from each
 * other by color.
 */
const char *cycle_color(void)
{
   static const char *colors[] = {
      "\x1b[31;1m",
      "\x1b[32;1m",
      "\x1b[33;1m",
      "\x1b[34;1m",
      "\x1b[35;1m",
      "\x1b[36;1m",
      "\x1b[37;1m"
   };

   static int counter = 0;
   static int climit = sizeof(colors) / sizeof(colors[0]);

   return colors[ counter++ % climit ];
}







#ifdef TERM_MAIN

const char *wordlist[] = {
   "bag",
   "sack",
   "package",
   "purse",
   "backpack",
   "box",
   "parcel",
   "basket",
   "hamper",
   "gunny sack",
   "pannier",
   "saddlebag"
};

void test_ScrSize_type(void)
{
   ScrSize ssize;
   SS_WIDE(ssize) = 10;
   SS_HIGH(ssize) = 20;

   printf("ssize is %x (%u : %u)\n", ssize, SS_WIDE(ssize), SS_HIGH(ssize));
}

void test_tl_2(void)
{
   puts((isatty(STDIN_FILENO)) ? "stdin IS ATTY" : "stdin IS NOT ATTY");
   puts((isatty(STDOUT_FILENO)) ? "stdout IS ATTY" : "stdout IS NOT ATTY");
   
   int result;
   struct { char code; short vals[5]; } sbuff = {2};
   result = ioctl(STDIN_FILENO, TIOCLINUX, &sbuff);
   printf("Result was %d (%s).\n", result, strerror(errno));
}

void test_tl_8(void)
{
   int result;
   char buffer[220 * 52];
   *buffer = 8;
   result = ioctl(STDIN_FILENO, TIOCLINUX, buffer);
   printf("Result was %d (%s).\n", result, strerror(errno));
}

void test_TIOCLINUX(void)
{
   test_tl_2();
   test_tl_8();
}


int main(int argc, const char **argv)
{
   /* show_words(wordlist, sizeof(wordlist)/sizeof(wordlist[0]), NULL); */

   test_TIOCLINUX();
   return 0;
}


#endif


/* Local Variables: */
/* compile-command: "b=term;  \*/
/*  cc -Wall -Werror -ggdb     \*/
/*  -std=c99 -pedantic         \*/
/*  -D${b^^}_MAIN -o $b ${b}.c \*/
/*  -lreadargs -ldb"           \*/
/* End: */

