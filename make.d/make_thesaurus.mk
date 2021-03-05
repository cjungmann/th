thesaurus.db: files/mthesaur.txt
	./th -Tv


files/mthesaur.txt:
	install -d files
	wget -nc -P files ftp://ftp.ibiblio.org/pub/docs/books/gutenberg/3/2/0/3202/files.zip
	unzip -n files/files.zip
