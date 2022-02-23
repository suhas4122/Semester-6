if  [ $2 -gt 0 -a $2 -lt 5 ] 
then
    cat "$1"|awk -v var="$2" '{print $var}'|sort -f|uniq -ic|sort -nr -k1,1|awk '{print tolower($2),$1}'>>"1c_output_${2}_column.freq"     
fi