#include "functions.h"


// Reading from files all the details
int readFromFile(char **Seq1, char ***Seq2, Weight *weights, int *numOfSequences){

    char charBuffer[5000] = {0};
	FILE *file = fopen(FILE_NAME, "r");
	if (!file){
		fclose(file);
		return 0;
	}  
    // Read the weights from the file
	fscanf(file, "%f%f%f%f", &weights->w1, &weights->w2, &weights->w3,&weights->w4);

    // Reading main Sequence and copying it to Seq1
    fscanf(file,"%s",charBuffer);
	*Seq1 = (char*)malloc(strlen(charBuffer) + 1);
	strcpy(*Seq1, charBuffer);

    // Read the number of sequences for memory allocation.
	fscanf(file, "%d", numOfSequences);

	// Allocate memory for sequence array
	*Seq2 = (char**) calloc(*numOfSequences,sizeof(char*));

    // Read all comparison Sequnces
    for(int i =0 ; i < *numOfSequences; i++) {
        fscanf(file, "%s", charBuffer);
        (*Seq2)[i] = (char*) calloc(strlen(charBuffer), sizeof(char));
        strcpy((*Seq2)[i], charBuffer);

    }
    fclose(file);
	return 1;
}

// Writing to file the maximum score for each Sequence
int writeToFile(Score* maxScore){

	FILE *file = fopen(OUTPUT_FILE, "a");
	if (!file){
		fclose(file);
		return 0;
	}
	// Writing the score after getting it from the slave
	fprintf(file,"The best score is: OffSet = %d, First letter Index = %d, Last letter Index = %d,score = %f\n",maxScore->off,maxScore->n,maxScore->k,maxScore->scoreWeight);
	fclose(file);
	return 1;
}

// The Master sends to each process the weight and the main sequence to work on them.
void sendMainSequence(Weight* weight,char* Seq1,int numProc,int tag){

	int sequenceLength = strlen(Seq1);
	for(int i = 1 ; i < numProc; i++){

		MPI_Send(weight,1,weightMPIType(),i,tag,MPI_COMM_WORLD);
		MPI_Send(&sequenceLength,1,MPI_INT,i,tag,MPI_COMM_WORLD);
		MPI_Send(Seq1,sequenceLength,MPI_CHAR,i,tag,MPI_COMM_WORLD);
	}
}

// Splitting the sequences between all the proccess and splitting the strings.
void sendNumberOfSequence(char** Seq2,int numOfSequences, int numProc, int tag){

	int seqSize;
	for(int i = 1,seqNum = 0; i < numProc; i++){ 

		int tasks = (numOfSequences / (numProc-1)) + ((i <= (numOfSequences % (numProc-1)))?1:0);  // Calculating the tasks for each
		MPI_Send(&tasks,1,MPI_INT,i,tag,MPI_COMM_WORLD);
		for(int j = 0; j < tasks; j++,seqNum++){
			seqSize = strlen(Seq2[seqNum]);
			MPI_Send(&seqSize,1,MPI_INT,i,tag,MPI_COMM_WORLD);
			MPI_Send(Seq2[seqNum], strlen(Seq2[seqNum]),MPI_CHAR,i,tag,MPI_COMM_WORLD);
		}
	}
}

// The Master collects all the scores for the slaves and write them to the file
void receiveScores(int numProc,int numOfSequences,int tag,Score* maxScore,MPI_Status* status){

	for (int i = 1; i < numProc; i++){

		int tasks = (numOfSequences / (numProc-1)) +((i <= (numOfSequences % (numProc-1)))?1:0); 
		for(int j = 0; j < tasks; j++)
			MPI_Recv(maxScore,1,scoreMPIType(),i,tag,MPI_COMM_WORLD,status);	
	}
}


// The Master receives the Main Sequence Seq1
char* receiveMainSequence(Weight* weight, int source, int tag, MPI_Status* status){

	int sequenceLength;
	char* Seq1;
	MPI_Recv(weight,1, weightMPIType(), source, tag,MPI_COMM_WORLD, status);
	MPI_Recv(&sequenceLength,1,MPI_INT,source,tag,MPI_COMM_WORLD,status);
	Seq1 = (char*)calloc(sequenceLength,sizeof(char));
	MPI_Recv(Seq1, sequenceLength, MPI_CHAR, source, tag,MPI_COMM_WORLD, status);
	return Seq1;
}

// Each processor receives the number of task after the calculation by the Master
int receiveNumberOfSequence(int source, int tag, MPI_Status *status) {

	int tasks;
	MPI_Recv(&tasks, 1, MPI_INT, source, tag, MPI_COMM_WORLD, status);
	return tasks;
}

// Recieve a Sequence from the Master
char* receiveSequence(int source, int tag, MPI_Status* status){

	char* sequence;
	int seqSize;
	MPI_Recv(&seqSize,1,MPI_INT,source,tag,MPI_COMM_WORLD,status);
	sequence = (char*)calloc(seqSize,sizeof(char));
	MPI_Recv(sequence, seqSize, MPI_CHAR, source, tag,MPI_COMM_WORLD, status);
	return sequence;
}

// Each process after calculating the score returns it to the Master
void sendScore(Score* maxScore,int dest,int tag){
	
	MPI_Send(maxScore,1,scoreMPIType(),dest,tag,MPI_COMM_WORLD);
}

// Getting the maximum score of the mutations
Score maxWeight(Score* scoreList, int size){

    Score maxScore = scoreList[0];
	for(int i = 1; i < size; i++){
		if(maxScore.scoreWeight < scoreList[i].scoreWeight){
            maxScore.off = scoreList[i].off;
			maxScore.n = scoreList[i].n;
			maxScore.k = scoreList[i].k;
			maxScore.scoreWeight = scoreList[i].scoreWeight;
	
		}
	}
	return maxScore;
}

// Calculates score for each mutation by the weights specified in the file
float calcWeight(const char* str, const Weight* weights)
{
	int NumberOfStars = 0,NumberOfColons = 0,NumberOfPoints = 0,NumberOfSpaces = 0;
	for(int i = 0 ; i < strlen(str) ; i++){  // Basic switch case to make the counting easier
		switch(str[i]){
		case '*':
			NumberOfStars++;
			break;
		case ':':
			NumberOfColons++;
			break;
		case '.':
			NumberOfPoints++;
			break;
		case ' ':
			NumberOfSpaces++;
			break;
		default:
			break;
		}
	}
	return ((weights->w1) * NumberOfStars) - ((weights->w2) * NumberOfColons) - ((weights->w3) * NumberOfPoints) - ((weights->w4) * NumberOfSpaces); // Calculating weight
}

void calcScore(Weight *weight, char *Seq1, char *currSeq2, Score *maxScore) {
    int sizeOfOffset = strlen(Seq1) - strlen(currSeq2);
    int sizeOfMutationArray = (sizeOfOffset) * (strlen(currSeq2) * (1+strlen(currSeq2))/2); // Calculating the amount of mutations there are offsetSize*((1+n)*n/2)
	Score* scoreArray = (Score*)malloc(sizeof(Score)*(sizeOfMutationArray));
	int count = 0;


	//Going through all options of the mutation with each offset and diffrenet N and K
	#pragma omp parallel for // parllerel work on the mutation to speed  up process
	for( int o = 0 ; o < sizeOfOffset; o++){
		for(int n = 0 ; n < strlen(currSeq2) - 1; n++){
			for(int k = n + 1; k < strlen(currSeq2); k++){
				Score *currentScore = (Score*)malloc(sizeof(Score));
				char* mutant = useGPU(strlen(currSeq2), n , k , o, Seq1, currSeq2, omp_get_thread_num()); // calling Cuda function
				float mutantScore = calcWeight(mutant,weight);
				currentScore->off = o;
				currentScore-> n = n + 1;
				currentScore-> k = k + 1;
				currentScore->scoreWeight = mutantScore;
				memcpy(&scoreArray[count],currentScore, sizeof(Score)); // Copying each score to the array
				if (mutant != NULL)
					free(mutant);
				if(currentScore != NULL)
					free(currentScore);
				count++; // Spreading the mutation maximum scores in the mutation array
			}
		}
	}

	*maxScore = maxWeight(scoreArray, sizeOfMutationArray); // Get the maximum weight from all the mutations
	if (scoreArray != NULL)
		free(scoreArray);
}

//Applying a scoreMPI type for a faster more friendly usage
MPI_Datatype scoreMPIType()
{
	Score maxScore;
    MPI_Datatype ScoreMPIType;
    MPI_Datatype type[3] = { MPI_INT, MPI_INT, MPI_FLOAT };
    int blocklen[3] = { 1, 1, 1 };
    MPI_Aint disp[3];

    disp[0] = (char *) &maxScore.n - (char *) &maxScore;
    disp[1] = (char *) &maxScore.k - (char *) &maxScore;
    disp[2] = (char *) &maxScore.scoreWeight - (char *) &maxScore;

    MPI_Type_create_struct(3, blocklen, disp, type, &ScoreMPIType);
    MPI_Type_commit(&ScoreMPIType);
    return ScoreMPIType;
}

//Applying a weighbtMPI type for a faster more friendly usage
MPI_Datatype weightMPIType()
{
    Weight weights;
    MPI_Datatype weightMPIType;
    MPI_Datatype type[4] = { MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT };
    int blocklen[4] = { 1, 1, 1, 1 };
    MPI_Aint disp[4];

    disp[0] = (char *) &weights.w1 - (char *) &weights;
    disp[1] = (char *) &weights.w2 - (char *) &weights;
    disp[2] = (char *) &weights.w3 - (char *) &weights;
    disp[3] = (char *) &weights.w4 - (char *) &weights;
	
    MPI_Type_create_struct(4, blocklen, disp, type, &weightMPIType);
    MPI_Type_commit(&weightMPIType);
    return weightMPIType;
}
