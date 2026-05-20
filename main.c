#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int indice_filho;
  unsigned char caracter;
} Aresta;

typedef struct {
  Aresta *filhos; // vetor de filhos
  int tamanho_do_vetor_de_filhos;

  int codigo;
  int num_filhos;
} NoTrie;

typedef struct {
  NoTrie *nos;    // vetor de nos
  int tamanho;    // tamanho do vetor
  int capacidade; // o malloc está usando isso como referencia. Verificar se é
                  // necessário
} Trie;

typedef struct {
  int prefixo;
  unsigned char caracter;
} EntradaDicionario;

// inicializa a trie com capacidade = 256 por utilizar 8 bits
// tamanho = 1 pois apenas existe a raiz
static inline void inicializa_trie(Trie *trie) {
  trie->capacidade = 256;
  trie->tamanho = 1;
  trie->nos = malloc(sizeof(NoTrie) * trie->capacidade);

  // indice da raiz = -1 com 0 filhos
  // vetor de filhos = NULL e com tamanho = 0
  trie->nos[0].codigo = -1;
  trie->nos[0].num_filhos = 0;
  trie->nos[0].filhos = NULL;
  trie->nos[0].tamanho_do_vetor_de_filhos = 0;
}

static inline int trie_criar_no(Trie *trie, int codigo) {
  if (trie->tamanho >= trie->capacidade) {
    trie->capacidade *= 2;
    trie->nos = realloc(trie->nos, sizeof(NoTrie) * trie->capacidade);
  }

  int indice = trie->tamanho++;
  trie->nos[indice].codigo = codigo;
  trie->nos[indice].num_filhos = 0;
  trie->nos[indice].filhos = NULL;
  trie->nos[indice].tamanho_do_vetor_de_filhos = 0;
  return indice;
}

static inline int trie_buscar_filho(Trie *trie, int indice_no,
                                    unsigned char c) {
  NoTrie *no = &trie->nos[indice_no];

  // mantive a busca linear pelo bem da memória
  // podia ter mudado a estrutura para buscar o caracter
  // em O(1) se fizesse um vetor de 256
  // em cada ponteiro de filhos

  for (int i = 0; i < no->num_filhos; i++) {
    if (no->filhos[i].caracter == c) {
      return no->filhos[i].indice_filho;
    }
  }
  return -1;
}

static inline void trie_adicionar_filho(Trie *trie, int indice_pai,
                                        unsigned char c, int indice_filho) {
  NoTrie *pai = &trie->nos[indice_pai];

  if (pai->num_filhos >= pai->tamanho_do_vetor_de_filhos) {
    pai->tamanho_do_vetor_de_filhos = pai->tamanho_do_vetor_de_filhos == 0
                                          ? 4
                                          : pai->tamanho_do_vetor_de_filhos * 2;

    pai->filhos =
        realloc(pai->filhos, pai->tamanho_do_vetor_de_filhos * sizeof(Aresta));
  }
  pai->filhos[pai->num_filhos].caracter = c;
  pai->filhos[pai->num_filhos].indice_filho = indice_filho;

  pai->num_filhos++;
}
static inline void trie_destruir(Trie *trie) {
  for (int i = 0; i < trie->tamanho; i++) {
    free(trie->nos[i].filhos);
  }

  free(trie->nos);
}
static inline void escrever_varint(FILE *f, uint32_t valor) {
  // Codificação de tamanho variável (1-5 bytes)
  do {
    uint8_t byte = valor & 0x7F; // pega os 7 bits menos significativos
    valor >>= 7;                 // desloca 7 bits para descartar os já pegos
    if (valor != 0) {            // se ainda tem bits ativa o de continuação
      byte |= 0x80;              // bit de continuação | 0x80=10000000
    }
    fputc(byte, f);
  } while (valor != 0);
}

// Lê um inteiro de tamanho variável
static inline uint32_t ler_varint(FILE *f) {
  uint32_t valor = 0;
  int shift = 0;
  uint8_t byte;

  do {
    byte = fgetc(f);
    valor |= (uint32_t)(byte & 0x7F) << shift;
    shift += 7;
  } while (byte & 0x80);

  return valor;
}

void lz78_comprimir(FILE *entrada, FILE *saida) {
  Trie trie;
  inicializa_trie(&trie);

  fwrite("LZ78", 1, 4, saida);

  int no_atual = 0;
  int proximo_codigo = 1;
  int c;
  int num_tuplas = 0;

  // buffer para acumular tuplas
  typedef struct Tupla {
    uint32_t codigo;
    uint8_t caracter;
  } Tupla;

  Tupla *buffer = malloc(sizeof(Tupla) * 10000);
  int buffer_pos = 0;

  while ((c = fgetc(entrada)) != EOF) {
    unsigned char uc = (unsigned char)c;
    int filho = trie_buscar_filho(&trie, no_atual, uc);

    if (filho != -1) {
      no_atual = filho;
    } else {
      // Adiciona tupla ao buffer
      int codigo_saida = (no_atual == 0) ? 0 : trie.nos[no_atual].codigo;
      buffer[buffer_pos].codigo = codigo_saida;
      buffer[buffer_pos].caracter = uc;
      buffer_pos++;
      num_tuplas++;

      if (buffer_pos >= 10000) {
        for (int i = 0; i < buffer_pos; i++) {
          escrever_varint(saida, buffer[i].codigo);
          fputc(buffer[i].caracter, saida);
        }
        buffer_pos = 0;
      }

      // cria novo no
      int novo_no = trie_criar_no(&trie, proximo_codigo++);
      trie_adicionar_filho(&trie, no_atual, uc, novo_no);
      no_atual = 0;
    }
  }
  // se tiver sequencia final
  if (no_atual != 0) {
    buffer[buffer_pos].codigo = trie.nos[no_atual].codigo;
    buffer[buffer_pos].caracter = 0; // marca de fim
    buffer_pos++;
    num_tuplas++;
  }

  for (int i = 0; i < buffer_pos; i++) {
    escrever_varint(saida, buffer[i].codigo);
    if (buffer[i].caracter != 0 || i < buffer_pos - 1) {
      fputc(buffer[i].caracter, saida);
    }
  }

  free(buffer);
  trie_destruir(&trie);

  fprintf(stdout, "Tuplas geradas: %d\n", num_tuplas);
}

static inline void imprimir_sequencia(EntradaDicionario *dict, int codigo,
                                      FILE *saida) {
  // manda os caracteres de baixo para cima
  if (codigo == 0) {
    return;
  }
  imprimir_sequencia(dict, dict[codigo - 1].prefixo, saida);
  fputc(dict[codigo - 1].caracter, saida);
}

void lz78_descomprimir(FILE *entrada, FILE *saida) {
  // verifica cabeçalho
  char magic[4];
  if (fread(magic, 1, 4, entrada) != 4 || memcmp(magic, "LZ78", 4) != 0) {
    fprintf(stderr, "erro no cabeçalho do arquivo\n");
    return;
  }

  int capacidade = 1000;
  int tamanho = 0;
  EntradaDicionario *dict = malloc(sizeof(EntradaDicionario) * capacidade);

  while (!feof(entrada)) {
    uint32_t codigo = ler_varint(entrada);

    if (feof(entrada))
      break;

    int c = fgetc(entrada);
    if (c == EOF) {
      imprimir_sequencia(dict, codigo, saida);
      break;
    }

    unsigned char caracter = (unsigned char)c;

    imprimir_sequencia(dict, codigo, saida);
    fputc(caracter, saida);

    // dobra a capacidade se precisar
    if (tamanho >= capacidade) {
      capacidade *= 2;
      dict = realloc(dict, sizeof(EntradaDicionario) * capacidade);
    }

    dict[tamanho].prefixo = codigo;
    dict[tamanho].caracter = caracter;
    tamanho++;
  }

  free(dict);
}

void mostrar_uso(const char *programa) {
  printf("Uso: %s <modo> <arquivo_entrada> <arquivo_saida>\n", programa);
  printf("Modos:\n");
  printf("  -c  Comprimir\n");
  printf("  -d  Descomprimir\n");
  printf("\nExemplos:\n");
  printf("  %s -c entrada.txt saida.lz78\n", programa);
  printf("  %s -d saida.lz78 recuperado.txt\n", programa);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    mostrar_uso(argv[0]);
    return EXIT_FAILURE;
  }

  const char *modo = argv[1];
  const char *arquivo_entrada = argv[2];
  const char *arquivo_saida = argv[3];

  FILE *entrada = fopen(arquivo_entrada, "rb");
  if (!entrada) {
    fprintf(stderr, "não foi possível abrir '%s'\n", arquivo_entrada);
    return EXIT_FAILURE;
  }

  FILE *saida = fopen(arquivo_saida, "wb");
  if (!saida) {
    fprintf(stderr, "Erro: não foi possível criar '%s'\n", arquivo_saida);
    fclose(entrada);
    return EXIT_FAILURE;
  }

  if (strcmp(modo, "-c") == 0) {
    printf("Comprimindo '%s'\n", arquivo_entrada);
    lz78_comprimir(entrada, saida);
    printf("pronto\n");
  } else if (strcmp(modo, "-d") == 0) {
    printf("Descomprimindo '%s'\n", arquivo_entrada);
    lz78_descomprimir(entrada, saida);
    printf("pronto\n");
  } else {
    fprintf(stderr, "modo errado - '%s'\n", modo);
    mostrar_uso(argv[0]);
  }
  fclose(entrada);
  fclose(saida);
  return EXIT_SUCCESS;
}
