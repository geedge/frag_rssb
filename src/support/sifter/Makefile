

frag_index_bin=/home/lishu/frag/frag_index/frag_index/src/lib/
frag_index_inc=/home/lishu/frag/frag_index/frag_index/src/inc/

frag_rssb_bin=/home/lishu/frag/frag_rssb/frag_rssb/src/lib/
frag_rssb_inc=/home/lishu/frag/frag_rssb/frag_rssb/src/inc/

all:
	cd src; $(MAKE);
frag_rssb:
	cp ./bin/*.a $(frag_rssb_bin)
frag_index:
	cp ./bin/*.a $(frag_index_bin)
clean:
	cd src; $(MAKE) clean;

inc:
	cp ./src/sifter.h $(frag_index_inc)
	cp ./src/sifter.h $(frag_rssb_inc)
