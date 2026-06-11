# Implementação do algoritmo de compressão LZ78 em C

## 1. Descrição

Este projeto é uma implementação do algoritmo de compressão LZ78, desenvolvido em linguagem C.

O algoritmo LZ78 aqui implementado apresenta alguns pontos bastante interessantes como o uso de uma arvore de prefixos (trie) e o conceito de variable-length int

Uma maneira lógica de listar os passos na hora da compressão é:
- entrada → [trie: busca] → tuplas (varint + byte) → arquivo

passos da descompressão:
- arquivo → tuplas (varint + byte) → [vetor + buffer] → saída


## 2. Funcionalidades Implementadas

O programa implementa as seguintes operações:
  1. Compressão de um arquivo passado como argumento.
  2. Descompressão de um arquivo que foi compactado por este programa.

## 3. Complexidade

Esta seção detalha os requisitos de tempo (quão rápido as operações são executadas) e de espaço (quanta memória a estrutura utiliza) da implementação.

### Complexidade de Tempo (Time Complexity)

#### Compressão
| Operação | Complexidade | Justificativa |
| ------------- | ------------- | ------------- |
| **Busca na trie (por nó)**  | *O(k)*  | Busca linear entre os filhos de cada nó, onde k é o número de filhos  |
| **Compressão total**  | *O(n · k)*  | n bytes de entrada, cada byte faz uma busca linear  |
| **Escrita de varint**  | *O(1)*  | No máximo 5 bytes por tupla  |

Na prática, k tende a ser pequeno (máximo 256), então se comporta quase como O(n).

#### Descompressão
| Operação | Complexidade | Justificativa |
| ------------- | ------------- | ------------- |
| **Leitura de varint**  | *O(1)*  | No máximo 5 iterações  |
| **Reconstrução de sequência**  | *O(L)*  | L é o comprimento da sequência, percorre prefixos até chegar na raiz  |
| **Descompressão total**  | *O(m * L)*  | m tuplas, cada uma reconstrói uma sequência  |

### Complexidade de Espaço (Space Complexity)

| Estrutura | Complexidade | Justificativa |
| ------------- | ------------- | ------------- |
| **Trie (compressão)**  | *O(n)*  | Cada byte do arquivo pode gerar no máximo um novo nó  |
| **Dicionário (descompressão)**  | *O(m)*  | Um **EntradaDicionario** por tupla lida  |
| **Buffer de reconstrução**  | *O(L)*  | Tamanho da maior sequência reconstruída  |

### Resultados
| Arquivo  | Original | Compactado | Redução |
| ------------- | ------------- | ------------- | ------------- |
| **Pequeno** | *21 B*  | *30 B*  | *-42.86%* |
| **Texto Comum**  | *8.339 B*  | *5.692 B*  | *31.79%*  |
| **Repetitivo**  | *127.500 B*  | *20.016 B*  | *84.30%*  |
| **Maior** | *31.888.890 B*  | *4.096.067 B*  | *87.16%*  |

## 4. Exemplo de Uso
### Compilação
```
make
```
### Execução
```
./lz78 -c arquivo_a_ser_compactado.txt arquivo_destino
./lz78 -d arquivo_a_ser_descompactado arquivo_destino.txt
```

## 5. Referências

  * https://pt.wikipedia.org/wiki/LZ78
  * https://cs.stanford.edu/people/eroberts/courses/soco/projects/data-compression/lossless/lz78/concept.htm
  * https://hackernoon.com/how-lz78-compression-algorithm-works-x7103tlm
