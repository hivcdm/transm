ifndef config
  config=debug
endif
export config

PROJECTS := transm

.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

transm: 
	@echo "==== Building transm ($(config)) ===="
	@${MAKE} --no-print-directory -C . -f transm.make

clean:
	@${MAKE} --no-print-directory -C . -f transm.make clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   transm"
	@echo ""
	@echo "For more information, see http://industriousone.com/premake/quick-start"
