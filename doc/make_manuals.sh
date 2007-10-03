#/bin/sh

################################################################################
#                            make_manuals.sh                                   #
#             Builds manuals from the texinfo Cinelerra CV file                #
#         Copyright (C) 2006 2007 Nicolas MAUFRAIS - e.conti@gmx.net           #
################################ Licence: ######################################
# You may redistribute this script and/or modify it under the terms of the GNU #
# General Public License, as published by the Free Software Foundation; either #
# version 2 of the License, or (at your option) any later version.             #
################################################################################

# We determine the languages the manual is available in
for available_language in `ls | grep "^cinelerra_cv_manual_.*\.texi$" | sed "s+cinelerra_cv_manual_++g" | sed "s+\.texi++g"`;
do
	available_languages_list="${available_language} ${available_languages_list}"
done

# Error flag init
error=0
# If one argument is passed
if [ $# -eq 1 ]
then
	language=$1
	# If there is no texinfo cinelerra CV manual in that language
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
	echo "Syntax: ${shell_name} <language code>"
	# We print the available languages codes
	echo "Available languages codes: ${available_languages_list}"
	exit 1
fi

echo "------- Making PDF manual (${language}) -------"
rm -rf cinelerra_cv_manual_${language}.{aux,cp,cps,fn,ky,log,pg,toc,tp,vr}
texi2pdf -q --pdf cinelerra_cv_manual_${language}.texi
rm -rf cinelerra_cv_manual_${language}.{aux,cp,cps,fn,ky,log,pg,toc,tp,vr}

echo "------- Making HTML manual (${language}) -------"
texi2html --nosec-nav cinelerra_cv_manual_${language}.texi
# We do not use the following command in this script. However, we use it to
# generate the html manual with splited pages which is on the Cinelerra CV
# website :
texi2html --nosec-nav --no-menu --split chapter cinelerra_cv_manual_${language}.texi
echo "------- Making TXT manual (${language}) -------"
rm -rf manual_images_${language}/*.txt
for image in `/bin/ls manual_images_${language} | grep "\.png$"`;
do
	text_file="manual_images_${language}/`basename ${image} .png`.txt"
	echo "Image: $image" > "manual_images_${language}/`basename ${image} .png`.txt" > ${text_file}
done
makeinfo --plaintext --no-headers cinelerra_cv_manual_${language}.texi -o cinelerra_cv_manual_${language}.txt
rm -rf manual_images_${language}/*.txt

# We can generate the docbook manual with makeinfo CVS only. If you installed
# the makeinfo CVS binary, change the makeinfo_cvs variable contents accordingly.
makeinfo_cvs=/usr/local/bin/makeinfo_cvs
if [ -x ${makeinfo_cvs} ]
then
	echo "------- Making DocBook manual (${language}) -------"
	${makeinfo_cvs} --docbook --no-number-sections --no-warn cinelerra_cv_manual_${language}.texi -o cinelerra_cv_manual_${language}.xml
	First_line_to_delete=`cat cinelerra_cv_manual_${language}.xml | grep -n "<chapter label=\"\" xreflabel=\"Cinelerra CV Manual\" id=\"Top\">" | sed "+s+:.*++g"`
	if [ -n "${First_line_to_delete}" ]
	then
	      Last_line_to_delete=`expr ${First_line_to_delete} + 3`
	      cat cinelerra_cv_manual_${language}.xml | sed "${First_line_to_delete},${Last_line_to_delete}d" > cinelerra_cv_manual_${language}_temp.xml
	      mv cinelerra_cv_manual_${language}_temp.xml cinelerra_cv_manual_${language}.xml
	fi
fi
