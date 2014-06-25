include ../../rules.mk

ELA_MAN = explain_syslog.8.gz syslog_to_svclog.8.gz

all: $(ELA_MAN)

%.8.gz: %.8
	gzip -c $< > $@

install: all
	@$(call install_man,$(ELA_MAN),$(DESTDIR))

uninstall:
	@$(call uninstall_man,$(ELA_MAN), $(DESTDIR))

clean:
	rm -f *.gz
