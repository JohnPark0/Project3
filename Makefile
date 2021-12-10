FileSystem.out : RR.o FileSys.o
	gcc -o Filesystem.out RR.o FileSys.o

RR.o : RR.c
	gcc -c -o RR.o RR.c

FileSys.o : FileSys.c
	gcc -c -o FileSys.o FileSys.c

clean :
	rm *.o
