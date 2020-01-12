# Very basic smoke test for decompressor
test:
	$(MAKE) -C ../src
	$(MAKE) -C ../examples/tgunzip
	$(MAKE) -C ../examples/tgzip
	../examples/tgunzip/tgunzip corpus.tar.gz corpus-out.tar
	md5sum -c corpus.md5sum

clean:
	$(MAKE) -C ../src $@
	$(MAKE) -C ../examples/tgzip $@
	$(MAKE) -C ../examples/tgunzip $@
