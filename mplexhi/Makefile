include ../global_config

CC = gcc
CFLAGS += -DHAVE_CONFIG_H -I.
OBJS = \
	$(OBJDIR)/bits.o \
	$(OBJDIR)/buffer.o \
	$(OBJDIR)/inits.o \
	$(OBJDIR)/inptstrm.o \
	$(OBJDIR)/interact.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/multplex.o \
	$(OBJDIR)/systems.o \
	$(OBJDIR)/vector.o

OUTPUT = $(OBJDIR)/mplexhi

$(OUTPUT):  $(OBJS)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJS)

$(OBJS):
	$(CC) -c $(CFLAGS) $(subst $(OBJDIR)/,, $*.c) -o $*.o

clean:
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;
	rm -rf $(OUTPUT)

install:
	cp $(OUTPUT) /usr/bin

$(OBJDIR)/bits.o: bits.c
$(OBJDIR)/buffer.o: buffer.c
$(OBJDIR)/inits.o: inits.c
$(OBJDIR)/inptstrm.o: inptstrm.c
$(OBJDIR)/interact.o: interact.c
$(OBJDIR)/main.o: main.c
$(OBJDIR)/multplex.o: multplex.c
$(OBJDIR)/systems.o: systems.c
$(OBJDIR)/vector.o: vector.c
