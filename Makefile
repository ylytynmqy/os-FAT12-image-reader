all:
	nasm -f elf32 my_print.asm
	g++ -std=c++11 -m32 main.cpp my_print.o -o main
clean:
	rm py_print.o
