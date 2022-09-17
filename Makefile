export TARGET := five_clique
export BUILD := build
export BINDIR := $(BUILD)/bin

# debug is the default build
export OBJDIR := $(BUILD)/objects/debug
export CXXTARGET := $(BINDIR)/$(TARGET)_debug

.PHONY: all bear debug release format run clean $(GIT_SUBMODULES)

# passes C++ building to another makefile to assist in separate release and
# debug builds
all run valgrind debug release format $(TARGET):
	@$(MAKE) $@ --no-print-directory -j -f makefiles/main.mk

# runs `bear -- make` and then compdb
bear: clean
	bear -- $(MAKE)
	compdb list > compile_commands2.json
	mv compile_commands2.json compile_commands.json

# update git submodules
GIT=git
GIT_SUBMODULES = $(shell [ -d '.gitmodules' ] && sed -nE 's/path = +(.+)/\1\/.git/ p' .gitmodules | paste -s -)
$(GIT_SUBMODULES): %/.git: .gitmodules
	$(GIT) submodule update --init $*
	@touch $@

# change the executale target and object build location on release build
release: export OBJDIR := $(BUILD)/objects/release
release: export CXXTARGET := $(BINDIR)/$(TARGET)_release

valgrind: export OBJDIR := $(BUILD)/objects/valgrind
valgrind: export CXXTARGET := $(BINDIR)/$(TARGET)_valgrind

clean:
	rm -rvf $(BUILD)
	rm -vf $(TARGET)
