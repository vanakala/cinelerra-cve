include ../global_config

OBJS = $(OBJDIR)/mplex.o
CC = gcc
CFLAGS += -I../libmpeg3

OUTPUT := $(OBJDIR)/mplexlo

$(OUTPUT): $(OBJS)
	$(CC) -o $(OUTPUT) $(OBJS) ../libmpeg3/$(OBJDIR)/libmpeg3.a -lpthread -lm

$(OBJS):
	$(CC) -c $(CFLAGS) $(subst $(OBJDIR)/,, $*.c) -o $*.o

clean:
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;
	rm -rf $(OBJDIR)

install:
	cp $(OUTPUT) /usr/bin

$(OBJDIR)/mplex.o: mplex.c
