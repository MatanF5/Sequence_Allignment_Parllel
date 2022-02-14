#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include "structs.h"


MPI_Datatype weightMPIType();
MPI_Datatype scoreMPIType();

int readFromFile(char **Seq1, char ***Seq2, Weight *weights, int *numOfSequences);

int writeToFile(Score* maxScore);

void sendMainSequence(Weight* weight,char* Seq1,int numProc,int tag);

void sendNumberOfSequence(char** Seq2,int numOfSequences, int numProc, int tag);

void receiveScores(int numProc,int numOfSequences,int tag,Score* maxScore,MPI_Status* status);

char* receiveMainSequence(Weight* weight, int source, int tag, MPI_Status* status);

int receiveNumberOfSequence(int source, int tag, MPI_Status *status);

char* receiveSequence(int source, int tag, MPI_Status* status);

void sendScore(Score* maxScore,int dest,int tag);

float calcWeight(const char* str, const Weight* weights);

void calcScore(Weight *weight, char *Seq1, char *currSeq2, Score *maxScore);

char* useGPU(int seqLength, int firstIndex, int lastIndex, int offset, char *Seq1, char* currSeq2, int tid);

Score maxWeight(Score* scoreList, int size);