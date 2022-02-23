if [ ! -f "$1" ]
then
    printf "File not found\n"
    exit
fi
if [[ $1 = *.tar.bz2 ]]
then
    tar xvjf $1
elif [[ $1 == *.tar.gz ]]
then
    tar xvzf $1
elif [[ $1 == *.bz2 ]]
then
    bzip2 -d -k $1
elif [[ $1 == *.rar ]]
then
    unrar x $1
elif [[ $1 == *.gz ]]
then
    gzip -dk $1
elif [[ $1 == *.tar ]]
then
    tar xvf $1
elif [[ $1 == *.tbz2 ]]
then
	tar xvjf $1
elif [[ $1 == *.tgz ]]
then
	tar xvzf $1
elif [[ $1 == *.zip ]]
then
    unzip $1
elif [[ $1 == *.Z ]]
then
   	uncompress $1
elif [[ $1 == *.7z ]]	
then
    7z x $1
else
	echo "Extension not supported"
fi
