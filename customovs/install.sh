#!/bin/sh

set -x

OVS_VERSION="2.15.0"

CUSTOMOVS_ROOT=$(dirname $(readlink -f "$0"))
OVS_ROOT=$CUSTOMOVS_ROOT/openvswitch-$OVS_VERSION

# download
if [ ! -d $OVS_ROOT ]; then
    wget -P $CUSTOMOVS_ROOT https://www.openvswitch.org/releases/openvswitch-$OVS_VERSION.tar.gz > /dev/null 2>&1
    tar -C $CUSTOMOVS_ROOT -xvf $CUSTOMOVS_ROOT/openvswitch-$OVS_VERSION.tar.gz > /dev/null 2>&1
    rm $CUSTOMOVS_ROOT/openvswitch-$OVS_VERSION.tar.gz
fi

# overwrite with custom files
cp $CUSTOMOVS_ROOT/flow.h $OVS_ROOT/include/openvswitch/flow.h
cp $CUSTOMOVS_ROOT/meta-flow.h $OVS_ROOT/include/openvswitch/meta-flow.h
cp $CUSTOMOVS_ROOT/meta-flow.c $OVS_ROOT/lib/meta-flow.c
cp $CUSTOMOVS_ROOT/flow.c $OVS_ROOT/lib/flow.c
cp $CUSTOMOVS_ROOT/nx-match.c $OVS_ROOT/lib/nx-match.c
cp $CUSTOMOVS_ROOT/match.c $OVS_ROOT/lib/match.c
cp $CUSTOMOVS_ROOT/meta-flow.xml $OVS_ROOT/lib/meta-flow.xml

# build
cd $OVS_ROOT
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --with-linux=/lib/modules/$(uname -r)/build > /dev/null 2>&1
make -j8 > /dev/null 2>&1
sudo make install > /dev/null 2>&1
sudo make modules_install > /dev/null 2>&1
cd $CUSTOMOVS_ROOT

# override kernel modules
CFG=/etc/depmod.d/openvswitch.conf
true | sudo tee $CFG
for module in $OVS_ROOT/datapath/linux/*.ko; do
    modname="$(basename ${module})"
    echo "override ${modname%.ko} * extra" | sudo tee -a $CFG > /dev/null 2>&1
    echo "override ${modname%.ko} * weak-updates" | sudo tee -a $CFG > /dev/null 2>&1
done

sudo depmod -a
modprobe openvswitch

# start & show
sudo /usr/share/openvswitch/scripts/ovs-ctl restart
sudo ovs-vsctl show
