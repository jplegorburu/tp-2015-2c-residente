HEADERS = planificador.h

planificador.o: planificador.c $(HEADERS)
	gcc -c planificador.c -o planificador.o
planificador: planificador.o 
	gcc planificador.o   -lpthread -lcommons -o planificador
clean:
	touch planificador.o; $(RM) planificador.o
	touch planificador; $(RM) planificador
	touch Consola_planificador.txt; $(RM) Consola_planificador.txt
	touch planificador.log; $(RM) planificador.log
