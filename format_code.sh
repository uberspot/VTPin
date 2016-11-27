#!/bin/sh

# This script reformats source files using the clang-format utility.
# Set the list of source directories on the "for" line below.
#
# The file .clang-format in this directory specifies the formatting parameters.
#
# Files are changed in-place, so make sure you don't have anything open in an
# editor, and you may want to commit before formatting.

echo "Formatting code under current directory"
find . \( -name '*.h' -or -name '*.cxx' -or -name '*.c'  \) -type f -print0 | xargs -0 clang-format -style=file -i