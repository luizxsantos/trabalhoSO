#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

// Definindo o limite de threads e quantidade de inteiros em cada arquivo.
#define MAX_T 8
#define TAM_ARQ 1000

// Struct para as informações das threads
typedef struct {
    int *data;
    int start;
    int end;
    int thread_id;
    struct timespec start_time, end_time;
} ThreadData;

// Definindo os protótipos das funções
void mergeArq(int *data, int start, int mid, int end);
void mergeSort(int *data, int start, int end);
void *sortSegment(void *arg);
int loadData(char **inputFiles, int numFiles, int **data); 
void saveData(const char *outputFilename, int *data, int totalElements);

//main
int main(int argc, char *argv[]) {

    // convertendo o número de threads da linha de comando para
    int threadQtd = atoi(argv[1]);
    if (threadQtd !=1 && threadQtd !=2 && threadQtd != 4 && threadQtd != 8) {
        fprintf(stderr, "quantidade de inválida\n");
        return 1;
    }

    //arquivo de saída
    char *outputFilename = NULL;
    int numFiles = argc- 4;
    char **inputFiles = argv+ 2;

    for (int i = 2; i< argc;i++) {
        if (strcmp(argv[i], "-o") == 0) {
            outputFilename = argv[i+1];
            numFiles = i-2;
            break;
        }
    }

    if (outputFilename == NULL) {
        fprintf(stderr, "informe o arquivo de saida");
        return 1;
    }

    // carregando os dados dos arquivos de entrada
    int *data;
    int totalElements = loadData(inputFiles, numFiles, &data);

    pthread_t threads[threadQtd];
    ThreadData threadData[threadQtd];
    int size = totalElements / threadQtd;

    //cria e inicia as threads
    for (int i = 0; i < threadQtd; i++) {
        threadData[i].data = data;
        threadData[i].start = i * size;
        threadData[i].end = (i == threadQtd - 1) ? totalElements - 1 : (i + 1) * size - 1;
        threadData[i].thread_id = i;
        pthread_create(&threads[i], NULL, sortSegment, &threadData[i]);
    }
    //variavel para obter o tempo total de execução
    float time;
    // aguardar o término de todas as threads
    for (int i = 0; i < threadQtd; i++) {
        pthread_join(threads[i], NULL);
        double threadTime = (threadData[i].end_time.tv_sec - threadData[i].start_time.tv_sec) +(threadData[i].end_time.tv_nsec - threadData[i].start_time.tv_nsec) / 10e9;
        printf("Tempo de execução da Thread %d: %.10f segundos.\n", i, threadTime);
        time += threadTime;
    }
    printf("Tempo total de execução: %.10f segundos.\n", time);

    //juntar os segmentos ordenados
    for (int i = 1; i < threadQtd; i++) {
        mergeArq(data, 0, threadData[i - 1].end, threadData[i].end);
    }

    //gravar os dados no arquivo de saída
    saveData(outputFilename, data, totalElements);

    free(data);
    return 0;
}

void mergeArq(int *data, int start, int mid, int end) {
    int *temp = (int *) malloc((end - start + 1) * sizeof(int));
    int i = start, j = mid + 1, k = 0;

    while (i <= mid && j <= end) {
        if (data[i] <= data[j]) {
            temp[k++] = data[i++];
        } else {
            temp[k++] = data[j++];
        }
    }

    while (i <= mid) temp[k++] = data[i++];
    while (j <= end) temp[k++] = data[j++];

    for (i = start, k = 0; i <= end; i++, k++) {
        data[i] = temp[k];
    }

    free(temp);
}

//função para ordenação com mergesort
void mergeSort(int *data, int start, int end) {
    if (start < end) {
        int mid = start + (end - start) / 2;
        mergeSort(data, start, mid);
        mergeSort(data, mid + 1, end);
        mergeArq(data, start, mid, end);
    }
}

//função para carregar dados dos arquivos de entrada
int loadData(char **inputFiles, int numFiles, int **data) {
    int totalElements = 0;

    // primeiro, calcular o número total de elementos
    for (int i = 0; i < numFiles; i++) {
        FILE *file = fopen(inputFiles[i], "r");
        if (!file) {
            perror("erro ao abrir o arquivo");
            exit(EXIT_FAILURE);
        }
        int temp;
        while (fscanf(file, "%d,", &temp) == 1) {
            totalElements++;
        }
        fclose(file);
    }

    // Alocar espaço para os dados
    *data = (int *) malloc(totalElements * sizeof(int));
    int index = 0;

    // Ler os dados para o array
    for (int i = 0; i < numFiles; i++) {
        FILE *file = fopen(inputFiles[i], "r");
        if (!file) {
            perror("erro ao abrir o arquivo de entrada");
            exit(EXIT_FAILURE);
        }
        int temp;
        while (fscanf(file, "%d,", &temp) == 1) {
            (*data)[index++] = temp;
        }
        fclose(file);
    }

    return totalElements;
}

//função que cada thread irá executar
void *sortSegment(void *arg) {
    ThreadData *threadData = (ThreadData *) arg;
    clock_gettime(CLOCK_MONOTONIC, &threadData->start_time);
    // Ordena o segmento usando Merge Sort
    mergeSort(threadData->data, threadData->start, threadData->end);
    clock_gettime(CLOCK_MONOTONIC, &threadData->end_time);
    pthread_exit(NULL);
}


// Função para salvar dados no arquivo de saída
void saveData(const char *outputFilename, int *data, int totalElements) {
    FILE *file = fopen(outputFilename, "w");
    if (!file) {
        perror("erro ao abrir o arquivo de saída");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < totalElements; i++) fprintf(file, "%d\n", data[i]);

    fclose(file);
}
