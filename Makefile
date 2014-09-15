SUBDIRS = lib parser switch tcpinterface

.PHONY: subdirs $(SUBDIRS)

%:
	$(MAKE) TARGET=$@ subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(TARGET)

tcpinterface: lib parser
switch: lib
