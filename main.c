/*
// Projeto SO - exercicio 3
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "matrix2d.h"
#include "util.h"

/*--------------------------------------------------------------------
| Type: thread_info
| Description: Estrutura com Informacao para Trabalhadoras
---------------------------------------------------------------------*/

typedef struct {
  int    id;
  int    N;
  int    iter;
  int    trab;
  int    tam_fatia;
  double maxD;
  char* fichS;
  int periodoS;
} thread_info;

/*--------------------------------------------------------------------
| Type: doubleBarrierWithMax
| Description: Barreira dupla com variavel de max-reduction
---------------------------------------------------------------------*/

typedef struct {
  int             total_nodes;
  int             pending[2];
  double          maxdelta[2];
  int             iteracoes_concluidas;
  pthread_mutex_t mutex;
  pthread_cond_t  wait[2];
} DualBarrierWithMax;

/*--------------------------------------------------------------------
| Global variables
---------------------------------------------------------------------*/

DoubleMatrix2D     *matrix_copies[2];
DualBarrierWithMax *dual_barrier;
FILE *filename;
pid_t pidFilho;
double              maxD;
int periodoS;
int status; /* Variavel global associada ao estado do pid filho */
int alarm_flag = 1;
int ctrlc_flag = 1;

/*--------------------------------------------------------------------
| Function: dualBarrierInit
| Description: Inicializa uma barreira dupla
---------------------------------------------------------------------*/

DualBarrierWithMax *dualBarrierInit(int ntasks) {
  DualBarrierWithMax *b;
  b = (DualBarrierWithMax*) malloc (sizeof(DualBarrierWithMax));
  if (b == NULL) return NULL;

  b->total_nodes = ntasks;
  b->pending[0]  = ntasks;
  b->pending[1]  = ntasks;
  b->maxdelta[0] = 0;
  b->maxdelta[1] = 0;
  b->iteracoes_concluidas = 0;

  if (pthread_mutex_init(&(b->mutex), NULL) != 0) {
    fprintf(stderr, "\nErro a inicializar mutex\n");
    exit(1);
  }
  if (pthread_cond_init(&(b->wait[0]), NULL) != 0) {
    fprintf(stderr, "\nErro a inicializar variável de condição\n");
    exit(1);
  }
  if (pthread_cond_init(&(b->wait[1]), NULL) != 0) {
    fprintf(stderr, "\nErro a inicializar variável de condição\n");
    exit(1);
  }
  return b;
}

/*--------------------------------------------------------------------
| Function: dualBarrierFree
| Description: Liberta os recursos de uma barreira dupla
---------------------------------------------------------------------*/

void dualBarrierFree(DualBarrierWithMax* b) {
  if (pthread_mutex_destroy(&(b->mutex)) != 0) {
    fprintf(stderr, "\nErro a destruir mutex\n");
    exit(1);
  }
  if (pthread_cond_destroy(&(b->wait[0])) != 0) {
    fprintf(stderr, "\nErro a destruir variável de condição\n");
    exit(1);
  }
  if (pthread_cond_destroy(&(b->wait[1])) != 0) {
    fprintf(stderr, "\nErro a destruir variável de condição\n");
    exit(1);
  }
  free(b);
}

/*--------------------------------------------------------------------
| Function: dualBarrierWait
| Description: Ao chamar esta funcao, a tarefa fica bloqueada ate que
|              o numero 'ntasks' de tarefas necessario tenham chamado
|              esta funcao, especificado ao ininializar a barreira em
|              dualBarrierInit(ntasks). Esta funcao tambem calcula o
|              delta maximo entre todas as threads e devolve o
|              resultado no valor de retorno
---------------------------------------------------------------------*/

double dualBarrierWait (DualBarrierWithMax* b, int current, double localmax, char* fichS, int periodoS) {
  int next = 1 - current;
  int cont_Filhos = 0;
  char* fichS_aux = malloc(sizeof(char)*(strlen(fichS)+2));
  FILE* filename_aux;
  if (pthread_mutex_lock(&(b->mutex)) != 0) {
    fprintf(stderr, "\nErro a bloquear mutex\n");
    exit(1);
  }
  // decrementar contador de tarefas restantes
  b->pending[current]--;
  // actualizar valor maxDelta entre todas as threads
  if (b->maxdelta[current]<localmax)
    b->maxdelta[current]=localmax;
  // verificar se sou a ultima tarefa
  if (b->pending[current]==0) {
    // sim -- inicializar proxima barreira e libertar threads
    if(alarm_flag==0 && next!=0){
      if(cont_Filhos == 0){ /* Verifica se o progrma esta na primeira instancia do periodoS */
        if((pidFilho = fork()) == -1){
          die("\nErro a criar processo filho\n");
        }
        cont_Filhos++;
      }
      else if(waitpid(pidFilho, &status, WNOHANG) > 0){ /* Verifica se o filho ainda esta ainda a escrever no ficheiro */
        if((pidFilho = fork()) == -1){
          die("\nErro a criar processo filho\n");
        }
        cont_Filhos++;
      }
      if(pidFilho == 0){ /* Codigo do filho */
        strcpy(fichS_aux, fichS);
        strcat(fichS_aux, "~");
        filename_aux = fopen(fichS_aux, "w");
        dm2dPrintToFile(matrix_copies[next], filename_aux);
        if(fclose(filename) == EOF)
          fprintf(stderr, "\nErro a dar close ao ficheiro\n");
        if(rename(fichS_aux, fichS) == -1)
          fprintf(stderr, "\nErro a mudar o nome do ficheiro\n");
        exit(0);
      }
    }

    if(ctrlc_flag == 0){ /* Se o utilizador clicar no ctrl+c, isto acontece */
      next = 0;
      wait(&status);
      filename = fopen(fichS, "w");
      dm2dPrintToFile(matrix_copies[next], filename);
      if(fclose(filename) == EOF)
        fprintf(stderr, "\nErro a dar close ao ficheiro\n");
      exit(0);
    }

    b->iteracoes_concluidas++;
    b->pending[next]  = b->total_nodes;
    b->maxdelta[next] = 0;
    if (pthread_cond_broadcast(&(b->wait[current])) != 0) {
      fprintf(stderr, "\nErro a assinalar todos em variável de condição\n");
      exit(1);
    }
  }
  else {
    // nao -- esperar pelas outras tarefas
    while (b->pending[current]>0) {
      if (pthread_cond_wait(&(b->wait[current]), &(b->mutex)) != 0) {
        fprintf(stderr, "\nErro a esperar em variável de condição\n");
        exit(1);
      }
    }
  }
  double maxdelta = b->maxdelta[current];
  if (pthread_mutex_unlock(&(b->mutex)) != 0) {
    fprintf(stderr, "\nErro a desbloquear mutex\n");
    exit(1);
  }
  free(fichS_aux);
  return maxdelta;
}

/*--------------------------------------------------------------------
| Function: tarefa_trabalhadora
| Description: Funcao executada por cada tarefa trabalhadora.
|              Recebe como argumento uma estrutura do tipo thread_info
---------------------------------------------------------------------*/

void *tarefa_trabalhadora(void *args) {
  thread_info *tinfo = (thread_info *) args;
  int tam_fatia = tinfo->tam_fatia;
  int my_base = tinfo->id * tam_fatia;
  double global_delta = INFINITY;
  int periodoS = tinfo->periodoS;
  char* fichS = tinfo -> fichS;
  int iter = 0;

  do {
    int atual = iter % 2;
    int prox = 1 - iter % 2;
    double max_delta = 0;

    // Calcular Pontos Internos
    for (int i = my_base; i < my_base + tinfo->tam_fatia; i++) {
      for (int j = 0; j < tinfo->N; j++) {
        double val = (dm2dGetEntry(matrix_copies[atual], i,   j+1) +
                      dm2dGetEntry(matrix_copies[atual], i+2, j+1) +
                      dm2dGetEntry(matrix_copies[atual], i+1, j) +
                      dm2dGetEntry(matrix_copies[atual], i+1, j+2))/4;
        // calcular delta
        double delta = val - dm2dGetEntry(matrix_copies[atual], i+1, j+1);
        if (delta > max_delta) {
          max_delta = delta;
        }
        dm2dSetEntry(matrix_copies[prox], i+1, j+1, val);
      }
    }
    // barreira de sincronizacao; calcular delta global
    global_delta = dualBarrierWait(dual_barrier, atual, max_delta, fichS, periodoS);
  } while (++iter < tinfo->iter && global_delta >= tinfo->maxD);

  return 0;
}

/*-------------------------------------------------------------------
| Function: alarm_handler
| Description: Trata da interrupcao do temporizador/alarme
-------------------------------------------------------------------*/
void alarm_handler(int signum){
  if(signal(SIGALRM, SIG_IGN) == SIG_ERR)
    fprintf(stderr, "\nErro no signal do alarme\n");
  alarm_flag = 0;
  alarm(periodoS);
  if(signal(SIGALRM, alarm_handler) == SIG_ERR)
    fprintf(stderr, "\nErro no signal do alarme\n");
}

/*-------------------------------------------------------------------
| Function: ctrlc_handler
| Description: Trata da interrupcao do CTRL+C
-------------------------------------------------------------------*/
void ctrlc_handler(int signum){
  ctrlc_flag = 0;
  if(signal(SIGINT, ctrlc_handler) == SIG_ERR)
    fprintf(stderr, "\nErro no signal do ctrl+c\n");
}

/*--------------------------------------------------------------------
| Function: main
| Description: Entrada do programa
---------------------------------------------------------------------*/

int main (int argc, char** argv) {
  int N;
  double tEsq, tSup, tDir, tInf;
  int iter, trab;
  int tam_fatia;
  int res;
  char* fichS;

  if (argc != 11) {
    fprintf(stderr, "Utilizacao: ./heatSim N tEsq tSup tDir tInf iter trab maxD fichS periodoS\n\n");
    die("\nNumero de argumentos invalido\n");
  }

  // Ler Input
  N    = parse_integer_or_exit(argv[1], "N",    1);
  tEsq = parse_double_or_exit (argv[2], "tEsq", 0);
  tSup = parse_double_or_exit (argv[3], "tSup", 0);
  tDir = parse_double_or_exit (argv[4], "tDir", 0);
  tInf = parse_double_or_exit (argv[5], "tInf", 0);
  iter = parse_integer_or_exit(argv[6], "iter", 1);
  trab = parse_integer_or_exit(argv[7], "trab", 1);
  maxD = parse_double_or_exit (argv[8], "maxD", 0);
  fichS = argv[9];
  periodoS = parse_integer_or_exit(argv[10], "periodoS", 0);

  if (N % trab != 0) {
    fprintf(stderr, "\nErro: Argumento %s e %s invalidos.\n"
                    "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
    return -1;
  }

  // Inicializar Barreira
  dual_barrier = dualBarrierInit(trab);
  if (dual_barrier == NULL)
    die("\nNao foi possivel inicializar barreira\n");

  //Manda o signal do CTRL+C
  if(signal(SIGINT, ctrlc_handler) == SIG_ERR)
    fprintf(stderr, "\nErro no signal do ctrl+c\n");

  // Inicializa o temporizador
  if(signal(SIGALRM, alarm_handler) == SIG_ERR)
    fprintf(stderr, "\nErro no signal do alarme\n");
  if(periodoS > 0)
    alarm(periodoS);

  // Calcular tamanho de cada fatia
  tam_fatia = N / trab;

  // Reservar memoria para trabalhadoras
  thread_info *tinfo = (thread_info*) malloc(trab * sizeof(thread_info));
  pthread_t *trabalhadoras = (pthread_t*) malloc(trab * sizeof(pthread_t));

  // Abrir o ficheiro e suas condicoes de aberturas
  filename = fopen(fichS, "r");

  matrix_copies[1] = dm2dNew(N+2,N+2);

  if(filename == NULL){
    matrix_copies[0] = dm2dNew(N+2,N+2);
    dm2dSetLineTo (matrix_copies[0], 0, tSup);
    dm2dSetLineTo (matrix_copies[0], N+1, tInf);
    dm2dSetColumnTo (matrix_copies[0], 0, tEsq);
    dm2dSetColumnTo (matrix_copies[0], N+1, tDir);
  }
  else{
    matrix_copies[0] = readMatrix2dFromFile(filename, N+2, N+2);
    if(matrix_copies[0] == NULL){
      matrix_copies[0] = dm2dNew(N+2,N+2);
      dm2dSetLineTo (matrix_copies[0], 0, tSup);
      dm2dSetLineTo (matrix_copies[0], N+1, tInf);
      dm2dSetColumnTo (matrix_copies[0], 0, tEsq);
      dm2dSetColumnTo (matrix_copies[0], N+1, tDir);
      if(unlink(fichS) == -1)
        fprintf(stderr, "\nErro a dar unlink ao ficheiro\n");
    }
    if(fclose(filename) == EOF)
      fprintf(stderr, "\nErro a dar close ao ficheiro\n");
  }

  if (matrix_copies[0] == NULL || matrix_copies[1] == NULL) {
    die("\nErro ao criar matrizes\n");
  }

  dm2dCopy (matrix_copies[1],matrix_copies[0]);

  if (tinfo == NULL || trabalhadoras == NULL) {
    die("\nErro ao alocar memoria para trabalhadoras\n");
  }

  // Criar trabalhadoras
  for (int i=0; i < trab; i++) {
    tinfo[i].id = i;
    tinfo[i].N = N;
    tinfo[i].iter = iter;
    tinfo[i].trab = trab;
    tinfo[i].tam_fatia = tam_fatia;
    tinfo[i].maxD = maxD;
    tinfo[i].fichS = fichS;
    tinfo[i].periodoS = periodoS;
    res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);
    if (res != 0) {
      die("\nErro ao criar uma tarefa trabalhadora\n");
    }
  }

  // Esperar que as trabalhadoras terminem
  for (int i=0; i<trab; i++) {
    res = pthread_join(trabalhadoras[i], NULL);
    if (res != 0)
      die("\nErro ao esperar por uma tarefa trabalhadora\n");
  }

  dm2dPrint (matrix_copies[dual_barrier->iteracoes_concluidas%2]);
  wait(&status);
  if(access(fichS, F_OK) == 0){
    if(unlink(fichS))
      fprintf(stderr, "\nErro a dar unlink ao ficheiro ou nao foi necessario criar um ficheiro\n");
  }

  // Libertar memoria
  dm2dFree(matrix_copies[0]);
  dm2dFree(matrix_copies[1]);
  free(tinfo);
  free(trabalhadoras);
  dualBarrierFree(dual_barrier);

  return 0;
}
