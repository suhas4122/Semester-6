mkdir 1.b.files.out
for i in 1.b.files/*
do
	sort -nr $i -o 1.b.files.out/"$(basename -- $i)"
    echo "Sorted $(basename -- $i)"
done
sort -nmr 1.b.files.out/* -o 1.b.out.txt 
echo "Merged all the files"