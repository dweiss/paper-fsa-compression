#!/bin/bash

# compute data for the chart: automaton size/moved states

SOFTWARE=software

java -server -Xmx1024m -jar $SOFTWARE/morfologik-allsteps/morfologik*.jar \
     fsa_build \
       --format cfsa2 \
       --progress \
       --sorted -i data-sets/weiss/pl.dict \
       -o tmp.fsa 2>&1 | tee size-moved.stdout

rm tmp.fsa
