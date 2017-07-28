# Make file for sim
# should handle multicore simulations also

OS=`arch`

sim: sim.c
	gcc -g -Wall -o sim sim.c
install:
	@if [ "$(OS)" = "sol-sparc64" -o "$(OS)" = "sun4" ]; then cp sim $(HOME)/tools/.solaris/;echo "copying exe to $(HOME)/tools/.solaris";chmod -f 755 $(HOME)/tools/.solaris/sim;echo "changing exe permissions(755)"; else cp sim $(HOME)/tools/;echo "copying exe to $(HOME)/tools";chmod -f 755 $(HOME)/tools/sim;echo "changing exe permissions(755)"; fi

clean:
	@echo "cleaning sim"
	rm -f sim *.o *.hex

