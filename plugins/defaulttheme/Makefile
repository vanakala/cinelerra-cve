include ../../global_config

OBJS = $(OBJDIR)/defaulttheme.o

OUTPUT_THEME = ../$(OBJDIR)/defaulttheme.plugin

$(OUTPUT_THEME): $(OBJS)
	$(LDLINKER) -o $(OUTPUT_THEME) $(OBJS)
	strip $(OUTPUT_THEME)
	../../guicast/$(OBJDIR)/bootstrap $(OUTPUT_THEME) data/*.png



include ../../plugin_config

$(OBJDIR)/defaulttheme.o: defaulttheme.C

inst:
	rm -f /usr/lib/cinelerra/defaulttheme.plugin
	cp $(OUTPUT_THEME) /usr/lib/cinelerra
