all:
	cd src; $(MAKE);
	cd test; $(MAKE);
	cd test_readfile; $(MAKE);
	cd test_md5;$(MAKE);
	
clean:
	cd src; $(MAKE) clean;
	cd test; $(MAKE) clean;
	cd test_readfile; $(MAKE) clean;
	cd test_md5;$(MAKE) clean;
	rm -f lib/usm_comm*;
