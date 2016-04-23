CC=mpicc

traffic:
	$(CC) $(LFLAGS) main.c -o traffic
	mpiexec -n 16 ./traffic
clean:
	rm -f *.o
	rm -f *~
	rm traffic
