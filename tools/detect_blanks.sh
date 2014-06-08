#!/bin/bash

function analyze_indentation() {

gawk '
/^[[:blank:]]+/ {
  scheme = ""
  last = "X"
  n = 1
  while (1) {
    char = substr($0, n, 1)
    if (char == " " && last != "S") {
      last = "S"
      scheme = scheme last;
    }
    else if (char == "	" && last != "T") {
      last = "T"
      scheme = scheme last
    }
    else
      break
    n += 1
  }
  print scheme
}
' $1 | sort -u
}

while [ $# -ne 0 ]
do
    echo $1
    analyze_indentation $1
    shift
done
