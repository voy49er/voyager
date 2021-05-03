#!/bin/sh

VOYAGER_ROOT=$(dirname $(readlink -f "$0"))

if [ ! $# -eq 3 ]; then
    echo "usage: ./split.sh <tssfile> <start_threshold> <end_threshold>"
    exit
fi

origin=$VOYAGER_ROOT/data/tss/$1
if [ ! -f $origin ]; then
    echo "tss file does not exist."
    exit
fi

if [ ! $2 -gt 0 -o ! $3 -gt 0 ]; then
    echo "threshold must be positive."
    exit
fi

for p in `seq $2 $3`;
do
    split=$(echo $origin | sed -e 's/.tss$/.split'$p'.tss/')
    
    pattern=$(printf '%'$p's' | sed -e 's/ /[0-9]\\+ /g' | sed -e 's/ $//')

    lineno=$(expr $(head -1 $origin) + 2)
    cat $origin | sed -e $lineno',${s/\('"$pattern"'\) /\1\n/g}' > $split

    len=$(expr $(wc -l < $split) - $lineno)
    cat $split | awk '{if(NR=='$lineno')print '$len'; else print $0}' > tmp
    mv tmp $split

    echo "write "$(echo $split | grep -o "/data/tss/.*")
done
