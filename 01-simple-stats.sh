#!/bin/bash

# simple stats -- bytes, lines, longest line.

echo "Name, Bytes, Lines"
echo "---"
find data-sets -name "*.dict" -exec wc --bytes -l {} \; | ruby -ne 'file, size, lines = $_.split().reverse(); puts ("%-20s %10d %10s" % [file[/([^\/]+$)/], size, lines])' | sort