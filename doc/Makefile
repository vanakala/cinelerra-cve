IMAGES = \
	arrow.png \
	autokeyframe.png \
	camera.png \
	channel.png \
	crop.png \
	cut.png \
	expandpatch_checked.png \
	ibeam.png \
	left_justify.png \
	mask.png \
	mutepatch_up.png \
	paste.png \
	projector.png \
	record.png \
	recordpatch_up.png \
	rewind.png \
	singleframe.png \
	titlesafe.png \
	toolwindow.png \
	top_justify.png \
	wrench.png \
	magnify.png

$(OUTPUT):
	cinelerra.html

all:
	for i in $(IMAGES) ; \
		do if ! test -e $$i ; \
			then cp -a ../plugins/defaulttheme/data/$$i . ; \
		fi ; \
	done
	makeinfo --html --no-split cinelerra.texi

install:
	cp $(IMAGES) /amazon/httpd/html/cinelerra
	cp cinelerra.html /amazon/httpd/html/cinelerra

clean:
	rm -f $(IMAGES) $(OUTPUT)
