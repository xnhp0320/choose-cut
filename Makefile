# Example makefile which makes a "libccan.a" of everything under ccan/.
# For simple projects you could just do:

SRCFILES += $(wildcard ccan/*/*.c)

CCAN_CFLAGS=-g -O2 -Wall -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wundef -DCCAN_STR_DEBUG=1 -mbmi -msse4.2
#CCAN_CFLAGS=-g3 -ggdb -Wall -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wundef -DCCAN_STR_DEBUG=1 -mbmi -msse
CFLAGS = $(CCAN_CFLAGS) -I. $(DEPGEN)
CFLAGS_FORCE_C_SOURCE = -x c

MODS :=	darray opt heap 
# Anything with C files needs building; dir leaves / on, sort uniquifies
MODS_WITH_SRC = $(patsubst ccan/%/, %, $(sort $(foreach m, $(MODS), $(dir $(wildcard ccan/$m/*.c)))))

default: main

# Automatic dependency generation: makes ccan/*/*.d files.
DEPGEN=-MMD
-include $(foreach m, $(MODS), ccan/$(m)/*.d)

DIRS=$(patsubst %, ccan/%, $(filter-out $(MODS_EXCLUDE), $(MODS_WITH_SRC)))

# Generate everyone's separate Makefiles.
-include $(foreach dir, $(DIRS), $(dir)-Makefile)

ccan/%-Makefile:
	@echo $@: $(wildcard ccan/$*/*.[ch]) ccan/$*/_info > $@
	@echo ccan/$*.o: $(patsubst %.c, %.o, $(wildcard ccan/$*/*.c)) >> $@

# We compile all the ccan/foo/*.o files together into ccan/foo.o
OBJFILES=$(DIRS:=.o)


# We create all the .o files and link them together.
$(OBJFILES): %.o:
	$(LD) -r -o $@ $^

libccan.a: $(OBJFILES)
	$(AR) r $@ $(OBJFILES)

MYTESTFILES=test.c main.c
MYSRCFILES=$(filter-out $(MYTESTFILES), $(wildcard *.c))
MYOBJFILES=$(patsubst %.c, %.o, $(MYSRCFILES))

JEMALLOC= -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`
-include *.d
main: $(MYOBJFILES) main.o libccan.a 
	$(CC) -o $@ $^ $(JEMALLOC)

clean:
	rm -rf libccan.a
	rm -rf *.o
	rm -rf *.d
	rm -rf main
	rm -rf test

libclean:
	rm -rf $(OBJFILES)
	rm -rf ccan/*-Makefile
	rm -rf $(foreach m, $(MODS), ccan/$(m)/.d)
	rm -rf $(foreach m, $(MODS), ccan/$(m)/*.o)


test: $(MYOBJFILES) test.o libccan.a
	$(CC) -o $@ $^
