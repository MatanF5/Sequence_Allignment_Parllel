build:
	mpicxx -fopenmp -c main.c
	mpicxx -fopenmp -c functions.c
	nvcc -I./inc -c cudaFunc.cu
	mpicxx -fopenmp -o finalProject main.o functions.o cudaFunc.o /usr/local/cuda-11.0/lib64/libcudart_static.a -ldl -lrt

clean:
	rm -f *.o ./finalProject
	rm -f output.txt

run:
	mpiexec -n 2 ./finalProject