gcd()
{   
    if [ $2 -eq 0 ] 
    then
		echo $1 
	else
		k=`expr $1 % $2`
		res=`gcd $2 $k`
		echo $res 
	fi
}

ans=0 count=0
for i in $(echo $@|sed "s/,/ /g")
do 
	ans=`gcd $i $ans`
	count=$((count+1))
done

if [ $count -lt 10 ]
then 
    echo $ans
fi
   