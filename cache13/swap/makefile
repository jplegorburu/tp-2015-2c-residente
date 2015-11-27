HEADERS = swap.h

swap.o: swap.c $(HEADERS)
	gcc -c swap.c -o swap.o
swap: swap.o 
	gcc swap.o   -lpthread -lcommons -o swap
clean:
	touch swap.o; $(RM) swap.o
	touch swap; $(RM) swap
	touch swap.data; $(RM) swap.data
	touch swap.log; $(RM) swap.log
