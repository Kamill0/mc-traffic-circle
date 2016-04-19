CC=mpicc

traffic:
	$(CC) $(LFLAGS) main.c -o traffic
	mpiexec ./traffic
clean:
	rm -f *.o
	rm -f *~
	rm traffic
