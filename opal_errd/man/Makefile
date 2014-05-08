include ../../rules.mk


OPAL_ERRD_MAN = opal-elog-parse.8.gz opal_errd.8.gz

all: $(OPAL_ERRD_MAN)

%.8.gz: %.8
	gzip -c $< > $@

install: all
	@$(call install_man,$(OPAL_ERRD_MAN),$(DESTDIR))

uninstall:
	@$(call uninstall_man,$(OPAL_ERRD_MAN), $(DESTDIR))

clean:
	rm -f *.gz
