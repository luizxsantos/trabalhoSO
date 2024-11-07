#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#define MAX_THREADS 8

// Estrutura para armazenar os dados necessários para cada thread
typedef struct {
    int *data;
    int start;
    int end;
} ThreadData;

// Função para mesclar duas sublistas ordenadas
void merge(int *arr, int start, int mid, int end) {
    int n1 = mid - start + 1;
    int n2 = end - mid;
    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++)
        L[i] = arr[start + i];
    for (int i = 0; i < n2; i++)
        R[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = start;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}

// Função recursiva de Merge Sort
void mergeSort(int *arr, int start, int end) {
    if (start < end) {
        int mid = start + (end - start) / 2;
        mergeSort(arr, start, mid);
        mergeSort(arr, mid + 1, end);
        merge(arr, start, mid, end);
    }
}

// Função para ordenação paralela usando threads
void *threadedMergeSort(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int start = data->start;
    int end = data->end;
    int *arr = data->data;

    mergeSort(arr, start, end);
    pthread_exit(NULL);
}

// Função para carregar os dados dos arquivos
int *loadData(char **filenames, int numFiles, int *totalSize) {
    int *data = NULL;
    *totalSize = 0;
    
    for (int i = 0; i < numFiles; i++) {
        FILE *file = fopen(filenames[i], "r");
        if (file == NULL) {
            perror("Erro ao abrir o arquivo");
            exit(1);
        }
        
        // Conta o número de inteiros no arquivo
        int num;
        while (fscanf(file, "%d", &num) != EOF) {
            *totalSize += 1;
        }
        fclose(file);
    }

    // Aloca memória para os dados
    data = (int *)malloc(*totalSize * sizeof(int));
    if (data == NULL) {
        perror("Erro ao alocar memória");
        exit(1);
    }

    // Lê os dados novamente e os armazena
    int idx = 0;
    for (int i = 0; i < numFiles; i++) {
        FILE *file = fopen(filenames[i], "r");
        if (file == NULL) {
            perror("Erro ao abrir o arquivo");
            exit(1);
        }
        
        int num;
        while (fscanf(file, "%d", &num) != EOF) {
            data[idx++] = num;
        }
        fclose(file);
    }

    return data;
}

// Função para salvar os dados ordenados em um arquivo
void saveData(int *data, int size, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo para escrita");
        exit(1);
    }

    for (int i = 0; i < size; i++) {
        fprintf(file, "%d\n", data[i]);
    }

    fclose(file);
}

// Função para medir o tempo
double getTime(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <num_threads> <input_files>... -o <output_file>\n", argv[0]);
        exit(1);
    }

    // Leitura dos parâmetros de linha de comando
    int numThreads = atoi(argv[1]);
    if (numThreads < 1 || numThreads > MAX_THREADS) {
        fprintf(stderr, "Número de threads deve ser entre 1 e %d\n", MAX_THREADS);
        exit(1);
    }

    // Identificação dos arquivos de entrada e saída
    int numFiles = argc - 3;  // Número de arquivos de entrada
    char **inputFiles = argv + 2;  // Arquivos de entrada
    char *outputFile = argv[argc - 1];  // Arquivo de saída

    // Carregar dados dos arquivos
    int totalSize;
    int *data = loadData(inputFiles, numFiles, &totalSize);

    // Criação das threads para ordenação
    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];
    int chunkSize = totalSize / numThreads;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < numThreads; i++) {
        threadData[i].data = data;
        threadData[i].start = i * chunkSize;
        threadData[i].end = (i == numThreads - 1) ? totalSize - 1 : (i + 1) * chunkSize - 1;

        pthread_create(&threads[i], NULL, threadedMergeSort, (void *)&threadData[i]);
    }

    // Espera todas as threads terminarem
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Mescla os resultados
    for (int i = 1; i < numThreads; i++) {
        merge(data, 0, (i - 1) * chunkSize + chunkSize - 1, i * chunkSize + chunkSize - 1);
    }

    // Salva os dados ordenados no arquivo de saída
    saveData(data, totalSize, outputFile);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double totalTime = getTime(start, end);
    printf("Tempo total de execução: %.3f segundos.\n", totalTime);

    free(data);
    return 0;
}
