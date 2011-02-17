# This script converts output of mmorph to 3 column HT-separated format.
# mmorph output has the inflected form (first column), and the base form
# (third column) in double quotes, and the second column is an equal sign.
# Spaces after the left square bracket and before the right square bracket
# are removed as well.
# The inflected form, and the base form may contain spaces!
# Author: Jan Daciuk
{
  sub("[^\"]*\"","",$0);	# delete first double quote
  # Now the first double quote is the one ending the inflected form
  sub("\"[^\"]*\"","\t",$0);	# convert the ending double quote of inflected
				# form, the equal sign, the beginning double
				# quote of the base form, and all white space
				# between them to a HT character
  sub("\" *","\t",$0);		# convert the ending double quote of base form
				# and the following spaces to HT
  gsub("\[ ", "[", $0);		# delete space after [
  gsub(" \]", "]", $0);		# delete space before ]
  gsub("_"," ",$0);		# mmorph uses _ to indicate spaces between 
				# words; I use it for various things, but
				# I can use spaces
  print;
}
