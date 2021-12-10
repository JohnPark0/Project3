FileSystem.out : RR.o FileSys.o Schedule.o
	gcc -o Filesystem.out RR.o FileSys.o Schedule.o

RR.o : RR.c
	gcc -c -o RR.o RR.c

FileSys.o : FileSys.c
	gcc -c -o FileSys.o FileSys.c

Schedule.o : Schedule.c
	gcc -c -o Schedule.o Schedule.c

clean :
	rm *.o
