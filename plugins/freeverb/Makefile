include ../../global_config

OBJS := $(OBJDIR)/freeverb.o


FREEVERBOBJS := \
	$(OBJDIR)/Components/allpass.o \
	$(OBJDIR)/Components/comb.o \
	$(OBJDIR)/Components/revmodel.o	

OUTPUT := ../$(OBJDIR)/freeverb.plugin

CFLAGS += -fPIC \
	-I../ \
	-I../../cinelerra \
	-I../../guicast \
	-I../../quicktime \
	-I../colors \
	-IComponents

DIRS := $(OBJDIR)/Components

$(shell echo $(CFLAGS) > ../$(OBJDIR)/c_flags)
$(shell echo $(OBJS) $(FREEVERBOBJS) > $(OBJDIR)/objs)



$(OUTPUT): $(DIRS) $(OBJS) $(FREEVERBOBJS)
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

$(FREEVERBOBJS):
	$(CC) -c `cat ../$(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.cpp) -o $*.o




$(OBJDIR)/freeverb.o: freeverb.C
$(OBJDIR)/Components/allpass.o:  Components/allpass.cpp
$(OBJDIR)/Components/comb.o: 	 Components/comb.cpp
$(OBJDIR)/Components/revmodel.o: Components/revmodel.cpp








