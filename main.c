#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include "structs.h"
#include "functions.h"


int main(int argc, char* argv[]){
    int my_Rank; // Rank of proccess
    int num_Proc; // number of proccess
    int tag = 0; // tag for messages
    MPI_Status status; // status of reciving

    char ** Seq2; // All Strings for comparison
    char *Seq1, *currSeq2; // Seq1 is main Sequence and currSeq2 is the compared one
    int numOfSequence;
    Weight weights = {0};
    Score Score = {0};

    	
	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &my_Rank);

	MPI_Comm_size(MPI_COMM_WORLD, &num_Proc);

    // Checking for 2 proccesses.
    if(num_Proc < 2){
        printf("2 Proccess are needed to run the program.\n");
		MPI_Abort(MPI_COMM_WORLD,1);
    }

    if (my_Rank == MASTER){
        if(!readFromFile(&Seq1, &Seq2, &weights, &numOfSequence)){
            printf("Failed to read file\n");
            MPI_Abort(MPI_COMM_WORLD,1);   
        } else {
            sendMainSequence(&weights, Seq1, num_Proc, tag); // Sending the main Sequence to all the slaves
			sendNumberOfSequence(Seq2, numOfSequence, num_Proc, tag); // Sending the number of Sequences to split between the slaves
			receiveScores(num_Proc,numOfSequence,tag,&Score,&status); // Collecting scores
        }

    } else{
        Seq1 = receiveMainSequence(&weights, MASTER, tag, &status); // Recieving main Sequence
		numOfSequence = receiveNumberOfSequence(MASTER, tag, &status); // Getting the number of sequences

        for (int i = 0; i < numOfSequence; i++){
			currSeq2 = receiveSequence(0, tag, &status);
            calcScore(&weights,Seq1, currSeq2, &Score); // Calculating the score
			printf("The best score is: offset = %d, First Index = %d, Last Index = %d, score = %f\n",Score.off,Score.n,Score.k,Score.scoreWeight);
            writeToFile(&Score); // wrting to output file
			sendScore(&Score,MASTER,tag); // Sending to master
		}
    }

    MPI_Finalize();
	return 0;
}