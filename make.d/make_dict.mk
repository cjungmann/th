# I'm not doing this anymore, but I'm keeping the scripts and rules
# around in case I want to come back to this.
dict.db: files/dict.xml
	@echo Making dict.db
	touch dict.db

files/dict.xml : files/gcide/CIDE.A
	@echo Creating files/dict.xml
	scripts/gcide_preprocess_to_dict_xml files/gcide files/dict.xml

files/gcide/CIDE.A:
	install -d files
	@echo "Downloading GCide dictionary from gnu.org"
	wget -nc "ftp://ftp.gnu.org/gnu/gcide/gcide-0.52.tar.xz"
	unxz gcide-0.52.tar.xz
	tar -xf gcide-0.52.tar
	mv gcide-0.52 files/gcide


