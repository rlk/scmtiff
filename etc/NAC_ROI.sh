#!/bin/sh

# ./NAC_ROI.sh $1 $2 $3
#     $1 is the file name
#     $2 is the page size
#     $3 is the tree depth

# Extract the file tag from the name and construct the SCM names.

tag=$(echo $1 | sed -e 's/NAC_ROI_\(.*\)_.*\.IMG/\1/')

tmp="NAC_ROI_$tag-$2-$3-M.tif"
scm="NAC_ROI_$tag-$2-$3.tif"

txt="desc.txt"

echo "Converting $1 to $scm"

# Determine the data range.

range=$(scmtiff -pextrema $1)

n0=$(echo "$range" | cut -f1)
n1=$(echo "$range" | cut -f2)

# Convert to SCM TIFF

set -x

scmtiff -T -pconvert -n$2 -d$3 -b8 -A -N$n0,$n1 -o$tmp $1
scmtiff -T -pmipmap $tmp
scmtiff -T -pborder -o$scm $tmp
scmtiff -T -pfinish -t$txt $scm
