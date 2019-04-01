/*
// Projeto SO - exercicio 3, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

/* 86481 - Miguel Baptista
   84682 - Miguel Rocha
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"

/*--------------------------------------------------------------------
| Thread Structure
---------------------------------------------------------------------*/
typedef struct {
  int id;
  int N;
  int trab;
  int iteracoes;
  double maxD;
} args_mythread_t;

/*--------------------------------------------------------------------
| Global Variables
---------------------------------------------------------------------*/
DoubleMatrix2D *matrix;
DoubleMatrix2D *matrix_aux;

int contador_trab = 0; /* Variavel que e usada para verificar o numero de trabalhadora a espera na barreira */
int verifica_trab = 1; /* Variavel que verifica se se pode avancar */
int verifica_delta; /* Variavel que verifica se o programa pode terminar */
double delta_value;

pthread_mutex_t single_mutex;
pthread_mutex_t mutex_maxD;
pthread_cond_t wait_for_all;

/*--------------------------------------------------------------------
| Function: simul (trabalhadora)
---------------------------------------------------------------------*/

void *simul(void *a) {

  args_mythread_t *args = (args_mythread_t *) a;

  int id = args->id;
  int N = args->N;
  int trab = args->trab;
  int iteracoes = args->iteracoes;
  double maxD = args->maxD;

  int iter, i, j;
  int size_slice=N/trab;
  int verifica_trab_local = 1;
  double value;
  double compare_value;
  double delta_value_local;

  DoubleMatrix2D *tmp;

  for (iter = 0; iter < iteracoes; iter++) {
    for (i = size_slice*id + 1; i < (size_slice*(id+1))+1 ; i++) {
      for (j = 1; j < (N+2) - 1; j++) {
        value = ( dm2dGetEntry(matrix, i-1, j) + dm2dGetEntry(matrix, i+1, j) + dm2dGetEntry(matrix, i, j-1) + dm2dGetEntry(matrix, i, j+1) ) / 4.0;
        dm2dSetEntry(matrix_aux, i, j, value);
        compare_value = dm2dGetEntry(matrix_aux, i, j) - dm2dGetEntry(matrix,i,j); /* Faz a diferenca entre os valores da nova matriz e os valores da antiga matriz */
        if(compare_value > delta_value_local) /* Arranja a maior diferenca da matriz possivel para cada local */
          delta_value_local = compare_value;
      }
    }

    if(pthread_mutex_lock(&single_mutex) != 0) {
      fprintf(stderr, "\nErro ao bloquear mutex\n");
      return NULL;
    }

    verifica_delta = 1; /* Coloca a flag do maxD a 1 */

    if(delta_value_local > delta_value) /* Coloca o maior valor das trabalhadoras como o global */
      delta_value = delta_value_local;

    verifica_trab_local = 1 - verifica_trab_local; /* Ativa a flag local */
    contador_trab++; /* Passou aqui uma trabalhadora */

    if(contador_trab == trab){ /* Ultima trabalhadora */
      tmp = matrix_aux;
      matrix_aux = matrix;
      matrix = tmp;
      contador_trab=0; /* Faz reset ao contador */
      if(delta_value <= maxD){ /* Se a condicao do maxD for atingida, entao atualiza a flag para 0 */
        verifica_delta = 1 - verifica_delta;
      }
      if(pthread_cond_broadcast(&wait_for_all) != 0) {
        fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
        return NULL;
      }
      verifica_trab = verifica_trab_local; /* Notifica todas as trabalhadoras */
      delta_value=0; /* Reinicia o valor do delta se ele for maior ainda que maxD */
    }
    else{
      while(verifica_trab_local != verifica_trab){ /* Nao e a ultima trabalhadora */
        if(pthread_cond_wait(&wait_for_all, &single_mutex) != 0) {
            fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
            return NULL;
          }
      }
    }

    delta_value_local=0; /* Reinicia olocal apos cada iteracao */

    if(pthread_mutex_unlock(&single_mutex) != 0) {
      fprintf(stderr, "\nErro ao desbloquear mutex\n");
      return NULL;
    }

    if(verifica_delta==0){ /* Verifica se a flag do delta e 0. Se for, termina as iteracoes */
      pthread_exit((void*)-1);
      return 0;
    }

  }

  return 0;

}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name){
  int value;

  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name){
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
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

  if(argc != 9) { /* Verificacoes do numero argumentos */
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab maxD\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N"); /* Inicializam e verificam o tipo dos argumentos */
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "trab");
  double maxD = parse_double_or_exit(argv[8], "maxD");
  int i;

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || (trab<=0) || (N % trab != 0) || maxD < 0) {   /* Verificacoes dos argumentos */
    fprintf(stderr, "\nErro: Argumentos invalidos.\nLembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, trab tem que ser multiplo de N e =>1, maxD>=0\n\n");
    return 1;
  }

  fprintf(stderr, "\nArgumentos:\n"	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n trab=%d maxD=%f\n",	N, tEsq, tSup, tDir, tInf, iteracoes, trab, maxD);

  args_mythread_t *slave_args;
  pthread_t       *slaves;

  slave_args = (args_mythread_t*)malloc(trab*sizeof(args_mythread_t));
  slaves     = (pthread_t*)malloc(trab*sizeof(pthread_t));

  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);

  initialize_matriz(matrix, N, tEsq, tSup, tDir, tInf);
  dm2dCopy (matrix_aux, matrix);

  if (matrix == NULL || matrix_aux == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }

    if(pthread_mutex_init(&single_mutex, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar mutex\n");
        return -1;
    }

    if(pthread_mutex_init(&mutex_maxD, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar mutex\n");
        return -1;
    }

    if(pthread_cond_init(&wait_for_all, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar variável de condição\n");
        return -1;
    }

    for (i=0; i<trab; i++){ /* Ciclo que cria threads */
      slave_args[i].id = i;
      slave_args[i].N = N;
      slave_args[i].iteracoes = iteracoes;
      slave_args[i].trab = trab;
      slave_args[i].maxD = maxD;
      if(pthread_create(&slaves[i], NULL, simul, &slave_args[i]) != 0){
        fprintf(stderr, "\nErro ao criar uma trabalhadora\n");
        return -1;
      }
    }

    for(i=0; i<trab; i++){ /* Ciclo que termina as threads */
      if(pthread_join(slaves[i], NULL) != 0) {
        fprintf(stderr, "\nErro no resultado\n");
        return -1;
      }
    }

    if(pthread_cond_destroy(&wait_for_all) != 0) {
      fprintf(stderr, "\nErro ao destruir variável de condição\n");
      exit(EXIT_FAILURE);
    }

    if(pthread_mutex_destroy(&single_mutex) != 0) {
      fprintf(stderr, "\nErro ao destruir mutex\n");
      exit(EXIT_FAILURE);
    }

    if(pthread_mutex_destroy(&mutex_maxD) != 0) {
      fprintf(stderr, "\nErro ao destruir mutex\n");
      exit(EXIT_FAILURE);
    }

    if (matrix == NULL) {
      fprintf(stderr, "\nErro na simulacao.\n\n");
      return -1;
    }

  dm2dPrint(matrix);

  free(slave_args);
  free(slaves);
  dm2dFree(matrix);
  dm2dFree(matrix_aux);

  return 0;
}
