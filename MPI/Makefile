build: apply_filters

apply_filters: apply_filters.c
	mpicc -o apply_filters apply_filters.c -lm -Wall

serial: homework
	mpirun -np 1 apply_filters imagini.in

distrib: homework
	mpirun -np 4 apply_filters imagini.in

clean:
	rm -f apply_filters
