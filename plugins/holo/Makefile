include ../../global_config

OBJS = $(OBJDIR)/holo.o  \
	$(OBJDIR)/holowindow.o \
	../burn/$(OBJDIR)/effecttv.o

OUTPUT = ../$(OBJDIR)/holo.plugin

CFLAGS += -I../burn 

include ../../plugin_config

$(OBJDIR)/holo.o: holo.C
$(OBJDIR)/holowindow.o: holowindow.C
