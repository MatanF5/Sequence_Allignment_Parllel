#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <string.h>
#include "structs.h"


__device__ const char* firstConservative[]={"NDEQ","NEQK","STA","MILV","QHRK","NHQK","FYW","HY","MILF"};

__device__ const char* secondConservative[]={"SAG","ATV","CSA","SGND","STPA","STNK","NEQHRK","NDEQHK","SNDEQK","HFY","FVLIM"};


// Comparing letters 
__device__ int compareChar(const char* s, char c)
{
	do{
		if(*s == c) return 1;
	}while (*s++);
	return 0;
}

// Checking for conservation 
__device__ int checkConservative(const char* conservative[],const int size,char ch1, char ch2)
{
	for (int i =0; i< size;i++)
    {
		if(compareChar(conservative[i],ch1) && compareChar(conservative[i],ch2))
			return 1;
	}
	return 0;
}

// Same function as strcpy in C to help with changing the mutation
__device__ char * my_strcpy(char *dest, const char *src){

	int i = 0;
	do {
		dest[i] = src[i];}
	while (src[i++] != 0);
	return dest;

}


//creating the mutation itself
__global__  void createMutant(char *arr,int SeqLength,int firstIndex,int lastIndex,int offset, char *Seq1, char *currSeq2){

	char* temp = currSeq2;

	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= SeqLength-1)
		return;
	//changing to mutant (dleteing the letters in the indexes given)
	my_strcpy(&temp[firstIndex],&temp[firstIndex+1]);
	my_strcpy(&temp[lastIndex-1],&temp[lastIndex]);

    //Formating the mutation as the given symbols
	if(Seq1[i + offset] == temp[i])
		arr[i] = '*';
	else if (checkConservative(firstConservative,9,Seq1[offset+i], temp[i]))
		arr[i] = ':';
	else if (checkConservative(secondConservative,11,Seq1[offset+i], temp[i]))
		arr[i] = '.';
	else
		arr[i] = ' ';

}

// Using gPU to implement a quicker and more efficent search for mutation and calculations
char* useGPU(int seqLength, int firstIndex, int lastIndex, int offset, char *Seq1, char* currSeq2, int tid){


	// Error code to check return values for CUDA calls
    cudaError_t err = cudaSuccess;

	//Using cuda stream a sequence of operations that execute on the device in the order in which they are issued by the host code
	const int num_streams = 8;
	cudaStream_t stream[num_streams];
	cudaStreamCreate(&stream[tid]);
	// Allocate memory on GPU to copy the data from the host
    char *d_Mutant;
	size_t size = seqLength * sizeof(char);
    err = cudaMalloc((void **)&d_Mutant, size);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	cudaMemset(d_Mutant,0,size);
	 // Allocate memory on GPU to copy the data from the host
    char *d_Seq1;
	size_t size_Seq1= (strlen(Seq1) + 1) * sizeof(char);
    err = cudaMalloc((void **)&d_Seq1, size_Seq1);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    // Copy data from host to the GPU memory
    err = cudaMemcpy(d_Seq1, Seq1, size_Seq1, cudaMemcpyHostToDevice);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	// Allocate memory on GPU to copy the data from the host
    char *d_currSeq2;
	size_t size_currSeq2 = (strlen(currSeq2) + 1) * sizeof(char);
    err = cudaMalloc((void **)&d_currSeq2, size_currSeq2);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Copy data from host to the GPU memory
    err = cudaMemcpy(d_currSeq2, currSeq2, size_currSeq2, cudaMemcpyHostToDevice);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	int threadsPerBlock = 256;
    int blocksPerGrid =(seqLength + threadsPerBlock - 1) / threadsPerBlock;
    // Using 3D array for a faster solution
	createMutant<<<blocksPerGrid, threadsPerBlock, 0 , stream[tid]>>>(d_Mutant,seqLength,firstIndex,lastIndex,offset,d_Seq1,d_currSeq2);
    // check if the mutation failed.
	err = cudaGetLastError();
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to launch vectorAdd kernel -  %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	char* result = (char*)malloc(seqLength*sizeof(char));
    // Copy the  result from GPU to the host memory.
    err = cudaMemcpyAsync(result, d_Mutant, seqLength, cudaMemcpyDeviceToHost, stream[tid]);

    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to copy result array from device to host -%s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    // Free allocated memory on GPU:
    if (cudaFree(d_Mutant) != cudaSuccess){
        fprintf(stderr, "Failed to free device data - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    if (cudaFree(d_Seq1) != cudaSuccess){
        fprintf(stderr, "Failed to free device data - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    if (cudaFree(d_currSeq2) != cudaSuccess){
        fprintf(stderr, "Failed to free device data - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

	cudaStreamDestroy(stream[tid]);
    return result;
}