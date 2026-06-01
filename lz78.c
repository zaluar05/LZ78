#include "lz78.h"
#include "trie.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAMANHO_CABECALHO 4u
#define CAPACIDADE_INICIAL_DICIONARIO 256u
#define CAPACIDADE_INICIAL_BUFFER 256u


// entrada usada pelo dicionário durante a descompactação
typedef struct {
    uint32_t prefixo;
    unsigned char caractere;
} EntradaDicionario;


// grava um inteiro de 32 bits usando entre 1 e 5 bytes
static int escrever_varint(FILE *arquivo, uint32_t valor) {
    do {
        // os sete bits menos significativos armazenam parte do valor
        uint8_t byte = (uint8_t)(valor & 0x7Fu); // pega os 7 bits menos significativos
        valor >>= 7; // desloca 7 bits para descartar os já pegos

        // o bit mais significativo indica que ainda existem bytes seguintes
        if(valor != 0) byte |= 0x80u; // se ainda tem bits ativa o de continuação, bit de continuação | 0x80=10000000

        if(fputc(byte, arquivo) == EOF) return 0;
    } while(valor != 0);

    return 1;
}


// lê um inteiro codificado em tamanho variável
// retorna 1 para sucesso, 0 para fim normal do arquivo e -1 para erro
static int ler_varint(FILE *arquivo, uint32_t *valor) {
    if(arquivo == NULL || valor == NULL) return -1;

    uint32_t resultado = 0;
    unsigned int deslocamento = 0;

    // um uint32_t ocupa no máximo cinco bytes nesse formato
    for(int i = 0; i < 5; i++) {
        int leitura = fgetc(arquivo);

        if(leitura == EOF) {
            if(ferror(arquivo)) return -1;

            // EOF antes de começar uma nova tupla é normal
            // EOF durante um varint representa arquivo inválido
            return i == 0 ? 0 : -1;
        }

        uint8_t byte = (uint8_t)leitura;

        // no quinto byte, apenas os quatro bits inferiores podem ser usados
        if(i == 4 && (byte & 0xF0u) != 0) return -1;

        resultado |= (uint32_t)(byte & 0x7Fu) << deslocamento;

        if((byte & 0x80u) == 0) {
            *valor = resultado;
            return 1;
        }

        deslocamento += 7;
    }

    return -1;
}


// grava uma tupla no formato: prefixo em varint + caractere
static int escrever_tupla(FILE *saida, uint32_t prefixo, unsigned char caractere) {
    return escrever_varint(saida, prefixo) && fputc(caractere, saida) != EOF;
}


// aumenta o vetor usado como dicionário durante a descompactação
static int reservar_espaco_dicionario(EntradaDicionario **dicionario, size_t *capacidade, size_t tamanho_necessario) {
    if(tamanho_necessario <= *capacidade) return 1;

    size_t nova_capacidade = *capacidade == 0 ? CAPACIDADE_INICIAL_DICIONARIO : *capacidade;

    while(nova_capacidade < tamanho_necessario) {
        if(nova_capacidade > SIZE_MAX / 2) return 0;
        nova_capacidade *= 2;
    }
    if(nova_capacidade > SIZE_MAX / sizeof(EntradaDicionario)) return 0;

    EntradaDicionario *novo_dicionario = realloc(*dicionario, nova_capacidade * sizeof(EntradaDicionario));
    if(novo_dicionario == NULL) return 0;

    *dicionario = novo_dicionario;
    *capacidade = nova_capacidade;
    return 1;
}


// aumenta o vetor temporário usado para reconstruir uma sequência
static int reservar_espaco_buffer(unsigned char **buffer, size_t *capacidade, size_t tamanho_necessario) {
    if(tamanho_necessario <= *capacidade) return 1;

    size_t nova_capacidade = *capacidade == 0 ? CAPACIDADE_INICIAL_BUFFER : *capacidade;

    while(nova_capacidade < tamanho_necessario) {
        if(nova_capacidade > SIZE_MAX / 2) return 0;
        nova_capacidade *= 2;
    }

    unsigned char *novo_buffer = realloc(*buffer, nova_capacidade);
    if(novo_buffer == NULL) return 0;

    *buffer = novo_buffer;
    *capacidade = nova_capacidade;

    return 1;
}


// reconstrói e grava a sequência representada por `codigo`
static int escrever_sequencia(const EntradaDicionario *dicionario, size_t tamanho_dicionario, uint32_t codigo, FILE *saida, 
                                unsigned char **buffer, size_t *capacidade_buffer) {
    size_t comprimento = 0;

    // caracteres são encontrados assim: codigo atual -> prefixo -> prefixo do prefixo -> ... -> raiz
    // por isso, eles são inicialmente armazenados em um buffer e gravados posteriormente na ordem inversa.
    while(codigo != 0) {
        if(codigo > tamanho_dicionario || comprimento >= tamanho_dicionario) return 0;

        if(!reservar_espaco_buffer(buffer,capacidade_buffer, comprimento + 1)) return 0;

        const EntradaDicionario *entrada = &dicionario[codigo - 1];

        (*buffer)[comprimento++] = entrada->caractere;
        codigo = entrada->prefixo;
    }

    // grava os caracteres na ordem original
    for(size_t i = comprimento; i > 0; i--) {
        if(fputc((*buffer)[i - 1], saida) == EOF) return 0;
    }

    return 1;
}


int lz78_comprimir(FILE *entrada, FILE *saida) {
    if(entrada == NULL || saida == NULL) return 0;

    Trie trie;
    if(!trie_inicializar(&trie)) {
        fprintf(stderr, "Erro: falha ao inicializar a trie.\n");
        return 0;
    }

    // cabeçalho usado para identificar arquivos produzidos pelo programa
    if(fwrite("LZ78", 1, TAMANHO_CABECALHO, saida) != TAMANHO_CABECALHO) {
        fprintf(stderr, "Erro: falha ao escrever o cabeçalho.\n");
        trie_destruir(&trie);
        return 0;
    }

    uint32_t no_atual = TRIE_RAIZ;
    size_t num_tuplas = 0;
    int leitura;

    while((leitura = fgetc(entrada)) != EOF) {
        unsigned char caractere = (unsigned char)leitura;
        uint32_t indice_filho;

        int resultado = trie_buscar_filho(&trie, no_atual, caractere, &indice_filho);
        if(resultado < 0) {
            fprintf(stderr, "Erro: falha ao pesquisar na trie.\n");
            trie_destruir(&trie);
            return 0;
        }

        // se a sequencia já existe no dicionário, avança na trie
        if(resultado == 1) {
            no_atual = indice_filho;
            continue;
        }

        // sequencia nao existe no dicionário, escreve a tupla (prefixo, caractere)
        // e adiciona a nova sequencia a trie, voltando a buscar a partir da raiz
        if(!escrever_tupla(saida, no_atual, caractere)) {
            fprintf(stderr, "Erro: falha ao escrever uma tupla.\n");
            trie_destruir(&trie);
            return 0;
        }

        uint32_t novo_indice;
        if(!trie_adicionar_filho(&trie,no_atual,caractere, &novo_indice)) {
            fprintf(stderr, "Erro: falha ao adicionar uma sequência à trie.\n");
            trie_destruir(&trie);
            return 0;
        }

        no_atual = TRIE_RAIZ;
        num_tuplas++;
    }

    if(ferror(entrada)) {
        fprintf(stderr, "Erro: falha durante a leitura do arquivo de entrada.\n");
        trie_destruir(&trie);
        return 0;
    }

    // se o arquivo terminar enquanto uma sequencia já existente estiver
    // sendo percorrida, escreve de novo a tupla que originou o nó,
    // assim todas as tuplas terão prefixo e caractere.
    if(no_atual != TRIE_RAIZ) {
        const NoTrie *no = trie_obter_no(&trie, no_atual);

        if(no == NULL || !escrever_tupla(saida, no->prefixo, no->caractere)) {
            fprintf(stderr, "Erro: falha ao escrever a sequência final.\n");
            trie_destruir(&trie);
            return 0;
        }

        num_tuplas++;
    }

    trie_destruir(&trie);
    if(fflush(saida) == EOF) {
        fprintf(stderr, "Erro: falha ao concluir a escrita do arquivo.\n");
        return 0;
    }

    printf("Tuplas geradas: %zu\n", num_tuplas);
    return 1;
}


int lz78_descomprimir(FILE *entrada, FILE *saida) {
    if(entrada == NULL || saida == NULL) return 0;

    char cabecalho[TAMANHO_CABECALHO];

    if(fread(cabecalho, 1, TAMANHO_CABECALHO, entrada) != TAMANHO_CABECALHO || memcmp(cabecalho, "LZ78", TAMANHO_CABECALHO) != 0) {
        fprintf(stderr, "Erro: cabeçalho inválido.\n");
        return 0;
    }

    EntradaDicionario *dicionario = NULL;
    size_t tamanho_dicionario = 0;
    size_t capacidade_dicionario = 0;

    unsigned char *buffer = NULL;
    size_t capacidade_buffer = 0;

    while(1) {
        uint32_t prefixo;
        int resultado = ler_varint(entrada, &prefixo);
        // EOF antes do começo de uma nova tupla encerra normalmente a leitura
        if(resultado == 0) break;

        if(resultado < 0) {
            fprintf(stderr, "Erro: inteiro de tamanho variável inválido.\n");
            free(buffer);
            free(dicionario);
            return 0;
        }

        int leitura = fgetc(entrada);
        if(leitura == EOF) {
            fprintf(stderr, "Erro: tupla incompleta no arquivo compactado.\n");
            free(buffer);
            free(dicionario);
            return 0;
        }

        // prefixo 0 é a sequencia vazia, os demais devem corresponder a entradas
        // anteriores obrigatoriamente
        if(prefixo > tamanho_dicionario) {
            fprintf(stderr, "Erro: prefixo inválido no arquivo compactado.\n");
            free(buffer);
            free(dicionario);
            return 0;
        }

        unsigned char caractere = (unsigned char)leitura;
        // reconstroi o prefixo e adiciona o último caractere armazenado na tupla
        if(!escrever_sequencia(dicionario, tamanho_dicionario, prefixo, saida, &buffer, &capacidade_buffer) || 
        fputc(caractere, saida) == EOF) {
            fprintf(stderr, "Erro: falha ao reconstruir uma sequência.\n");
            free(buffer);
            free(dicionario);
            return 0;
        }

        if(tamanho_dicionario >= UINT32_MAX || !reservar_espaco_dicionario(&dicionario, &capacidade_dicionario, tamanho_dicionario + 1)) {
            fprintf(stderr, "Erro: falha ao ampliar o dicionário.\n");
            free(buffer);
            free(dicionario);
            return 0;
        }

        dicionario[tamanho_dicionario].prefixo = prefixo;
        dicionario[tamanho_dicionario].caractere = caractere;
        tamanho_dicionario++;
    }

    free(buffer);
    free(dicionario);

    if(fflush(saida) == EOF) {
        fprintf(stderr, "Erro: falha ao concluir a escrita do arquivo.\n");
        return 0;
    }

    return 1;
}