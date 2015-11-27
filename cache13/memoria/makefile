HEADERS = memoria.h

memoria.o: memoria.c $(HEADERS)
	gcc -c memoria.c -o memoria.o
memoria: memoria.o 
	gcc memoria.o   -lpthread -lcommons -o memoria
clean:
	touch memoria.o; $(RM) memoria.o
	touch memoria; $(RM) memoria
	touch memoria.log; $(RM) memoria.log
