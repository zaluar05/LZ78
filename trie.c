#include "trie.h"

#include <stdint.h>
#include <stdlib.h>

#define CAPACIDADE_INICIAL_NOS 256u
#define CAPACIDADE_INICIAL_FILHOS 4u

// realoca o vetor de nós quando necessário para adicionar um novo nó
static int reservar_espaco_para_no(Trie *trie) {
    if(trie->tamanho < trie->capacidade) return 1;

    size_t nova_capacidade;
    if(trie->capacidade == 0) {
        nova_capacidade = CAPACIDADE_INICIAL_NOS;
    } 
    else {
        nova_capacidade = trie->capacidade * 2;

        // caso de overflow no aumento anterior
        if(nova_capacidade < trie->capacidade) return 0;
    }

    if(nova_capacidade > SIZE_MAX / sizeof(NoTrie)) return 0;

    NoTrie *novos_nos = realloc(trie->nos, nova_capacidade * sizeof(NoTrie));
    if(novos_nos == NULL) return 0;

    trie->nos = novos_nos;
    trie->capacidade = nova_capacidade;
    return 1;
}


// aumenta o vetor de filhos de um nó se precisar
static int reservar_espaco_para_filho(NoTrie *no) {
    if(no->num_filhos < no->capacidade_filhos) return 1;

    size_t nova_capacidade;
    if(no->capacidade_filhos == 0) {
        nova_capacidade = CAPACIDADE_INICIAL_FILHOS;
    } 
    else {
        nova_capacidade = no->capacidade_filhos * 2;

        // caso de overflow no aumento anterior
        if (nova_capacidade < no->capacidade_filhos) return 0;
    }

    if(nova_capacidade > SIZE_MAX / sizeof(Aresta)) return 0;

    Aresta *novos_filhos = realloc(no->filhos, nova_capacidade * sizeof(Aresta));
    if(novos_filhos == NULL) return 0;

    no->filhos = novos_filhos;
    no->capacidade_filhos = nova_capacidade;
    return 1;
}


int trie_inicializar(Trie *trie) {
    if(trie == NULL) return 0;

    trie->nos = NULL;
    trie->tamanho = 0;
    trie->capacidade = 0;

    if(!reservar_espaco_para_no(trie)) return 0;

    // a raiz é a sequencia vazia, o campo prefixo e caracter não são usados
    trie->nos[TRIE_RAIZ].filhos = NULL;
    trie->nos[TRIE_RAIZ].num_filhos = 0;
    trie->nos[TRIE_RAIZ].capacidade_filhos = 0;
    trie->nos[TRIE_RAIZ].prefixo = TRIE_RAIZ;
    trie->nos[TRIE_RAIZ].caractere = 0;

    trie->tamanho = 1;
    return 1;
}


void trie_destruir(Trie *trie) {
    if(trie == NULL) return;

    for(size_t i = 0; i < trie->tamanho; i++) {
        free(trie->nos[i].filhos);
    }

    free(trie->nos);
    trie->nos = NULL;
    trie->tamanho = 0;
    trie->capacidade = 0;
}


int trie_buscar_filho(Trie *trie, uint32_t indice_pai, unsigned char caractere, uint32_t *indice_filho) {
    if(trie == NULL || indice_filho == NULL || indice_pai >= trie->tamanho) return -1;

    NoTrie *pai = &trie->nos[indice_pai];

    // busca linear pelos filhos já q a implementação com vetor dinâmico permite economizar no consumo
    // de memória, mas não permite busca em O(1)
    for(size_t i = 0; i < pai->num_filhos; i++) {
        if(pai->filhos[i].caractere == caractere) {
            *indice_filho = pai->filhos[i].indice_filho;
            return 1;
        }
    }
    return 0;
}


int trie_adicionar_filho(Trie *trie, uint32_t indice_pai, unsigned char caractere, uint32_t *indice_filho) {
    if(trie == NULL || indice_filho == NULL || indice_pai >= trie->tamanho) return 0;

    // codigo da sequencia é armazenado em unsigned int de 32 bits
    if(trie->tamanho > UINT32_MAX) return 0;

    uint32_t filho_existente;
    int resultado_busca = trie_buscar_filho(trie, indice_pai, caractere, &filho_existente);
    if(resultado_busca != 0) return 0;

    if(!reservar_espaco_para_no(trie)) return 0;

    NoTrie *pai = &trie->nos[indice_pai];
    if(!reservar_espaco_para_filho(pai)) return 0;

    uint32_t novo_indice = (uint32_t)trie->tamanho;

    trie->nos[novo_indice].filhos = NULL;
    trie->nos[novo_indice].num_filhos = 0;
    trie->nos[novo_indice].capacidade_filhos = 0;
    trie->nos[novo_indice].prefixo = indice_pai;
    trie->nos[novo_indice].caractere = caractere;

    pai->filhos[pai->num_filhos].indice_filho = novo_indice;
    pai->filhos[pai->num_filhos].caractere = caractere;
    pai->num_filhos++;

    trie->tamanho++;
    *indice_filho = novo_indice;
    return 1;
}


NoTrie* trie_obter_no(Trie *trie, uint32_t indice_no) {
    if(trie == NULL || indice_no >= trie->tamanho) return NULL;

    return &trie->nos[indice_no];
}