#!/usr/bin/env python2.7

import re

trace_file = open("/tmp/trace_output.txt", "r")

blocks = {}
sum_of_malloc = 0
sum_of_free = 0
sum_of_realloced_full = 0
sum_of_realloced_pointers = 0

current_malloc_sum = 0
max_malloc_sum = 0

malloc_re = re.compile(r'^M (.*)$')
free_re = re.compile(r'^F (.*)$')
realloc_re = re.compile(r'^R (.*)$')

for line in trace_file:
    try:
        if re.match(malloc_re, line):
            words = line.split()
            # store allocated size
            blocks[words[1]] = int(words[2])
            sum_of_malloc += int(words[2])
            current_malloc_sum += int(words[2])
            max_malloc_sum = max(max_malloc_sum, current_malloc_sum)

        elif re.match(free_re, line):
            pointer = line.split()[1]
            if blocks.has_key(pointer):
                sum_of_free += blocks[pointer]
                current_malloc_sum -= blocks[pointer]
                del blocks[pointer]

        elif re.match(realloc_re, line):
            words = line.split()
            if words[1] in blocks:
                sum_of_realloced_full += blocks[words[1]]
                sum_of_realloced_pointers += int(words[2])
                current_malloc_sum -= blocks[words[1]]

    except (ValueError, IndexError) as e:
        print line  # + " " + str(e)

print "current_malloc_sum: " + str(current_malloc_sum) + " Bytes"
print "max_malloc_sum: " + str(max_malloc_sum) + " Bytes"

print "Total malloc: " + str(sum_of_malloc) + " Bytes"
print "Total malloc - free: " + str(sum_of_malloc - sum_of_free) + " Bytes"
print "Size of remaining non-freed blocks: " + str(sum(blocks.values())) + " Bytes"

print "Size of realloced virtual objects (full): " + str(sum_of_realloced_full) + " Bytes"
print "Size of realloced virtual object pointers: " + str(sum_of_realloced_pointers) + " Bytes"

print "Percentage of vobjects to total malloced blocks: " + str(100 * (float(sum_of_realloced_full) / float(sum_of_malloc))) + " %"
print "Percentage of vobject pointers to total malloced blocks: " + str(100 * (float(sum_of_realloced_pointers) / float(sum_of_malloc))) + " %"
