main:  
	nasm -f elf -o asm.o -g main.asm  
	ld -m elf64 -s -o main asm.o 

clean:  
	rm -rf asm.o main.o main
