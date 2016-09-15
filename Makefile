
SUBDIRS := lib sample

.PHONY: all $(SUBDIRS)

All: subdirs doc

subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done

doc:
	doxygen

clean:
	rm -r doxygen_sqlite3.db html latex
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean ; done
