//Miguel Baptista - 86481
//Miguel Rocha - 86482

/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "leQueue.h"
#include "matrix2d.h"
#include "mplib3.h"

/* Estrutura do thread */
typedef struct {
  int id;
  int N;
  int trab;
  int iteracoes;
} args_mythread_t;


/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

DoubleMatrix2D *simul(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int linhas, int colunas, int numIteracoes, int id, int trab) {

int iter, i, j;
double value;

if(linhas < 2 || colunas < 2)
  return NULL;

DoubleMatrix2D *m, *aux, *tmp;

m = matrix;
aux = matrix_aux;

for (iter = 0; iter < numIteracoes; iter++) {
  for (i = 1; i < linhas - 1; i++) {
    for (j = 1; j < colunas - 1; j++) {
      value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) + dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
      dm2dSetEntry(aux, i, j, value);
    }
  }

    tmp = aux;
    aux = m;
    m = tmp;

    if(trab>1){ /* Faz envio de mensagens apenas se o numero de trabalhadoras for maior que 1 */
      if(id==1){ /* Caso 1 - Caso em que o id da trabalhadora e igual a 1 - envia a ultima linha para a trabalhadora seguinte */
        if(enviarMensagem(id, id+1, dm2dGetLine(m, linhas-2), colunas*sizeof(double)) == -1) {
          printf("Erro na comunicacao das mensagens\n");
          pthread_exit((void*)-1);
        }
        if(receberMensagem(id+1, id, dm2dGetLine(m, linhas-1), colunas*sizeof(double)) == -1) {
          printf("Erro na comunicacao das mensagens\n");
          pthread_exit((void*)-1);
        }
      }
      else if(id%2 != 0){ /* Caso em que id da trabalhadora e impar */
        if(id==trab){ /* Caso em que ja estamos na ultima trabalhadora - manda a sua primeira linha para a trabalhadora anterior */
          if(enviarMensagem(id, id-1, dm2dGetLine(m, 1), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
          if(receberMensagem(id-1, id, dm2dGetLine(m, 0), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
        }
        else{ /* Caso em que estamos nas trabalhadoras do meio - ocorre a troca entre trabalhadoras consecutivas */
          if(enviarMensagem(id, id-1, dm2dGetLine(m, 1), colunas*sizeof(double)) == -1) { 
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
          if(receberMensagem(id-1, id, dm2dGetLine(m, 0), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
          if(enviarMensagem(id, id+1, dm2dGetLine(m, linhas-2), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
          if(receberMensagem(id+1, id, dm2dGetLine(m, linhas-1), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
        }
      }
      else{ /* Caso em que o id da trabalhadora e par */
        if(id==trab){ /* Caso em que ja estamos na ultima trabalhadora - a trabalhadora anterior recebe a primeira linha da ultima trabalhadra */
          if(receberMensagem(id-1, id, dm2dGetLine(m, 0), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
          if(enviarMensagem(id, id-1, dm2dGetLine(m, 1), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
        }
        else{ /* Caso em que estamos nas trabalhadoras do meio - ocorre a troca entre trabalhadoras consectuvais */
          if(receberMensagem(id-1, id, dm2dGetLine(m, 0), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
          if(enviarMensagem(id, id-1, dm2dGetLine(m, 1), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }

          if(receberMensagem(id+1, id, dm2dGetLine(m, linhas-1), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
          if(enviarMensagem(id, id+1, dm2dGetLine(m, linhas-2), colunas*sizeof(double)) == -1) {
            printf("Erro na comunicacao das mensagens\n");
            pthread_exit((void*)-1);
          }
        }
      }
    }
  }
  return m;
}

/*--------------------------------------------------------------------
| Function: myThread
---------------------------------------------------------------------*/

void *myThread(void *a) {

  args_mythread_t *args = (args_mythread_t *) a;

  int id = args->id;
  int N = args->N;
  int trab = args->trab;
  int iteracoes = args->iteracoes;
  int size_slice=N/trab;

  DoubleMatrix2D *matrix_trab = dm2dNew(size_slice+2, N+2);
  DoubleMatrix2D *matrix_trab_aux = dm2dNew(size_slice+2, N+2);
  DoubleMatrix2D *result_trab;

  if(receberMensagem(0, id, dm2dGetLine(matrix_trab, 0), (size_slice+2)*(N+2)*sizeof(double)) == -1) { /* Recebe um bloco de linhas do thread principal (main) */
    printf("Erro na comunicacao das mensagens\n");
    pthread_exit((void*)-1);
  }

  dm2dCopy(matrix_trab_aux, matrix_trab);
  result_trab = simul(matrix_trab, matrix_trab_aux, size_slice+2, N+2, iteracoes, id, trab); 

  if(enviarMensagem(id, 0, dm2dGetLine(result_trab, 1), ((size_slice*id)+1)*(N+2)*sizeof(double)) == -1) { /* Envia os resultados calculodas de cada bloco da matriz para o main */
    printf("Erro na comunicacao das mensagens\n");
    pthread_exit((void*)-1);
  }

  return 0;
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name) {

  int value;

  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);  }

  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name) {

  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);  }

  return value;
}

/*--------------------------------------------------------------------
| Function: initialize_matriz
---------------------------------------------------------------------*/

void initialize_matriz(DoubleMatrix2D *matrix, int N, double tEsq, double tSup, double tDir, double tInf) {

  int i;

  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

  if(argc != 9) {   //Verificacoes do numero argumentos
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");                  //Inicializam e verificam o tipo dos argumentos
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "trab");
  int csz = parse_integer_or_exit(argv[8], "csz");
  int i, j, size_slice;

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || (trab<=0) || (N % trab != 0) || csz<0) {   //Verificacoes dos argumentos
    fprintf(stderr, "\nErro: Argumentos invalidos.\nLembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, trab tem que ser multiplo de N e =>1 e csz>=0\n\n");
    return 1;
  }

  fprintf(stderr, "\nArgumentos:\n"	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n Numero de trabalhadoras=%d Capacidade do canal=%d\n",	N, tEsq, tSup, tDir, tInf, iteracoes, trab, csz);

  DoubleMatrix2D *matrix, *matrix_aux, *result;
  args_mythread_t *slave_args;
  pthread_t       *slaves;

  slave_args = (args_mythread_t*)malloc(trab*sizeof(args_mythread_t));
  slaves     = (pthread_t*)malloc(trab*sizeof(pthread_t));
  size_slice = N/trab;
  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);

  if (matrix == NULL || matrix_aux == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }

  if(trab == 1){ /* Caso em que existe apenas uma so trabalhadora - faz apenas o simula */

    initialize_matriz(matrix, N, tEsq, tSup, tDir, tInf);

    dm2dCopy (matrix_aux, matrix);
    result = simul(matrix, matrix_aux, N+2, N+2, iteracoes, 0, trab);

    if (result == NULL) {
      printf("\nErro na simulacao.\n\n");
      return -1;
    }
    dm2dPrint(result);
    dm2dFree(result);
  }

  else{ /* Caso em que existe mais que uma trabalhadora */
    if (inicializarMPlib(csz,trab+1) == -1) { /* Inicializa a biblioteca de mensagens */
      printf("Erro ao inicializar MPLib.\n");
      return 1;
    }

    initialize_matriz(matrix, N, tEsq, tSup, tDir, tInf);

    for (i=0; i<trab; i++){ /* Ciclo que cria threads */
      slave_args[i].id = i+1;
      slave_args[i].N = N;
      slave_args[i].iteracoes = iteracoes;
      slave_args[i].trab = trab;
      pthread_create(&slaves[i], NULL, myThread, &slave_args[i]);
    }

    for(j=0; j<trab; j++){ /* Ciclo que envia blocos da matriz para as trabalhadoras */
      if(enviarMensagem(0, j+1, dm2dGetLine(matrix, j*size_slice), (size_slice+2)*(N+2)*sizeof(double)) == -1) {
        printf("Erro na comunicacao das mensagens\n");
        pthread_exit((void*)-1);
      }
    }

    for(j=0; j<trab; j++){ /* Ciclo que recebe blocos da matriz das trabalhadoras */
      if(receberMensagem(j+1, 0, dm2dGetLine(matrix, (j*size_slice)+1), (size_slice)*(N+2)*sizeof(double)) == -1) {
        printf("Erro na comunicacao das mensagens\n");
        pthread_exit((void*)-1);
      }
    }

    dm2dPrint(matrix);

    if (matrix == NULL) {
      printf("\nErro na simulacao.\n\n");
      return -1;
    }

    for(i=0; i<trab; i++){
      if(pthread_join(slaves[i], NULL) != 0) {
        printf("Erro no resultado\n");
      }
    }
  }

  dm2dFree(matrix);
  dm2dFree(matrix_aux);

  free(slave_args);
  free(slaves);

  libertarMPlib();

  return 0;
}
