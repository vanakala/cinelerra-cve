include ../../global_config

OBJS = $(OBJDIR)/suv.o

OUTPUT_THEME = ../$(OBJDIR)/suv.plugin

$(OUTPUT_THEME): $(OBJS)
	$(LDLINKER) -o $(OUTPUT_THEME) $(OBJS)
	strip $(OUTPUT_THEME)
	../../guicast/$(OBJDIR)/bootstrap $(OUTPUT_THEME) data/*.png



include ../../plugin_config

$(OBJDIR)/suv.o: suv.C

inst:
	rm -f /usr/lib/cinelerra/suv.plugin
	cp $(OUTPUT_THEME) /usr/lib/cinelerra
