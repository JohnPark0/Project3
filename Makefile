all : FileSystem.out ImgMaker.out

FileSystem.out : RR.o FileSys.o Schedule.o
	gcc -o Filesystem.out RR.o FileSys.o Schedule.o

ImgMaker.out : ImgMaker.o FileSys.o
	gcc -o ImgMaker.out ImgMaker.o FileSys.o

RR.o : RR.c
	gcc -c -o RR.o RR.c

FileSys.o : FileSys.c
	gcc -c -o FileSys.o FileSys.c

Schedule.o : Schedule.c
	gcc -c -o Schedule.o Schedule.c

ImgMaker.o : ImgMaker.c
	gcc -c -o ImgMaker.o ImgMaker.c

clean :
	rm *.o