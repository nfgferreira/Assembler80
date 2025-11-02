#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
// #include <sys\types.h>
// #include <sys\stat.h>
// #include <io.h>
#include <setjmp.h>
#include <string.h>
#include <unistd.h>
#include "l80.h"

extern jmp_buf erro_tratamento;		  /* ponteiro para erro de tratamento durante analise sintatica/semantica */
extern jmp_buf erro_tratamento2;	  /* ponteiro para erro de tratamento apos analise sintatica/semantica */
extern int byte;					  /* valor retornado por analex no caso de BYTE */
extern unsigned int valor;			  /* valor retornado por analex */
extern int rel;						  /* tipo de alocacao que e' o numero ('A', 'C' ou 'D') */
extern char rel_atual;				  /* particao (P ou D) atual de codigo */
extern unsigned int catual, datual;	  /* enderecos atuais dos segmentos */
extern unsigned int i_cseg;			  /* indice de codigo para code */
extern unsigned int i_cseg;			  /* indice de codigo para code */
extern unsigned int i_dseg;			  /* idem para data */
extern unsigned char *cseg[num_part]; /* particao code */
extern unsigned char *dseg[num_part]; /* particao data */
extern unsigned int base_cseg;		  /* endereco base do arquivo atual para cseg */
extern unsigned int base_dseg;		  /* endereco base do arquivo atual para dseg */
extern unsigned int i_os;			  /* indice de particoes de off-set */
extern off_set *end_os[num_prt_os];
extern unsigned char modo_end_c[0x10000 / 4]; /* modo de enderecamento de code */
extern unsigned char modo_end_d[0x10000 / 4]; /* idem data */
extern simb *simbolo;						  /* ponteiro para simbolo retornado por analex */
extern char *nome;							  /* nome do arquivo sendo linkado */
extern char nome_modulo[comp_max + 1];		  /* nome do modulo sendo linkado */
extern int simbolo_redefinido;				  /* indica simbolo definido mais do que 1 vez */
extern simb *inic_simbolo[inic_simb_size];	  /* ponteiros para tabela de simbolo */

/*****************************************************************************
    void trata_programa ()

    Trata chegada de atomos relativos a programa.
    Atomos reconhecidos:
        BYTE
        EXTERNAL_PLUS_OFFSET
        SET_LOCATION_COUNTER
        PROGRAM_RELATIVE
        DATA_RELATIVE

*****************************************************************************/

void trata_programa(void)
{
    int atomo;

    while (1)
        switch (atomo = analex())
        {
        case BYTE:
            coloca_byte(byte);
            break;

        case EXTERNAL_PLUS_OFFSET:
            coloca_external_plus_offset(valor, rel);
            break;

        case SET_LOCATION_COUNTER:
            set_location_counter(valor, rel);
            break;

        case PROGRAM_RELATIVE:
            coloca_program_relative(valor);
            break;

        case DATA_RELATIVE:
            coloca_data_relative(valor);
            break;

        default:
            volta(atomo);
            return;
        }
}

/*****************************************************************************
    void coloca_byte ()

    Coloca byte absoluto no endereco atual.

*****************************************************************************/

void coloca_byte(int byte)
{
    unsigned int count, n;

    switch (rel_atual)
    {
    case 'A':						 /* em enderecamento ASEG */
        longjmp(erro_tratamento, 5); /* nao pode colocar byte em endereco ASEG */

    case 'P': /* enderecamento CSEG */
        count = catual - i_cseg;
        while (count)
            if (!(i_cseg & mask_part)) /* necessario alocar */
            {
                coloca_byte_cseg(0xff);
                count--;
            }
            else
            {
                n = ((comp_part - i_cseg) & mask_part) > count ? count : ((comp_part - i_cseg) & mask_part);
                memset((cseg[i_cseg >> part_bits] + (i_cseg & mask_part)), 0xff, n);
                count -= n;
                if (!(i_cseg += n))
                    longjmp(erro_tratamento, 6); /* ultrapassados 64K de codigo */
            }
        coloca_byte_cseg(byte);
        catual = i_cseg;
        break;

    case 'D': /* enderecamento DSEG */
        count = datual - i_dseg;
        while (count)
            if (!(i_dseg & mask_part)) /* necessario alocar */
            {
                coloca_byte_dseg(0xff);
                count--;
            }
            else
            {
                n = ((comp_part - i_dseg) & mask_part) > count ? count : ((comp_part - i_dseg) & mask_part);
                memset((dseg[i_dseg >> part_bits] + (i_dseg & mask_part)), 0xff, n);
                count -= n;
                if (!(i_dseg += n))
                    longjmp(erro_tratamento, 7); /* ultrapassados 64K de dados */
            }
        coloca_byte_dseg(byte);
        datual = i_dseg;
        break;
    }
}

/*****************************************************************************
    void coloca_byte_cseg

    Coloca byte em segmento CSEG

*****************************************************************************/

void coloca_byte_cseg(int byte)
{
    if (!(i_cseg & mask_part)) /* caso nao ha' espaco alocado para colocar byte */
        if ((cseg[i_cseg >> part_bits] = (unsigned char *)malloc(comp_part)) == NULL)
            longjmp(erro_tratamento, 4); /* nao ha' memoria suficiente */

    *(cseg[i_cseg >> part_bits] + (i_cseg & mask_part)) = byte;
    if (!++i_cseg)
    {
        i_cseg--;					 /* volta para valor antigo para poder desalocar corretamente */
        longjmp(erro_tratamento, 6); /* ultrapassados 64K de codigo */
    }
}

/*****************************************************************************
    void coloca_byte_dseg

    Coloca byte em segmento DSEG

*****************************************************************************/

void coloca_byte_dseg(int byte)
{
    if (!(i_dseg & mask_part)) /* caso nao ha' espaco alocado para colocar byte */
        if ((dseg[i_dseg >> part_bits] = (unsigned char *)malloc(comp_part)) == NULL)
            longjmp(erro_tratamento, 4); /* nao ha' memoria suficiente */

    *(dseg[i_dseg >> part_bits] + (i_dseg & mask_part)) = byte;
    if (!++i_dseg)
    {
        i_dseg--;					 /* volta para valor antigo para poder desalocar corretamente */
        longjmp(erro_tratamento, 7); /* ultrapassados 64K de dados */
    }
}

/*****************************************************************************
    void set_location_counter ()

    Acerta endereco atual.

*****************************************************************************/

void set_location_counter(unsigned int valor, char rel)
{
    if ((rel_atual = rel) == 'P')
    {
        if (catual > valor + base_cseg)
            longjmp(erro_tratamento, 10); /* codigo retornou para tras */
        catual = valor + base_cseg;
    }
    else if (rel == 'D')
    {
        if (datual > valor + base_dseg)
            longjmp(erro_tratamento, 10); /* codigo retornou para tras */
        datual = valor + base_dseg;
    }
}

/*****************************************************************************
    void coloca_external_plus_offset ()

    Coloca offset de endereco externo na tabela, dado o endereco e a alocacao.

*****************************************************************************/

void coloca_external_plus_offset(unsigned int valor, char rel)
{
    if (rel != 'A')
        longjmp(erro_tratamento, 2); /* arquivo inconsistente */

    if (rel_atual != 'P' && rel_atual != 'D')
        longjmp(erro_tratamento, 2); /* arquivo inconsistente */

    if (!(i_os & mask_prt_os)) /* caso nao haja espaco, aloca */
    {
        if ((i_os >> os_bits) >= num_prt_os)
        {
            i_os--;						 /* volta para valor antigo para poder desalocar corretamente (garantia) */
            longjmp(erro_tratamento, 8); /* numero de offsets externos excedido */
        }
        if ((end_os[i_os >> os_bits] = (off_set *)malloc(comp_prt_os * (sizeof(off_set)))) == NULL)
            longjmp(erro_tratamento, 4); /* memoria insuficiente */
    }

    (end_os[i_os >> os_bits] + (i_os & mask_prt_os))->r_endereco = rel_atual;
    (end_os[i_os >> os_bits] + (i_os & mask_prt_os))->endereco = (rel_atual == 'P' ? catual : datual);
    (end_os[i_os >> os_bits] + (i_os & mask_prt_os))->os = valor;
    i_os++;
}

/*****************************************************************************
    coloca_program_relative ()

    Coloca endereco relativo a programa na memoria.

*****************************************************************************/

void coloca_program_relative(unsigned int valor)
{
    unsigned char mask, *end_c;

    /* pega mascara e endereco de onde colocar os bits */
    if (rel_atual != 'A')
    {
        get_bits(rel_atual == 'P' ? modo_end_c : modo_end_d,
                 rel_atual == 'P' ? catual : datual, &mask, &end_c);
        *end_c |= mask & 0x55;
    }
    else
        longjmp(erro_tratamento, 5); /* nao pode colocar byte em endereco ASEG */

    coloca_byte((unsigned char)((valor + base_cseg) & 0xff));
    coloca_byte((unsigned char)((valor + base_cseg) >> 8));
}

/*****************************************************************************
    coloca_data_relative ()

    Coloca endereco relativo a dados na memoria.

*****************************************************************************/

void coloca_data_relative(unsigned int valor)
{
    unsigned char mask, *end_c;

    /* pega mascara e endereco de onde colocar os bits */
    if (rel_atual != 'A')
    {
        get_bits(rel_atual == 'P' ? modo_end_c : modo_end_d,
                 rel_atual == 'P' ? catual : datual, &mask, &end_c);
        *end_c |= mask & 0xaa;
    }
    else
        longjmp(erro_tratamento, 5); /* nao pode colocar byte em endereco ASEG */

    coloca_byte((unsigned char)((valor + base_dseg) & 0xff));
    coloca_byte((unsigned char)((valor + base_dseg) >> 8));
}

/*****************************************************************************
    get_bits ()

    Dado ponteiro para vetor de tipo de alocacao e endereco, volta com ponteiro
para byte desejado e mascara para os dois bits que vao ser lidos ou
modificados.

*****************************************************************************/

void get_bits(unsigned char *v, unsigned int end, unsigned char *m, unsigned char **b)
{
    *m = 0x03 << ((end & 0x03) << 1);
    *b = v + (end >> 2);
}

/*****************************************************************************
    trata_define_entry_point ()

    Coloca na tabela de simbolos os valores passados.

*****************************************************************************/

void trata_define_entry_point(void)
{
    int atomo;

    while ((atomo = analex()) == DEFINE_ENTRY_POINT)
    {
        if (simbolo->definido)
        {
            mprintf("%s(%s): SYMBOL %s IS REDEFINED.\n", str_maiuscula(nome), str_maiuscula(nome_modulo), simbolo->nome);
            simbolo_redefinido = 1;
        }
        else
        {
            simbolo->definido = 1;
            simbolo->atrib = rel;
            simbolo->valor = valor;
            if (rel != 'A')
                simbolo->valor += (rel == 'P' ? base_cseg : base_dseg);
        }
    }
    volta(atomo);
}

/*****************************************************************************
    trata_chain_external ()

    Trata chegada de ponteiro para lista ligada de simbolo externo.
    O procedimento realizado e' o seguinte:
        se o simbolo aponta para algum lugar deve-se varrer a lista ligada
         do arquivo que esta' sendo tratado a partir do endereco do simbolo
         passado por analex ate' que se encontre o final. No lugar do final
         deve-se colocar o ponteiro que esta' na tabela de simbolos e apos
         isto deve-se atualizar o ponteiro da tabela de simbolos com o valor
         retornado por analex.
        se o simbolo nao aponta para nenhum lugar deve-se colocar o endereco
         passado por analex.

*****************************************************************************/

void trata_chain_external(void)
{
    int atomo;
    char rel_d, rel_o;
    unsigned int valor_d, valor_o;
    unsigned char mask, *end_c;

    while ((atomo = analex()) == CHAIN_EXTERNAL)
    {
        if (rel == 'A')
            if (valor)
                longjmp(erro_tratamento, 2); /* inconsistencia no arquivo */
            else
                continue; /* nada faca se simbolo nao utilizado */

        rel_d = rel;
        valor_d = valor + (rel == 'P' ? base_cseg : base_dseg);

        do /* trasforma lista ligada do ultimo arquivo colocado para base atual */
            pega_prev(rel_o = rel_d, valor_o = valor_d, &rel_d, &valor_d, err_trt1);
        while (rel_d != 'A');

        if (valor_d)
            longjmp(erro_tratamento, 2); /* inconsistencia no arquivo */

        /* coloca endereco antigo da tabela  no final da lista ligada do arquivo atual e atualiza  inicio da lista ligada */
        get_bits(rel_o == 'P' ? modo_end_c : modo_end_d, valor_o, &mask, &end_c);
        *end_c &= ~mask;
        if (simbolo->atrib_p != 'A')
            *end_c |= mask & (simbolo->atrib_p == 'P' ? 0x55 : 0xaa);
        if (rel_o == 'P')
        {
            *(cseg[valor_o >> part_bits] + (valor_o & mask_part)) = simbolo->prev & 0xff;
            *(cseg[(valor_o + 1) >> part_bits] + ((valor_o + 1) & mask_part)) = simbolo->prev >> 8;
        }
        else
        {
            *(dseg[valor_o >> part_bits] + (valor_o & mask_part)) = simbolo->prev & 0xff;
            *(dseg[(valor_o + 1) >> part_bits] + ((valor_o + 1) & mask_part)) = simbolo->prev >> 8;
        }

        simbolo->atrib_p = rel;
        simbolo->prev = valor + (rel == 'P' ? base_cseg : base_dseg);
    }

    volta(atomo);
}

/*****************************************************************************
    pega_prev ()

    Dado endereco de origem, acerta endereco destino.

*****************************************************************************/

void pega_prev(char rel_o, unsigned int end_o, char *rel_d, unsigned int *end_d, void (*erro)(void))
{
    unsigned char mask, *end_c;
    unsigned char aux;

    if (rel_o == 'P')
    {
        get_bits(modo_end_c, end_o, &mask, &end_c);
        *end_d = 0xff & *(cseg[end_o >> part_bits] + (end_o & mask_part));
        *end_d += (*(cseg[(end_o + 1) >> part_bits] + ((end_o + 1) & mask_part))) << 8;
    }
    else
    {
        get_bits(modo_end_d, end_o, &mask, &end_c);
        *end_d = 0xff & *(dseg[end_o >> part_bits] + (end_o & mask_part));
        *end_d += (*(dseg[(end_o + 1) >> part_bits] + ((end_o + 1) & mask_part))) << 8;
    }

    aux = mask & *end_c;
    while (!(mask & 0x03))
    {
        aux >>= 2;
        mask >>= 2;
    }

    switch (aux & 0x03)
    {
    case 0:
        *rel_d = 'A';
        break;

    case 1:
        *rel_d = 'P';
        break;

    case 2:
        *rel_d = 'D';
        break;

    case 3:
        (*erro)(); /* erro interno */
    }
}

/*****************************************************************************
    varre_t_sym ()

    Varre tabela de simbolos colocando os valores dos simbolos externos.
Caso apareca algum simbolo utilizado mas que nao seja definido a rotina volta
com 1. Caso isto nao ocorra, volta com 0.

*****************************************************************************/

int varre_t_sym(void)
{
    simb *s;
    int i;
    char rel_d, rel_o;
    unsigned int valor_d, valor_o;
    unsigned char mask, *end_c;
    int res = 0; /* indica que ha' simbolo nao definido */

    for (i = 0; i < inic_simb_size; i++)
        if ((s = inic_simbolo[i]) != NULL)
            do
                if (s->atrib_p == 'P' || s->atrib_p == 'D') /* tem lista ligada */
                    if (s->definido)						/* se o simbolo esta' definido */
                    {
                        /* (rel_d, valor_d) = ponteiro para destino */
                        rel_d = s->atrib_p;
                        valor_d = s->prev;

                        do
                        {
                            pega_prev(rel_o = rel_d, valor_o = valor_d, &rel_d, &valor_d, err_trt2);
                            if (rel_d == 'P') /* testa se programa nao esta' maluco */
                            {
                                if (valor_d >= i_cseg)
                                    longjmp(erro_tratamento2, 1); /* ha' arquivo invalido: apontando para lugar que nao existe */
                            }
                            else if (rel_d == 'D')
                            {
                                if (valor_d >= i_dseg)
                                    longjmp(erro_tratamento2, 1); /* ha' arquivo invalido: apontando para lugar que nao existe */
                            }
                            get_bits(rel_o == 'P' ? modo_end_c : modo_end_d, valor_o, &mask, &end_c);
                            *end_c &= ~mask;
                            if (s->atrib != 'A')								  /* marca a que segmento o simbolo esta' associado (se absoluto ja' esta' marcado) */
                                *end_c |= mask & (s->atrib == 'P' ? 0x55 : 0xaa); /* coloca novo atributo do simbolo */
                            if (rel_o == 'P')									  /* coloca o valor do simbolo em relacao ao segmento */
                            {
                                *(cseg[valor_o >> part_bits] + (valor_o & mask_part)) = s->valor & 0xff;
                                *(cseg[(valor_o + 1) >> part_bits] + ((valor_o + 1) & mask_part)) = s->valor >> 8;
                            }
                            else
                            {
                                *(dseg[valor_o >> part_bits] + (valor_o & mask_part)) = s->valor & 0xff;
                                *(dseg[(valor_o + 1) >> part_bits] + ((valor_o + 1) & mask_part)) = s->valor >> 8;
                            }
                        } while (rel_d != 'A');

                        if (valor_d)
                            longjmp(erro_tratamento2, 1); /* ha' inconsistencia em algum arquivo */
                    }
                    else
                    {
                        s->definido = -1;
                        res = 1;
                    }
            while ((s = s->next_simbolo) != NULL);

    return res;
}

/*****************************************************************************
    err_trt1 ()

    Erro interno para tratamento ainda na fase de leitura de arquivo.

*****************************************************************************/

void err_trt1(void)
{
    longjmp(erro_tratamento, 9); /* erro interno */
}

/*****************************************************************************
    err_trt2 ()

    Erro interno para tratamento apos a fase de leitura de arquivo.

*****************************************************************************/

void err_trt2(void)
{
    longjmp(erro_tratamento2, 2); /* erro interno */
}

/*****************************************************************************
    void trata_nao_definidos (void)

    Imprime na tela as variaveis que nao estao definidas mas sao utilizadas.

*****************************************************************************/

void trata_nao_definidos(void)
{
    int i;
    simb *s;
    int coluna = 0;
    int nlinha = 0;

    mprintf("\nUNDEFINED SYMBOLS:");

    for (i = 0; i < inic_simb_size; i++)
        if ((s = inic_simbolo[i]) != NULL)
            do
                if (s->definido == -1) /* nao definido e utilizado: erro */
                {
                    if (!nlinha)
                        putchar('\n');
                    if (coluna)
                    {
                        while (coluna++ < 13)
                            putchar(' ');
                        coluna = 0;
                    }
                    coluna = mprintf("%s", s->nome);
                    if (++nlinha >= 6)
                        coluna = nlinha = 0;
                }
            while ((s = s->next_simbolo) != NULL);
}

/*****************************************************************************
    void acerta enderecos ()

    Coloca enderecos corretos

*****************************************************************************/

void acerta_enderecos(unsigned int areac, unsigned int aread)
{
    unsigned int i;
    unsigned int result;
    unsigned char mask, *end_c;

    for (i = 0; i < i_cseg; i++)
    {
        get_bits(modo_end_c, i, &mask, &end_c);
        if (mask & *end_c & 0x55) /* caso relativo a codigo */
        {
            result = *(cseg[i >> part_bits] + (i & mask_part)) & 0xff;
            result += *(cseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) << 8;
            result += areac;
            *(cseg[i >> part_bits] + (i & mask_part)) = result & 0xff;
            *(cseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) = result >> 8;
        }
        else if (mask & *end_c & 0xaa) /* caso relativo a dados */
        {
            result = *(cseg[i >> part_bits] + (i & mask_part)) & 0xff;
            result += *(cseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) << 8;
            result += aread;
            *(cseg[i >> part_bits] + (i & mask_part)) = result & 0xff;
            *(cseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) = result >> 8;
        }
    }

    for (i = 0; i < i_dseg; i++)
    {
        get_bits(modo_end_d, i, &mask, &end_c);
        if (mask & *end_c & 0x55) /* caso relativo a codigo */
        {
            result = *(dseg[i >> part_bits] + (i & mask_part)) & 0xff;
            result += *(dseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) << 8;
            result += areac;
            *(dseg[i >> part_bits] + (i & mask_part)) = result & 0xff;
            *(dseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) = result >> 8;
        }
        else if (mask & *end_c & 0xaa) /* caso relativo a dados */
        {
            result = *(dseg[i >> part_bits] + (i & mask_part)) & 0xff;
            result += *(dseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) << 8;
            result += aread;
            *(dseg[i >> part_bits] + (i & mask_part)) = result & 0xff;
            *(dseg[(i + 1) >> part_bits] + ((i + 1) & mask_part)) = result >> 8;
        }
    }
}

/*****************************************************************************
    void trata_offsets ()

    Coloca offsets externos.

*****************************************************************************/

void trata_offsets(void)
{
    unsigned int i;
    unsigned int result;
    off_set *os;

    for (i = 0; i < i_os; i++)
    {
        os = end_os[i >> os_bits] + (i & mask_prt_os);
        if (os->r_endereco == 'P')
        {
            result = *(cseg[os->endereco >> part_bits] + (os->endereco & mask_part)) & 0xff;
            result += *(cseg[(os->endereco + 1) >> part_bits] + ((os->endereco + 1) & mask_part)) << 8;
            result += os->os;
            *(cseg[os->endereco >> part_bits] + (os->endereco & mask_part)) = result & 0xff;
            *(cseg[(os->endereco + 1) >> part_bits] + ((os->endereco + 1) & mask_part)) = result >> 8;
        }
        else
        {
            result = *(dseg[os->endereco >> part_bits] + (os->endereco & mask_part)) & 0xff;
            result += *(dseg[(os->endereco + 1) >> part_bits] + ((os->endereco + 1) & mask_part)) << 8;
            result += os->os;
            *(dseg[os->endereco >> part_bits] + (os->endereco & mask_part)) = result & 0xff;
            *(dseg[(os->endereco + 1) >> part_bits] + ((os->endereco + 1) & mask_part)) = result >> 8;
        }
    }
}

/*****************************************************************************
    void monta_arquivo ()

    Monta arquivo final.

*****************************************************************************/

void monta_arquivo(int arq, unsigned int code, unsigned int data)
{
    unsigned int count;
    int i;

    if (data > code)
    { /* monta code e depois data */
        for (count = i_cseg, i = 0; count; i++)
            if (count >= comp_part)
            {
                if (write(arq, cseg[i], comp_part) != comp_part)
                    longjmp(erro_tratamento2, 6);
                count -= comp_part;
            }
            else
            {
                if (write(arq, cseg[i], count) != count)
                    longjmp(erro_tratamento2, 6);
                count = 0;
            }

        if (i_dseg)
        { /* coloca ff's entre particoes */
            memset(modo_end_c, 0xff, sizeof modo_end_c);
            count = data - code - i_cseg;
            while (count)
                if (count >= sizeof modo_end_c)
                {
                    if (write(arq, modo_end_c, sizeof modo_end_c) != sizeof modo_end_c)
                        longjmp(erro_tratamento2, 6);
                    count -= sizeof modo_end_c;
                }
                else
                {
                    if (write(arq, modo_end_c, count) != count)
                        longjmp(erro_tratamento2, 6);
                    count = 0;
                }
        }

        for (count = i_dseg, i = 0; count; i++)
            if (count >= comp_part)
            {
                if (write(arq, dseg[i], comp_part) != comp_part)
                    longjmp(erro_tratamento2, 6);
                count -= comp_part;
            }
            else
            {
                if (write(arq, dseg[i], count) != count)
                    longjmp(erro_tratamento2, 6);
                count = 0;
            }
    }
    else
    { /* monta data e depois code */
        for (count = i_dseg, i = 0; count; i++)
            if (count >= comp_part)
            {
                if (write(arq, dseg[i], comp_part) != comp_part)
                    longjmp(erro_tratamento2, 6);
                count -= comp_part;
            }
            else
            {
                if (write(arq, dseg[i], count) != count)
                    longjmp(erro_tratamento2, 6);
                count = 0;
            }

        if (i_cseg)
        { /* coloca ff's entre particoes */
            memset(modo_end_c, 0xff, sizeof modo_end_c);
            count = code - data - i_dseg;
            while (count)
                if (count >= sizeof modo_end_c)
                {
                    if (write(arq, modo_end_c, sizeof modo_end_c) != sizeof modo_end_c)
                        longjmp(erro_tratamento2, 6);
                    count -= sizeof modo_end_c;
                }
                else
                {
                    if (write(arq, modo_end_c, count) != count)
                        longjmp(erro_tratamento2, 6);
                    count = 0;
                }
        }

        for (count = i_cseg, i = 0; count; i++)
            if (count >= comp_part)
            {
                if (write(arq, cseg[i], comp_part) != comp_part)
                    longjmp(erro_tratamento2, 6);
                count -= comp_part;
            }
            else
            {
                if (write(arq, cseg[i], count) != count)
                    longjmp(erro_tratamento2, 6);
                count = 0;
            }
    }
}
