#pragma once

#define FILE_NAME "input.txt"
#define OUTPUT_FILE "output.txt"
#define MASTER 0


struct Weight {
	float w1;
	float w2;
	float w3;
	float w4;
}typedef Weight;

struct Score{
	int off; // OffSet
	int n; // First letter of mutation
	int k; // The last letter of mutation
	float scoreWeight; // Weight score
}typedef Score;