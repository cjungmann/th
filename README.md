# Berkeley Database Study

The Berkeley Database (*bdb*) seems like an interesting database
product.  Its low-level C-library approach seems similar to the
[FairCom DB](https://www.faircom.com/products/faircom-db) engine
I used back in the late 1990s.

The Berkeley Database is appealing because it is part of Linux
and BSD distributions and has very small footprint.  It rewards
detailed planning of the data, and it is an excuse to explore
some of my C language ideas.

This project is a restart of my [words project](https://www.github.com/cjungmann/words.git),
which is meant to be a command-line thesaurus and dictionary.
That project was my first use of *bdb*, so some of my work there
is a little clumsy.  I want to design the *bdb* code again from
scratch.  I will be copying some of the text parsing code from
the *words* project that will be applicable here.

Using the large datasets that are the thesaurus and dictionary,
I also want to test the performance differences between the
Queue and Recno data access methods.  I expect that Queue will be
faster with the beginning and end of fixed-length records could be
calculated.  Accessing by record number of a given variable-length
record would require a lookup of the file location.  I'd like to
measure the performace diffence to weigh that advantage against
the storage efficiency of variable-length records.

## Thesaurus Source

There are two public-domain sources of thesauri:
- [Moby Thesaurus](https://www.gutenberg.org/ebooks/3202)
- [Roget's Thesaurus](https://www.gutenberg.org/ebooks/10681)

I am using the Moby thesaurus because its organisation is much
simpler and thus easier to parse.  The problem is that the synonyms
are numerous and, lacking organization, much harder to scan when
searching for an appropriate synonym.

## Organizing Synonyms

With hundreds of synonyms for many words, it is very difficult to
scan the list to find an appropriate word.  I will try to impose
some order on the list to make it easier to use.

## Word Frequency

The easiest classification to use is word usage frequency.  I
plan to list the words from the greatest to the least frequency
of use.  Presumably, more popular words may be the best choices,
while less popular words may be obsolete.

There are several sources of word frequencies.  The one I'm
using is based on Google ngrams:

[Natural Language Corpus Data: Beautiful Data](https://norvig.com/ngrams/)

## Dictionary

Grouping synonyms by part of speech (ie noun, verb, adjective, etc.)
has potential to be useful, as well.  The first problem is in identifying
the part of speech represented by each word.  The second problem is in
presentation: it would be better, but harder to program, the have an
interface that has the user choose the part of speech before displaying
the words.

Electronic, public-domain dictionaries

- [GNU Collaborative International Dictionary of English](https://gcide.gnu.org.ua/)
- Gutenberg project [Webster's Unabridged Dictionary](https://www.gutenberg.org/ebooks/29765)

My first attempt is to use the GNU Collaborative International Dictionary
of English (GCIDE).  It is based on an old (1914) version of Webster's,
with some words added by more modern editors.

