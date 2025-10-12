#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
//#include <sys\types.h>
//#include <sys\stat.h>
//#include <io.h>
#include <setjmp.h>
#include <string.h>
#include "l80.h"

static int l_file;											/* arquivo sendo lincado */
static int voltou;											/* indica atomo voltado para analisador lexico */

extern int falta;											/* numero de bytes que falta ler do buffer de leitura de arquivo */
extern int devolvido;										/* atomo devolvido para analex */
extern int byte;											/* valor retornado por analex no caso de BYTE */
extern int mask;											/* mascara utilizada por analex para ler bits */
extern int leit;											/* indica numero de bytes do buffer lidos por analex */
extern simb *inic_simbolo [inic_simb_size];		/* ponteiros para tabela de simbolo */
extern simb *simbolo;										/* ponteiro para simbolo retornado por analex */
extern char bl [4096];									/* buffer de leitura de analex */
extern int resta_simb;									/* numero de simbolos ainda possiveis de serem usados na tabela */
extern int nset_simb;										/* numero de particoes de simbolo utilizadas */
extern simb *aloc_simb [max_simb_aloc];				/* ponteiro para arrays alocadas para simbolos */
extern int rel;												/* tipo de alocacao que e' o numero ('A', 'C' ou 'D') */
extern unsigned int valor;								/* valor retornado por analex */
extern int coloca_simbolo;								/* indica para analex que simbolo deve ser colocado se nao procurado */
extern char simbolo_analex [comp_max + 1];			/* nome do simbolo lido quando nao e' colocado na tabela de simbolos */
extern jmp_buf erro_tratamento;						/* ponteiro para erro de tratamento durante analise sintatica/semantica */

