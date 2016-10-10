all: pipe

pipe: main.o
	gcc -o pipe main.o

main.o: main.c
	gcc -c main.c -o main.o

clean:
	rm -f main.o pipe core *~
