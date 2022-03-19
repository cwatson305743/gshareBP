#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main() {
	// variables for opening trace file
	FILE* trace;
	char fileName[30];
	errno_t err;

	// gshare parameters
	const int M; // number of low order PC bits used for indexing prediction table
	const int N; // number of bits in global branch history register

	// read command-line arguments for gshare parameters
	printf("Enter integer value for m: ");
	scanf("%i", &M);
	printf("Enter integer value for n: ");
	scanf("%i", &N);
	printf("Enter name of trace file: ");
	scanf("%s", &fileName);

	// more branch predictor model parameters
	unsigned long int pc; // holds adress of branch instruction
	unsigned int index; // index into prediction table
	char actual; // actual outcome of branch instruction from trace file (taken/not taken)
	unsigned long pcBM = (1 << M) - 1; // bitmask used for isolating bits m+1:2 of PC
	unsigned long indexBM = (1 << M - N) - 1; // bitmask used for isolating lower M-N bits of PC bits m+1:2 used to form index
	int miss = 0; // number of branch mispredictions
	int branches = 0; // number of branches
	const int SIZE = pow(2, M); // number of entries in prediction table: 2^M
	int gr = 0; // global branch history register initialized to zero
	int grTemp; // temp var for holding upper N bits of index

	int* buffer = (int*)malloc(SIZE * sizeof(int)); // prediction table array holds 2^M ints

	if (buffer == NULL) {
		// malloc returns NULL if memory cannot be allocated
		printf("Error: memory could not be allocated for prediction table\n");
		exit(0);
	}

	// initialize all prediction table entries to 2
	for (int i = 0; i < SIZE; i++) {
		buffer[i] = 2;
	}

	if ((err = fopen_s(&trace, fileName, "r")) != 0) {
		// File could not be opened
		// print error message to stderr
		fprintf(stderr, "cannot open file '%s': %s\n", fileName, strerror(err));
		exit(0);
	}
	else {
		// File opened successfully
		// read file until end-of-file indicator is set
		while (!feof(trace)) {
			fscanf(trace, "%x %c\n", &pc, &actual);

			pc = pc >> 2; // remove lower two bits of PC (always zero)
			index = pc & pcBM; // bimodal model index: PC bits m+1:2

			// update index if gshare model is chosen
			if (N > 0) {
				grTemp = gr ^ (index >> M - N); // N gr bits XORed with upper N bits of PC bits m+1:2
				grTemp = grTemp << M - N; // needed for later to form index
				index = (index & indexBM) | grTemp;
			}
			
			if (actual == 't') { // if actual branch outcome is taken
				if (buffer[index] == 3 || buffer[index] == 2) { // prediction == actual
					if (buffer[index] == 2) {
						buffer[index]++;
					}
				}
				else { // prediction =/= actual
					buffer[index]++;
					miss++;
				}

				// update gr if gshare model is chosen
				if (N > 0) {
					gr = (gr >> 1) | (1 << N - 1); // update MSB of gr
				}
			}
			else { // if actual branch outcome is not taken
				if (buffer[index] == 3 || buffer[index] == 2) { // prediction =/= actual
					buffer[index]--;
					miss++;
				}
				else { // prediction == actual
					if (buffer[index] == 1) {
						buffer[index]--;
					}
				}

				// update gr if gshare model is chosen
				if (N > 0) {
					gr = (gr >> 1) | (0 << N - 1); // update MSB of gr
				}
			}

			branches++;
		}

	}

	// print outputs console
	char predictionStr[] = "number of prediction:";
	char mispredictionStr[] = "number of mispredictions:";
	char rateStr[] = "misprediction rate:";
	printf("\nOUTPUT\n");
	printf("\t%-30s %-30d\n", predictionStr, branches);
	printf("\t%-30s %-30d\n", mispredictionStr, miss);
	printf("\t%-30s %-5.*f%%\n\n", rateStr, 2, 100 * (float)miss / branches);
	char indexStr[] = "INDEX";
	char bimodalStr[] = "BIMODAL";
	printf("%-10s\t%-10s\n", indexStr, bimodalStr);
	for (int i = 0; i < SIZE; i++) {
		printf("%-10d\t%-10d\n", i, buffer[i]);
	}

	free(buffer); // deallocate memory 

	return 0;
}