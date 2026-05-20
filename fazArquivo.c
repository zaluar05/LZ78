#include <stdio.h>
#include <stdlib.h>
int n = 1000000;
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr,
            "Use da seguinte maneira:\n ./fazArquivo <arquivo destino>\n");
    return EXIT_FAILURE;
  }
  FILE *saida = fopen(argv[1], "wb");
  for (int i = 0; i < n; i++) {
    if (i % 2 == 0) {
      fprintf(saida, "Esse é um número par: %d\n", i);
    } else {
      fprintf(saida, "Esse é um número impar: %d\n", i);
    }
  }
  fclose(saida);
}
