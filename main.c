#include "lz78.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// exibe a forma correta de executar o programa
static void mostrar_uso(const char *programa) {
    fprintf(stderr, "Uso: %s <modo> <arquivo_entrada> <arquivo_saida>\n", programa);
    fprintf(stderr, "Modos:\n");
    fprintf(stderr, "  -c  Comprimir\n");
    fprintf(stderr, "  -d  Descomprimir\n");
    fprintf(stderr, "\nExemplos:\n");
    fprintf(stderr,"  %s -c entrada.txt compactado.lz78\n", programa);
    fprintf(stderr, "  %s -d compactado.lz78 recuperado.txt\n", programa);
}


int main(int argc, char *argv[]) {
    if(argc != 4) {
        mostrar_uso(argv[0]);
        return EXIT_FAILURE;
    }

    const char *modo = argv[1];
    const char *arquivo_entrada = argv[2];
    const char *arquivo_saida = argv[3];

    if(strcmp(modo, "-c") != 0 && strcmp(modo, "-d") != 0) {
        fprintf(stderr, "Erro: modo inválido: '%s'.\n", modo);
        mostrar_uso(argv[0]);
        return EXIT_FAILURE;
    }

    if(strcmp(arquivo_entrada, arquivo_saida) == 0) {
        fprintf(stderr, "Erro: os arquivos de entrada e saída devem ser diferentes.\n");
        return EXIT_FAILURE;
    }

    FILE *entrada = fopen(arquivo_entrada, "rb");
    if(entrada == NULL) {
        fprintf(stderr, "Erro: não foi possível abrir o arquivo de entrada '%s'.\n", arquivo_entrada);
        return EXIT_FAILURE;
    }

    FILE *saida = fopen(arquivo_saida, "wb");
    if(saida == NULL) {
        fprintf(stderr, "Erro: não foi possível criar o arquivo de saída '%s'.\n", arquivo_saida);
        fclose(entrada);
        return EXIT_FAILURE;
    }

    int sucesso;
    if(strcmp(modo, "-c") == 0) {
        printf("Comprimindo '%s'...\n", arquivo_entrada);
        sucesso = lz78_comprimir(entrada, saida);
    }
    else {
        printf("Descomprimindo '%s'...\n", arquivo_entrada);
        sucesso = lz78_descomprimir(entrada, saida);
    }

    fclose(entrada);

    if(fclose(saida) == EOF) {
        fprintf(stderr, "Erro: falha ao concluir a escrita do arquivo de saída.\n");
        sucesso = 0;
    }

    if(!sucesso) {
        remove(arquivo_saida);
        return EXIT_FAILURE;
    }

    printf("Operação concluída com sucesso.\n");
    return EXIT_SUCCESS;
}