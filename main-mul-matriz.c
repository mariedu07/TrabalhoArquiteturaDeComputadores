#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <omp.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

/*SE QUISER FAZER MOSTRAR O TEMPO EM MS, TIRA A LINHA 18*/

#define START_STOPWATCH( prm ) {gettimeofday( &prm.mStartTime, 0);} //relogio comeca a contar

#define STOP_STOPWATCH( prm ) {                                                        \
  gettimeofday( &prm.mEndTime, 0); /*pega o horario exato q parou*/                                                    \
  prm.mElapsedTime = (1000.0f * ( prm.mEndTime.tv_sec - prm.mStartTime.tv_sec) + (0.001f * (prm.mEndTime.tv_usec - prm.mStartTime.tv_usec)) );  \
  prm.mElapsedTime /= 1000.0f;                                                         \
}// relogio para de contar e ve quanto tempo se passou

typedef struct
{
  struct timeval mStartTime;
  struct timeval mEndTime;
  double mElapsedTime;
} STOPWATCH; //estrutura do cronometro

#define EPSILON 1E-30
#define ALING 64

typedef struct {
    int n, m;
    double *vetor;
}MATRIZ;

//ASSINATURAS DAS FUNCOES

size_t saveBin(MATRIZ *A, char *nomeArqv); //grava vet e matrizes no formato bin, *A estrutura de matriz que vai ser gravada em disco, nomeArqv nome do arquivobinario e return qtd bytes lidos

size_t loadBin(MATRIZ *A, char *nomeArqv); //size-t é um tipo especifico para tamanhos em c

void multMatriz(MATRIZ *  __restrict__ C, //endereco da memoria da matriz resultado
                  MATRIZ *  __restrict__ A, //endereco da memoria da matriz A
                  MATRIZ *  __restrict__ B, //endereco da memoria da matriz B
                  unsigned int nThreads){ // quantas threads vao ser usadas na multiplicacao

     #pragma omp parallel // avisa ao compilador que o bloco dele deve ser executado de forma paralela
     {
	#pragma omp for //abre o bloco
        for (int j = 0; j < C->n; j++){
            for (int i = 0; i < C->m; i++){
                double c = 0.0f; //vai acumular a soma
                for (int jA = 0; jA < A->m; jA++){ 
                    int ak = j * A->m + jA;
                    int bk = jA * B->m + i;
                    c += A->vetor[ak] * B->vetor[bk];
                }

                int ck = j * C->m +  i;
                C->vetor[ck] = c;
            }

        }
      }
}

// ./multi-mat-cpu.exec r2000x2000.bin i2000x2000.bin resultado.bin 4
int main (int ac, char **av){

    MATRIZ A, B, C;
    STOPWATCH stopwatch;
    START_STOPWATCH(stopwatch);


    char  *nomeArqvA = av[1],
          *nomeArqvB = av[2],
          *nomeArqvC = av[3];

    unsigned int nThreads = atoi(av[4]);
    double contaBytes = 0.0,
           *elapsedtime = NULL; //vetor para armazenar o tempo gasto por cada thread


    contaBytes  =  (double)loadBin(&A, nomeArqvA); //ve quantos bytes tem em a
    contaBytes  += (double) loadBin(&B, nomeArqvB); // ve quantos bytes tem em a e b
    C.m = A.n;
    C.n = B.m;

    C.vetor = (double *) aligned_alloc(ALING, C.m *  C.n * sizeof(double)); // aloca a memoria para a matriz resultado, o tamanho é m*n e cada elemento é um double, o alinhamento é de 64 bytes
    contaBytes  += (double) C.m *  C.n * sizeof(double); // ve quantos bytes tem em a, b e c
    bzero(C.vetor, C.m *  C.n * sizeof(double)); // limpa o espaco q acabamos de alocar


    elapsedtime = (double *) aligned_alloc(ALING, nThreads * sizeof(double)); //aloca a memoria para o vetor de tempo gasto por cada thread
    printf("\nMultiplicação de matrizes\n");
    printf("\t  Matriz A: %s \n", nomeArqvA);
    printf("\t  Matriz B: %s \n", nomeArqvB);
    printf("\t  Matriz C: %s \n", nomeArqvC);
    printf("\t   Threads: %u \n", nThreads);
    printf("\t   Memória: %lf MBytes \n", ((contaBytes) / 1048576));

    multiMatriz(&C, &A, &B, 2);
    saveBin(&C, nomeArqvC);

    printf("----------------------------------------------------------------\n");
    for (unsigned int i = 0; i < nThreads; i++){
        printf(" Tempo gasto pela thread [%.3u] foi de %15.8lf segundos\n", i, elapsedtime[i]);
    }

    free(A.vetor);
    free(B.vetor);
    free(C.vetor);


    STOP_STOPWATCH(stopwatch);
    printf("----------------------------------------------------------------\n");
    printf("\tTempo total de execução: %lf\n", stopwatch.mElapsedTime);
    printf("----------------------------------------------------------------\n");
    return EXIT_SUCCESS;
}

size_t saveBin(MATRIZ *A, char * filename){
    FILE *ptr = fopen(filename, "wb");
    size_t bytes_written = 0, aux;
    assert(ptr != NULL);
    aux = fwrite(&A->m, sizeof(A->m), 1, ptr); bytes_written += aux * sizeof(A->m);
    aux = fwrite(&A->n, sizeof(A->n), 1, ptr); bytes_written += aux * sizeof(A->n);
    aux = fwrite(A->vetor, sizeof(double), A->m * A->n, ptr); bytes_written += aux * sizeof(double);

    fclose(ptr);

}

size_t loadBin(MATRIZ *A, char *filename){
    FILE *ptr = fopen(filename, "rb");
    assert(ptr != NULL);
    double *a = NULL;
    size_t bytes_read = 0, aux;
    //numread = fread( list, sizeof( char ), 25, stream );

    aux = fread(&A->m, sizeof(A->m), 1, ptr); bytes_read += aux * sizeof(A->m);
    aux = fread(&A->n, sizeof(A->n), 1, ptr); bytes_read += aux * sizeof(A->n);

    a = (double *) aligned_alloc(ALING,  A->m *  A->n * sizeof(double));
    assert(a != NULL);

    aux = fread(a, sizeof(double), A->m * A->n, ptr); bytes_read += aux * sizeof(double);

    A->vetor = a;
    fclose(ptr);
    return bytes_read;
    //print2Console(A);
}
