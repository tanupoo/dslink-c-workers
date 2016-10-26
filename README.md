Sample code to integrate a heavy work thread into the DSLink loop.
==================================================================

## Preparation

before you build this tool, you have to make the DSLink SDK C.
See README.md for detail.

Briefly, you just clone the SDK, and type ./tools/build.sh.

    % git clone http://github.com/IOT-DSA/sdk-dslink-c.git
    % cd sdk-dslink-c
    % ./tools/build.sh

## how to build

To get the source code of this tool, you just clone it like below.

    % git clone http://github.com/tanupoo/dslink-c-workers.git
    % cd dslink-c-workers

You also have to set the full path to the DSLink SDK directory
into DSLINK_SDK_DIR.

    e.g.
    % export DSLINK_SDK_DIR=/opt/dsa/sdk-dslink-c

Then, make it.

    % make

## how to run

don't forget to set LD_LIBRARY_PATH like below.

    e.g.
    % export LD_LIBRARY_PATH=$DSLINK_SDK_DIR/build

