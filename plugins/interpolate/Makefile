include ../../global_config

OBJS := $(OBJDIR)/interpolate.o


OUTPUT := ../$(OBJDIR)/interpolate.plugin

CFLAGS += -I../ -I../../cinelerra \
	-I../../guicast \
	-I../../quicktime

$(shell echo $(CFLAGS) > ../$(OBJDIR)/c_flags)
$(shell echo $(OBJS) > $(OBJDIR)/objs)



$(OUTPUT): $(DIRS) $(OBJS)
	$(LDLINKER) -o $(OUTPUT) `cat $(OBJDIR)/objs`

$(DIRS):
	if ! test -d $@ ; then mkdir -p $@ ; fi

clean:
	rm -rf $(OBJDIR)
	rm -f $(OUTPUT)

install:
	cp $(OUTPUT) $(PLUGINDIR)

$(OBJS):
	$(CC) -c `cat ../$(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o



$(OBJDIR)/interpolate.o: interpolate.C












