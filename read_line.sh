#!/bin/bash
input="./block.txt"
while IFS= read -r line
do
  echo "$line"
  read varname
  echo "$varname"
  #gr_modtool add -t general -l cpp "$line"
done < "$input"
