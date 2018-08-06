# Example makefile which makes a "libccan.a" of everything under ccan/.
# For simple projects you could just do:

SRCFILES += $(wildcard ccan/*/*.c)

CCAN_CFLAGS=-g -O2 -Wall -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wundef -DCCAN_STR_DEBUG=1 -mbmi -msse
#CCAN_CFLAGS=-g3 -ggdb -Wall -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wundef -DCCAN_STR_DEBUG=1 -mbmi -msse
CFLAGS = $(CCAN_CFLAGS) -I. $(DEPGEN)
CFLAGS_FORCE_C_SOURCE = -x c

MODS := a_star \
	aga \
	agar \
	alignof \
	altstack \
	antithread \
	antithread/alloc \
	argcheck \
	array_size \
	asearch \
	asort \
	asprintf \
	autodata \
	avl \
	base64 \
	bdelta \
	bitmap \
	block_pool \
	breakpoint \
	btree \
	build_assert \
	bytestring \
	cast \
	ccan_tokenizer \
	cdump \
	charset \
	check_type \
	ciniparser \
	compiler \
	container_of \
	cppmagic \
	cpuid \
	crc \
	crcsync \
	crypto/ripemd160 \
	crypto/sha256 \
	crypto/sha512 \
	crypto/shachain \
	crypto/siphash24 \
	daemonize \
	daemon_with_notify \
	darray \
	deque \
	dgraph \
	endian \
	eratosthenes \
	err \
	failtest \
	foreach \
	generator \
	grab_file \
	hash \
	heap \
	htable \
	idtree \
	ilog \
	invbloom \
	io \
	isaac \
	iscsi \
	jacobson_karels \
	jmap \
	jset \
	json \
	lbalance \
	likely \
	list \
	lpq \
	lqueue \
	lstack \
	md4 \
	mem \
	minmax \
	net \
	nfs \
	noerr \
	ntdb \
	objset \
	opt \
	order \
	permutation \
	pipecmd \
	pr_log \
	ptrint \
	ptr_valid \
	pushpull \
	rbtree \
	rbuf \
	read_write_all \
	rfc822 \
	rszshm \
	short_types \
	siphash \
	sparse_bsearch \
	str \
	str/hex \
	strgrp \
	stringbuilder \
	stringmap \
	strmap \
	strset \
	structeq \
	take \
	tal \
	tal/grab_file \
	tal/link \
	tal/path \
	tal/stack \
	tal/str \
	tal/talloc \
	talloc \
	tally \
	tap \
	tcon \
	time \
	timer \
	tlist \
	tlist2 \
	ttxml \
	typesafe_cb \
	version \
	xstring

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

-include *.d
main: $(MYOBJFILES) main.o libccan.a 
	$(CC) -o $@ $^

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
