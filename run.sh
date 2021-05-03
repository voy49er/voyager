#!/bin/sh

VOYAGER_ROOT=$(dirname $(readlink -f "$0"))
CONFIG=$VOYAGER_ROOT/config/default.cfg

toponame=$(cat $CONFIG | awk -F "=" '$1=="toponame"{print $2;exit;}')

if [ $1 = "setup" ]; then
    if [ -n "$2" ]; then
        toposrc=$2
    else
        toposrc=$toponame
    fi
    cd $VOYAGER_ROOT/src
    if [ -d $VOYAGER_ROOT/data/topo/$toposrc ]; then
        for topofile in `ls -v $VOYAGER_ROOT/data/topo/$toposrc`
        do
            if echo $topofile | grep -q -E "\.topo$"; then
                ./setup -f $toposrc/$topofile -m compact
            fi
        done
    elif [ -f $VOYAGER_ROOT/data/topo/$toposrc ]; then
        ./setup -f $toposrc -m compact
    else
        echo "source does not exist."
    fi
    cd $VOYAGER_ROOT

elif [ $1 = "mininet" -o $1 = "mn" ]; then
    sudo mn -c > /dev/null
    sudo python $VOYAGER_ROOT/scripts/atopo.py $toponame

elif [ $1 = "voyager" -o $1 = "v" ]; then
    sudo ryu-manager $VOYAGER_ROOT/voyager.py
fi
