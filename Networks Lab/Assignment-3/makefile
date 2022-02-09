all: ftpS ftpC

mylib.o: mylib.c mylib.h
	gcc -Wall -c mylib.c -o mylib.o
ftpS.o: ftpS.c  mylib.h
	gcc -Wall -c ftpS.c -o ftpS.o
ftpC.o: ftpC.c  mylib.h
	gcc -Wall -c ftpC.c -o ftpC.o
ftpS: ftpS.o mylib.o
	gcc -Wall ftpS.o mylib.o -o ftpS 
ftpC: ftpC.c mylib.o
	gcc -Wall  ftpC.c mylib.o -o ftpC

clean:
	rm -f ftpC ftpS mylib.o ftpS.o ftpC.o .tmp*.txt 
