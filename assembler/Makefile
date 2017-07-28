OS=`arch`
asm: asm.y asm.l asm_main.c semantic.c semantic.h parser.h
	bison -d -v -o y.tab.c asm.y
	flex asm.l
	gcc -c asm_main.c
	gcc -c lex.yy.c
	gcc -c y.tab.c
	gcc -c semantic.c
	gcc -o asm asm_main.o lex.yy.o y.tab.o semantic.o

install:
	@if [ "$(OS)" = "sol-sparc64" -o "$(OS)" = "sun4" ]; then cp asm $(HOME)/tools/.solaris/;echo "copying exe to $(HOME)/tools/.solaris";chmod -f 755 $(HOME)/tools/.solaris/asm;echo "changing exe permissions(755)"; else cp asm $(HOME)/tools/;echo "copying exe to $(HOME)/tools";chmod -f 755 $(HOME)/tools/asm;echo "changing exe permissions(755)"; fi

clean:	
	rm -f asm *.hex
	rm -f lex.yy.c
	rm -f *.o
	rm -f y.*
