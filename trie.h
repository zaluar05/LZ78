#ifndef TRIE_H
#define TRIE_H

#include <stddef.h>
#include <stdint.h>

// raiz da trie e o prefixo do vazio será representado pelo índice 0
#define TRIE_RAIZ 0u


// transição entre um nó e algum de seus filhos
typedef struct {
    uint32_t indice_filho;
    unsigned char caractere;
} Aresta;


// sequencia armazenada no dicionário. a sequencia completa pode ser reconstruída
// seguindo os prefixos até a raiz da trie, concatenando os caracteres encontrados
typedef struct {
    Aresta *filhos;
    size_t num_filhos;
    size_t capacidade_filhos;
    uint32_t prefixo;
    unsigned char caractere;
} NoTrie;


// a trie é implementada usando um vetor de nós. o índice de cada nó no vetor
// corresponde tambem ao seu código no dicionário do LZ78
typedef struct {
    NoTrie *nos;
    size_t tamanho;
    size_t capacidade;
} Trie;



// inicializa uma trie contendo apenas a raiz
// retorna 1 para sucesso e 0 para falha
int trie_inicializar(Trie *trie);

// libera a memória usada pela trie
void trie_destruir(Trie *trie);

// busca o filho de `indice_pai` pelo byte `caractere` procurado. `indice_filho` recebe o indice do filho encontrado.
// retorna 1 se encontrou, 0 se não encontrou, ou -1 se os argumentos são inválidos.
int trie_buscar_filho(Trie *trie, uint32_t indice_pai, unsigned char caractere, uint32_t *indice_filho);

// cria um novo nó a partir de `indice_pai`. a sequencia representada no novo nó sera pai + `caractere`.
// retorna 1 para sucesso e 0 para fracasso.
int trie_adicionar_filho(Trie *trie, uint32_t indice_pai, unsigned char caractere, uint32_t *indice_filho);

// retorna o ponteiro para leitura do nó `indice_no`.
// retorna o endereço do nó ou NULL caso seja inválido.
NoTrie* trie_obter_no(Trie *trie, uint32_t indice_no);

#endif