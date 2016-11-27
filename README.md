VTPin
-----

Build
=====

    make -B vtpin

to only build the vtpin library or

    make -B all

to also build the test and complementary files.
Also you can use

    make -B debug

for debugging

Running
=======

Run a basic test with

    ./run.sh -r -p -c -l ./bin/libvtpin.so -e "./bin/vtpin_test"

or e.g.

    ./run.sh -r -x -p -l ./bin/libvtpin.so -e "./bin/inheritance_test"


Do `./run.sh -h` for more options. E.g. with -d you can directly run gdb with the command and arguments preloaded into it.
