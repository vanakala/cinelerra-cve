#CFLAGS += -g

include ../../global_config

OBJS = $(OBJDIR)/burn.o  \
	$(OBJDIR)/burnwindow.o \
	$(OBJDIR)/effecttv.o

OUTPUT = ../$(OBJDIR)/burn.plugin

include ../../plugin_config

$(OBJDIR)/burn.o: burn.C
$(OBJDIR)/burnwindow.o: burnwindow.C
$(OBJDIR)/effecttv.o: effecttv.C
