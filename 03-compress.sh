#!/bin/bash

SOFTWARE=software

TIME_CMD=/usr/bin/time
TIME_FMT="wall\t%e\t%E\tuser\t%U\tsys\t%S\tmax-mem-kb\t%M\tdta\t%D\tinputs\t%I\toutputs\t%O"

function morfologik_nx {
  $TIME_CMD -f $TIME_FMT -o $2.log \
  java -server -Xmx1024m -jar $SOFTWARE/cfsa2.nx/morfologik*.jar \
    fsa_build \
       --format cfsa2 \
       --progress \
       --sorted -i $1 \
       -o $2 >$2.stdout 2>&1
}

function morfologik_cfsa2 {
  $TIME_CMD -f $TIME_FMT -o $2.log \
  java -server -Xmx1024m -jar $SOFTWARE/morfologik/morfologik*.jar \
    fsa_build \
       --format cfsa2 \
       --progress \
       --sorted -i $1 \
       -o $2 >$2.stdout 2>&1
}

function morfologik_fsa5 {
  $TIME_CMD -f $TIME_FMT -o $2.log \
  java -server -Xmx1024m -jar $SOFTWARE/morfologik/morfologik*.jar \
    fsa_build \
       --format fsa5 \
       --progress \
       --sorted -i $1 \
       -o $2 >$2.stdout 2>&1
}

function fsa_build {
  $TIME_CMD -f $TIME_FMT -o $2.log \
  $SOFTWARE/fsa/fsa_build \
       -O \
       -i $1 \
       -o $2 >$2.stdout 2>&1
}

TOOLS="
 morfologik_nx 
 morfologik_cfsa2 
 morfologik_fsa5 
 fsa_build"

DATASETS="
data-sets/ciura-deorowicz/random.dict
data-sets/ciura-deorowicz/files.dict
data-sets/ciura-deorowicz/unix.dict
data-sets/ciura-deorowicz/ifiles.dict
data-sets/ciura-deorowicz/scrable.dict
data-sets/ciura-deorowicz/full.dict
data-sets/ciura-deorowicz/unix_m.dict
data-sets/ciura-deorowicz/deutsch.dict
data-sets/ciura-deorowicz/dimacs.dict
data-sets/ciura-deorowicz/webster.dict
data-sets/ciura-deorowicz/test.dict
data-sets/ciura-deorowicz/fr.dict
data-sets/ciura-deorowicz/eo.dict
data-sets/ciura-deorowicz/sample.dict
data-sets/ciura-deorowicz/esp.dict
data-sets/ciura-deorowicz/polish.dict
data-sets/ciura-deorowicz/english.dict
data-sets/ciura-deorowicz/enable.dict
data-sets/ciura-deorowicz/russian.dict
data-sets/ciura-deorowicz/one.dict
data-sets/weiss/streets.dict
data-sets/weiss/wikipedia.dict
data-sets/weiss/streets2.dict
data-sets/weiss/pl.dict
data-sets/weiss/wikipedia2.dict
"

RESULTS=results
TIMESTAMP=`date +%Y%m%d%H%M%S`

for dataset in $DATASETS; do
  for tool in $TOOLS; do
    datasetfile=`basename $dataset .dict`
    echo "$tool, $dataset..."
    mkdir -p $RESULTS/$TIMESTAMP/$datasetfile
    $tool $dataset $RESULTS/$TIMESTAMP/$datasetfile/$datasetfile.$tool

    OUTPUT_LENGTH=`wc --bytes $RESULTS/$TIMESTAMP/$datasetfile/$datasetfile.$tool | cut -d' ' -f 1`
    WALL_TIME=`cat $RESULTS/$TIMESTAMP/$datasetfile/$datasetfile.$tool.log`
    echo -e "$dataset\t$tool\t$OUTPUT_LENGTH\t$WALL_TIME" >> $RESULTS/$TIMESTAMP/results.log
  done
done
