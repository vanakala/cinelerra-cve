#/bin/sh

################################################################################
#                            make_wiki.sh                                      #
#                 Converts texinfo -> html -> wiki pages                       #
#          Copyright (C) 2006 2007 Nicolas MAUFRAIS - e.conti@gmx.net          #
################################ Licence: ######################################
# You may redistribute this script and/or modify it under the terms of the GNU #
# General Public License, as published by the Free Software Foundation; either #
# version 2 of the License, or (at your option) any later version.             #
################################################################################

# We determine the languages the manual is available in
for available_language in `ls | grep "^cinelerra_cv_manual_..\.texi$" | sed "s+.*_++g" | sed "s+\..*++g"`;
do
	available_languages_list="${available_language} ${available_languages_list}"
done

# Error flag init
error=0
# If one argument is passed
if [ $# -eq 1 ]
then
	language=$1
	# If there is not texinfo cinelerra CV manual in that language
	if [ ! -f cinelerra_cv_manual_${language}.texi ]
	then
		echo "ERROR: the manual is not available in the language code you specified: ${language}"
		error=1
	fi
else
	# The user did not specify a language code
	echo "ERROR: you have to specify the language code"
	error=1
fi

# There's an error. We print the syntax and exit
if [ ${error} -eq 1 ]
then
	# We determine the shell name
	shell_name=`basename $0`
	# We print the shell syntax
	echo "Syntax: ${shell_name} <two-letter ISO-639 language code>"
	# We print the available languages codes
	echo "Available languages codes: ${available_languages_list}"
	exit 1
fi

# First of all, we delete the (old) directory containing the wiki pages and
# recreate it
rm -fr cinelerra_cv_${language}
rm -fr cinelerra_cv_${language}.tgz
mkdir cinelerra_cv_${language}

# We generate html pages from the texinfo file.
# Those html page will be converted to the wiki format later.
texi2html --split chapter --nosec-nav --no-menu -prefix cinelerra_cv_${language} cinelerra_cv_manual_${language}.texi

# We go in the directory containing the wiki pages
cd cinelerra_cv_${language}
# For each html page
for html_page in `/bin/ls | grep "\.html$"`;
do
	echo "Converting ${html_page} to cinelerra_cv_${language}/${final_doku_page}"
	# We define the name of the files we will create
	original_doku_page="`basename ${html_page} .html`.doku.orig"
	temp_doku_page="`basename ${html_page} .html`.doku.temp"
	final_doku_page="`basename ${html_page} .html`.txt"
	# We convert the html page to the wiki format
	html2wiki --dialect DokuWiki ${html_page} > ${original_doku_page}
	# Counting the number of lines in the original wiki page
	nb_lines_original_doku_page=`cat ${original_doku_page} | wc -l`
	# We remove some lines at the start and beginning of the wiki page
        cat ${original_doku_page} | head -n `expr ${nb_lines_original_doku_page} - 6` | tail -n `expr ${nb_lines_original_doku_page} - 10` > ${temp_doku_page}
	# We put in the URL of the images.
	cat ${temp_doku_page} | sed "+s+{{\.\.\/\.\/manual_images_\([^/]*\)\/+{{http:\/\/cvs.cinelerra.org\/docs\/manual_images_\1\/+g" | grep -v "^| \[\[#SEC" | sed "+s+\[\[cinelerra_cv_${language}_\([0-9]*\)\.html.SEC[0-9]*.\([^\]]*\)+\[\[english_manual:cinelerra_cv_en_\1\\#\2+g" > ${original_doku_page}
	# Some characters are not displayed properly in the wiki page. We have to replace them
	cat ${original_doku_page} | sed "+s+&gt;+>+g" | sed "+s+\&amp;+\&+g" | sed "+s+&lt;+<+g" > ${final_doku_page}
	# We remove the temporary files
	rm ${original_doku_page} ${temp_doku_page} ${html_page}
done

rm cinelerra_cv_${language}_25.txt
rm cinelerra_cv_${language}_abt.txt
rm cinelerra_cv_${language}_fot.txt
rm cinelerra_cv_${language}.txt
rm cinelerra_cv_${language}_toc.txt

cd ..
