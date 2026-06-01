#ifndef LZ78_H
#define LZ78_H

#include <stdio.h>

// comprime os bytes lidos de `entrada` e grava o resultado em `saida`
// retorna 1 para sucesso e 0 para falha
int lz78_comprimir(FILE *entrada, FILE *saida);

// descomprime os bytes lidos de `entrada` e grava o resultado em `saida`
// retorna 1 para sucesso e 0 para falha
int lz78_descomprimir(FILE *entrada, FILE *saida);

#endif