HEADERS = cpu.h

cpu.o: cpu.c $(HEADERS)
	gcc -c cpu.c -o cpu.o
cpu: cpu.o 
	gcc cpu.o   -lpthread -lcommons -o cpu
clean:
	touch cpu.o; $(RM) cpu.o
	touch cpu; $(RM) cpu
	touch cpu.log; $(RM) cpu.log
