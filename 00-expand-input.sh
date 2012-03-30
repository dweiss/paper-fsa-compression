#!/bin/bash

# expand the input automata from FSA representation to plain text.

find data-sets -name *.fsa -printf 'java -jar software/morfologik/morfologik-stemming-nodict-trunk.jar fsa_dump -r -d %p > `dirname %p`/`basename %p .fsa`.dict\n' > .tmp
bash .tmp
rm .tmp