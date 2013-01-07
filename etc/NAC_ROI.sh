#!/bin/sh

# ./NAC_ROI.sh $1 $2 $3
#     $1 is the file name
#     $2 is the page size
#     $3 is the tree depth

# Extract the file tag from the name and construct the SCM names.

tag=$(echo $1 | sed -e 's/NAC_ROI_\(.*\)_.*\.IMG/\1/')

txt="NAC_ROI_$tag-$2-$3.txt"
tmp="NAC_ROI_$tag-$2-$3-M.tif"
scm="NAC_ROI_$tag-$2-$3.tif"

echo "Converting $1 to $scm"

# Determine the data range.

range=$(scmtiff -pextrema $1)

n0=$(echo "$range" | cut -f1)
n1=$(echo "$range" | cut -f2)

echo "$1 normalized to $n0 $n1"                                > $txt
echo "Source: LRO-L-LROC-5-RDR-V1.0 ARIZONA STATE UNIVERSITY" >> $txt
echo "SCM TIFF: Copyright (C) 2012-13 Robert Kooima"          >> $txt

# Convert to SCM TIFF

set -x

scmtiff -T -pconvert -n$2 -d$3 -b8 -A -N$n0,$n1 -o$tmp $1
scmtiff -T -pmipmap -A $tmp
scmtiff -T -pborder -o$scm $tmp
scmtiff -T -pfinish -t$txt $scm
