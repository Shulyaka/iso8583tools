SUBDIRS = $(shell find . -maxdepth 2 -mindepth 2 -name Makefile -type f -printf "%h\n")

.PHONY: subdirs $(SUBDIRS)

%:
	$(MAKE) TARGET=$@ subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(TARGET)

tcpinterface: lib parser
switch issuer saf: lib
