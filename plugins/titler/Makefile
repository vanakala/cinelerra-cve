include ../../global_config

OBJS = $(OBJDIR)/title.o  \
	$(OBJDIR)/titlewindow.o \
	../colors/$(OBJDIR)/colorpicker.o \
	../colors/$(OBJDIR)/plugincolors.o



OUTPUT_PLUGIN = ../$(OBJDIR)/titler.plugin
FREETYPE_DIR = ../../freetype-2.1.4
CFLAGS += -I$(FREETYPE_DIR)/include -I../../


$(OUTPUT_PLUGIN): $(OBJS)
	$(LDLINKER) -o $(OUTPUT_PLUGIN) \
		$(OBJS) \
		$(FREETYPE_DIR)/$(OBJDIR)/libfreetype.a
	strip $(OUTPUT_PLUGIN)

include ../../plugin_config

inst:
	cp $(OUTPUT_PLUGIN) /usr/lib/cinelerra
	cp -a fonts /usr/lib/cinelerra





$(OBJDIR)/title.o: title.C
$(OBJDIR)/titlewindow.o: titlewindow.C



