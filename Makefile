CC=mpicxx

compile:
	$(CC) -DSPRNG_MPI -DUSE_MPI -w -o traffic      main_sprng.cpp -I/opt/nfs/sprng5/include -L/opt/nfs/sprng5/lib -lsprng
run:
	sh station_name_list 101 116 > nodes
	mpiexec -nodes ./traffic
local:
	mpiexec -n 16 ./traffic
clean:
	rm -f *.o
	rm -f *~
	rm traffic
