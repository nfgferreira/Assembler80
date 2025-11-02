#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
// #include <sys\types.h>
// #include <sys\stat.h>
// #include <io.h>
#include <setjmp.h>
#include <string.h>
#include "l80.h"

static int voltou; /* indica atomo voltado para analisador lexico */

int falta;			/* numero de bytes que falta ler do buffer de leitura de arquivo */
int devolvido;		/* atomo devolvido para analex */
int byte;			/* valor retornado por analex no caso de BYTE */
int mask;			/* mascara utilizada por analex para ler bits */
int leit;			/* indica numero de bytes do buffer lidos por analex */
simb *simbolo;		/* ponteiro para simbolo retornado por analex */
char bl[4096];		/* buffer de leitura de analex */
unsigned int valor; /* valor retornado por analex */

extern simb *inic_simbolo[inic_simb_size]; /* ponteiros para tabela de simbolo */
extern int resta_simb;					   /* numero de simbolos ainda possiveis de serem usados na tabela */
extern int nset_simb;					   /* numero de particoes de simbolo utilizadas */
extern simb *aloc_simb[max_simb_aloc];	   /* ponteiro para arrays alocadas para simbolos */
extern int rel;							   /* tipo de alocacao que e' o numero ('A', 'C' ou 'D') */
extern int coloca_simbolo;				   /* indica para analex que simbolo deve ser colocado se nao procurado */
extern char simbolo_analex[comp_max + 1];  /* nome do simbolo lido quando nao e' colocado na tabela de simbolos */
extern jmp_buf erro_tratamento;			   /* ponteiro para erro de tratamento durante analise sintatica/semantica */
extern int l_file;						   /* arquivo sendo lincado */

/*****************************************************************************
    inicia_analex ()

    Inicia variaveis de analisador lexico.

*****************************************************************************/

void inicia_analex(void)
{
    voltou = 0;
    falta = 0;
}

/*****************************************************************************
    analex ()

    Decodifica arquivo de entrada.

*****************************************************************************/

int analex(void)
{
    if (voltou)
    {
        voltou = 0;
        return devolvido;
    }

    if (!pega_bit())
    {
        byte = pega_8bits();
        return BYTE;
    }

    switch (pega_2bits())
    {
    case 0:
        switch (pega_4bits())
        {
        case 0:
            pega_nome();
            return ENTRY_SYMBOL;

        case 1:
        case 3:
        case 4:
        case 5:
        case 8:
        case 12:
            return ERRO;

        case 2:
            pega_nome();
            return PROGRAM_NAME;

        case 6:
            pega_numero();
            pega_nome();
            return CHAIN_EXTERNAL;

        case 7:
            pega_numero();
            pega_nome();
            return DEFINE_ENTRY_POINT;

        case 9:
            pega_numero();
            return EXTERNAL_PLUS_OFFSET;

        case 10:
            pega_numero();
            return DEFINE_DATA_SIZE;

        case 11:
            pega_numero();
            return SET_LOCATION_COUNTER;

        case 13:
            pega_numero();
            return DEFINE_PROGRAM_SIZE;

        case 14:
            pega_numero();
            if (falta && mask != 0x80)
            {
                leit++;
                falta--;
                mask = 0x80;
            }
            return END_MODULE;

        case 15:
            return END_FILE;
        }

    case 1:
        pega_valor();
        return PROGRAM_RELATIVE;

    case 2:
        pega_valor();
        return DATA_RELATIVE;

    case 3:
        return ERRO;
    }
    return ERRO;
}

/*****************************************************************************
    pega_nome ()

    Devolve nome lido do arquivo de entrada.

*****************************************************************************/

void pega_nome(void)
{
    int i, j;
    char atm_name[comp_max + 1];

    for (j = 0, i = pega_3bits(); i; i--, j++)
        atm_name[j] = (char)pega_8bits();
    atm_name[j] = '\0';
    ver_nome(atm_name, j); /* coloca simbolo na tabela de simbolos */
}

/*****************************************************************************
    ver_nome ():

    Procura ou coloca nome na tabela de simbolos.

*****************************************************************************/

void ver_nome(char *s, int comp)
{
    int i, ent;
    simb *smb;

    if (!coloca_simbolo)
    {
        strcpy(simbolo_analex, s);
        simbolo = NULL;
        return;
    }

    for (ent = i = 0; i < comp; i++)
        ent = hash(ent, s[i]);

    if (inic_simbolo[ent &= (inic_simb_size - 1)] != NULL)
    {
        for (smb = inic_simbolo[ent]; smb->next_simbolo != NULL; smb = smb->next_simbolo)
            if (!strcmp(s, smb->nome))
            {
                simbolo = smb;
                return;
            }
        if (!strcmp(s, smb->nome))
            simbolo = smb;
        else
        {
            simbolo = smb->next_simbolo = get_simb();
            strcpy(simbolo->nome, s);
        }
    }
    else
    {
        simbolo = inic_simbolo[ent] = get_simb();
        strcpy(simbolo->nome, s);
    }
}

/*****************************************************************************
    pega_8bits ()

    Devolve valor dos proximos 8 bits do arquivo de entrada.

*****************************************************************************/

unsigned int pega_8bits(void)
{
    int i;

    i = 16 * pega_4bits();
    return i + pega_4bits();
}

/*****************************************************************************
    pega_4bits ()

    Devolve valor dos proximos 4 bits do arquivo de entrada.

*****************************************************************************/

int pega_4bits(void)
{
    int i;

    i = 4 * pega_2bits();
    return i + pega_2bits();
}

/*****************************************************************************
    pega_3bits ()

    Devolve valor dos proximos 3 bits do arquivo de entrada.

*****************************************************************************/

int pega_3bits(void)
{
    int i;

    i = 2 * pega_2bits();
    return i + pega_bit();
}

/*****************************************************************************
    pega_2bits ()

    Devolve valor dos proximos 2 bits do arquivo de entrada.

*****************************************************************************/

int pega_2bits(void)
{
    int i;

    i = 2 * pega_bit();
    return i + pega_bit();
}

/*****************************************************************************
    pega_bit ()

    Le bit do arquivo de entrada.

*****************************************************************************/

int pega_bit(void)
{
    int retorno;

    if (falta == 0)
    {
        falta = read(l_file, bl, sizeof bl);
        mask = 0x80;
        leit = 0;
        if (!falta)
            longjmp(erro_tratamento, 2); /* inconsistencia no arquivo: nao deveria estar lendo coisas se arquivo terminou */
    }
    retorno = bl[leit] & mask;
    if (mask == 1)
    {
        mask = 0x80;
        leit++;
        falta--;
    }
    else
        mask >>= 1;
    return retorno != 0;
}

/*****************************************************************************
    get_simb ():

    Devolve ponteiro para novo simbolo.

*****************************************************************************/

simb *get_simb(void)
{
    simb *smb;

    if (!resta_simb)
    {
        if (nset_simb >= max_simb_aloc)
            longjmp(erro_tratamento, 3); /* tabela se simbolos cheia */
        if ((aloc_simb[nset_simb] = (simb *)malloc((sizeof(simb)) * nsimb_aloc)) == NULL)
            longjmp(erro_tratamento, 4); /* memoria insuficiente */
        resta_simb = nsimb_aloc;
        nset_simb++;
    }
    smb = aloc_simb[nset_simb - 1] + nsimb_aloc - resta_simb--;
    smb->atrib = 0;		/* simbolo ainda nao tem valor */
    smb->atrib_p = 'A'; /* simbolo nao aponta para inicio de nada */
    smb->prev = 0;
    smb->definido = 0; /* indica que simbolo nao foi definido */
    smb->next_simbolo = NULL;
    return smb;
}

/*****************************************************************************
    pega_numero ()

    Devolve valor do numero de 16 bits, indicando a que modulo e' relativo.

*****************************************************************************/

void pega_numero(void)
{
    switch (pega_2bits())
    {
    case 0:
        rel = 'A';
        break;

    case 1:
        rel = 'P';
        break;

    case 2:
        rel = 'D';
        break;

    case 3:
        longjmp(erro_tratamento, 2); /* arquivo invalido */
    }

    pega_valor();
}

/*****************************************************************************
    pega_valor ()

    Devolve valor do numero de 16 bits.

*****************************************************************************/

void pega_valor(void)
{
    int i;

    i = pega_8bits();
    valor = i + 0x100 * pega_8bits();
}

/*****************************************************************************
    volta ()

    Devolve atomo.

*****************************************************************************/

void volta(int atomo)
{
    devolvido = atomo;
    voltou = 1;
}

/*****************************************************************************
    limpa_ts ():

    Inicializa tabela de simbolos.

*****************************************************************************/

void limpa_ts(void)
{
    int i;

    for (i = 0; i < inic_simb_size; i++)
        inic_simbolo[i] = NULL;
}
