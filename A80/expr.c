#include <stdio.h>
#include <setjmp.h>
#include "variaveis.h"
#include "protos.h"

/********************* variaveis exclusivas de expr ************************/

static jmp_buf sys_pilha; /* endereco de retorno caso erro em exp. */
static int nivel;		  /* nivel de parenteses */
static int parsing;		  /* indica se apenas fazendo parsing de expressao */

// Other variables
unsigned int valor;		  /* valor do numero retornado pelo analex */
int ppilha;				  /* ponteiro de pilha de expressoes */
number pilha[comp_pilha]; /* pilha de expressoes */

// Externals
extern char string[81]; /* string retornada pelo analisador lexico */
extern simb *simbolo;	/* ponteiro para simbolo retornado por analex */
extern int causa;		/* motivo do erro no analisador lexico */
extern unsigned int pc; /* program counter */
extern char alocatual;	/* tipo de alocacao atual: aseg, cseg ou dseg */

#ifndef bottom_up

/*****************************************************************************
    expressao ():

    Calcula expressao.
*****************************************************************************/

int expressao(void)
{
    parsing = 0;
    return expr();
}

/*****************************************************************************
    parse_expr ():

    Faz parsing de expressao.
*****************************************************************************/

int parse_expr(void)
{
    parsing = 1;
    return expr();
}

/*****************************************************************************
    expr ():

    Retorna 0 se expressao OK e !0 se algum erro.

    Expressoes pode ter os seguintes operandos, com prioridade crescente:

                                HIGH		LOW
                                OR			XOR
                                    AND
                                    NOT
                            EQ	NE	LE	GE	LT	GT
                                    +	-
                            *	/	MOD  SHL  SHR

    Isto gera a gramatica:

            S: HIGH O | LOW O | O
            O: A or O | A
            A: R AND A | R | NOT R AND A | NOT R
            R:	C r R | C
            C:	T s C | T
            T: n | n m T | (S) | (S) m T | - n | - n m T | - (S) | - (S) m T

    onde:		or:	OR | XOR
                r:		EQ | LT | LE | GT | GE | NE
                s:		+ | -
                n:		numero

    Modificando e impondo as tarefas:
            S: HIGH O							O, "high"
            S:	LOW O								O, "low"
            S:	O									O
            O:	AB									A, B
            B: or AB								A, "or", B
            B: epsilon
            A:	RD									R, D
            A:	NOT RD							R, "not", D
            D:	AND RD							R, "and", D
            D:	AND NOT RD						R, "not", "and", D
            D: epsilon
            R:	CE									C, E
            R:	r CE								C, "r", E
            E:	epsilon
            C:	TF									T, F
            F:	s TF								T, "s", F
            F:	epsilon
            T: nU									"n", U
            T:	(S)U								S, U
            T: -nU								"n", "-", U
            T:	-(S)U								S, "-", U
            U:	m n U								"n", "m", U
            U:	m (S) U							S, "m", U
            U: epsilon

    Na pratica as rotinas S, O e B serao juntadas e chamadas de hl_exp.
    A e D serao juntadas e chamadas de and_exp.
    R e E serao juntadas e chamadas de rel_exp.
    C e F serao juntadas e chamadas de soma_exp.
    T e U serao juntadas e chamadas de termo_exp.

*****************************************************************************/

int expr(void)
{
    int retorno;

    ppilha = 0;
    nivel = 0;
    if (retorno = setjmp(sys_pilha))
        return retorno;
    hl_exp();
    return 0;
}

void hl_exp(void)
{
    t_atomo atomo;
    t_atomo hl;

    if (++nivel >= 10)
        longjmp(sys_pilha, MUITOS_NIVEIS_DE_PARENTESES);

    if ((hl = analex()) != HIGH && hl != LOW)
        volta_atomo(hl);

    and_exp();
    while ((atomo = analex()) == OR || atomo == XOR)
    {
        and_exp();
        or_xor(atomo);
    }
    volta_atomo(atomo);

    if (hl == HIGH)
        high();
    else if (hl == LOW)
        low();
}

void and_exp(void)
{
    t_atomo atomo;
    int not;

    if (!(not = (atomo = analex()) == NOT))
        volta_atomo(atomo);
    rel_exp();
    if (not)
        complementa();

    while ((atomo = analex()) == AND)
    {
        if ((atomo = analex()) != NOT)
            volta_atomo(atomo);
        rel_exp();
        if (atomo == NOT)
            complementa();
        and();
    }
    volta_atomo(atomo);
}

void rel_exp(void)
{
    t_atomo relop, atomo;

    soma_exp();
    while ((relop = analex()) >= EQ && relop <= GT)
    {
        soma_exp();
        relacao(relop);
    }
    volta_atomo(relop);
}

void soma_exp(void)
{
    t_atomo sum;
    t_atomo troca_s;

    if ((troca_s = analex()) != MENOS)
        volta_atomo(troca_s);
    termo_exp();
    if (troca_s == MENOS)
        troca_sinal();

    while ((sum = analex()) == MAIS || sum == MENOS)
    {
        termo_exp();
        if (sum == MAIS)
            soma();
        else
            subtrai();
    }
    volta_atomo(sum);
}

void termo_exp(void)
{
    t_atomo atomo;
    t_atomo mlt;

    if ((atomo = analex()) == NUMERO || atomo == NOME || atomo == STRING || atomo == PC || atomo == NUL)
        coloca_numero(atomo);
    else if (atomo == AP)
    {
        hl_exp();
        if ((atomo = analex()) != FP)
        {
            volta_atomo(atomo);
            longjmp(sys_pilha, FECHA_PARENTESES_ESPERADO);
        }
    }
    else if (atomo == ERRO)
        longjmp(sys_pilha, causa);
    else
    {
        volta_atomo(atomo);
        longjmp(sys_pilha, NUMERO_ESPERADO);
    }

    while ((mlt = analex()) >= VEZES && mlt <= SHR)
    {
        if ((atomo = analex()) == NUMERO || atomo == NOME || atomo == STRING || atomo == PC || atomo == NUL)
            coloca_numero(atomo);
        else if (atomo == AP)
        {
            hl_exp();
            if ((atomo = analex()) != FP)
            {
                volta_atomo(atomo);
                longjmp(sys_pilha, FECHA_PARENTESES_ESPERADO);
            }
        }
        else if (atomo == ERRO)
            longjmp(sys_pilha, causa);
        else
        {
            volta_atomo(atomo);
            longjmp(sys_pilha, NUMERO_ESPERADO);
        }
        multiplica(mlt);
    }
    volta_atomo(mlt);
}

/*****************************************************************************
    complementa ():

    Complemeta numero na pilha de operadores.
*****************************************************************************/

void complementa(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor = ~pilha[ppilha - 1].valor;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        longjmp(sys_pilha, NOT_INVALIDO);
}

/*****************************************************************************
    troca_sinal ():

    Inverte sinal de numero na pilha de operadores.
*****************************************************************************/

void troca_sinal(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor = -pilha[ppilha - 1].valor;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        longjmp(sys_pilha, UNARIO_MENOS_INVALIDO);
}

/*****************************************************************************
    high ():

    Pega parte mais significativa de numero na pilha de operadores.
*****************************************************************************/

void high(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor >> 8) & 0xff;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        longjmp(sys_pilha, HIGH_COM_NUMERO_RELOCAVEL);
}

/*****************************************************************************
    low ():

    Pega parte menos significativa de numero na pilha de operadores.
*****************************************************************************/

void low(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor &= 0xff;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        longjmp(sys_pilha, LOW_COM_NUMERO_RELOCAVEL);
}

/*****************************************************************************
    and ():

    Faz and bit a bit de numeros na pilha de operadores.
*****************************************************************************/

void and(void)
{
    if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        pilha[ppilha - 1].valor = pilha[ppilha - 1].valor & pilha[ppilha].valor;
    }
    else if (!parsing)
        longjmp(sys_pilha, AND_INVALIDO);
    else if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido)
        longjmp(sys_pilha, AND_INVALIDO);
    else
        pilha[--ppilha - 1].atr = 0;
}

/*****************************************************************************
    or_xor ():

    Faz or ou xor bit a bit de numeros na pilha de operadores.
*****************************************************************************/

void or_xor(int op)
{
    if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        if (op == OR)
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor | pilha[ppilha].valor;
        else
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor ^ pilha[ppilha].valor;
    }
    else if (!parsing)
        longjmp(sys_pilha, (op == OR) ? OR_INVALIDO : XOR_INVALIDO);
    else if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido)
        longjmp(sys_pilha, (op == OR) ? OR_INVALIDO : XOR_INVALIDO);
    else
        pilha[--ppilha - 1].atr = 0;
}

/*****************************************************************************
    soma ():

    Faz soma de numeros na pilha de operadores.
*****************************************************************************/

void soma(void)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        pilha[--ppilha - 1].atr = 0;
        return;
    }

    if (pilha[ppilha - 2].atr == pilha[ppilha - 1].atr)
        if (pilha[ppilha - 1].atr & ABS)
        {
            ppilha--;
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor + pilha[ppilha].valor;
            return;
        }
        else
            longjmp(sys_pilha, SOMA_INVALIDA);
    else if (pilha[ppilha - 2].atr & ABS || pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        if (pilha[ppilha - 1].atr & ABS)
        {
            pilha[ppilha - 1].s = pilha[ppilha].s;
            pilha[ppilha - 1].atr = pilha[ppilha].atr;
        }
        pilha[ppilha - 1].valor = pilha[ppilha - 1].valor + pilha[ppilha].valor;
        return;
    }
    else
        longjmp(sys_pilha, SOMA_INVALIDA);
}

/*****************************************************************************
    subtrai ():

    Faz subtracao de numeros na pilha de operadores.
*****************************************************************************/

void subtrai(void)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        if (pilha[ppilha - 1].atr & extrn)
            longjmp(sys_pilha, SUBTRACAO_INVALIDA);
        else
        {
            pilha[--ppilha - 1].atr = 0;
            return;
        }
    }

    if (pilha[ppilha - 2].atr == pilha[ppilha - 1].atr)
        if (pilha[ppilha - 1].atr & extrn)
            longjmp(sys_pilha, SUBTRACAO_INVALIDA);
        else
        {
            ppilha--;
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor - pilha[ppilha].valor;
            pilha[ppilha - 1].atr = ABS | definido;
            return;
        }
    if (pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        pilha[ppilha - 1].valor = pilha[ppilha - 1].valor - pilha[ppilha].valor;
        return;
    }
    else
        longjmp(sys_pilha, SUBTRACAO_INVALIDA);
}

/*****************************************************************************
    relacao ():

    Faz compracao de numeros na pilha de operadores.
*****************************************************************************/

void relacao(int r)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        if ((pilha[ppilha - 1].atr | pilha[ppilha - 2].atr) & extrn)
            longjmp(sys_pilha, COMPARACAO_INVALIDA);
        else
        {
            pilha[--ppilha - 1].atr = 0;
            return;
        }
    }

    if (pilha[ppilha - 2].atr != pilha[ppilha - 1].atr)
        longjmp(sys_pilha, COMPARACAO_INVALIDA);
    if (pilha[ppilha - 1].atr & extrn)
        longjmp(sys_pilha, COMPARACAO_INVALIDA);
    switch (r)
    {
    case EQ:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor == pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case NE:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor != pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case GT:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor > pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case GE:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor >= pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case LT:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor < pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case LE:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor <= pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    default:
        longjmp(sys_pilha, ERRO_INTERNO);
    }
}

/*****************************************************************************
    coloca_numero ():

    Coloca numero na pilha de operadores.
*****************************************************************************/

void coloca_numero(int n)
{
    t_atomo atomo;

    if (ppilha == comp_pilha)
        longjmp(sys_pilha, MUITOS_OPERANDOS_PENDENTES);
    else
        switch (n)
        {
        case NUMERO:
            pilha[ppilha].valor = valor;
            pilha[ppilha++].atr = ABS | definido;
            break;

        case NOME:
            if (!(simbolo->atrib & definido))
                if (parsing)
                    pilha[ppilha++].atr = 0;
                else
                    longjmp(sys_pilha, IDENTIFICADOR_INDEFINIDO);
            else
            {
                if (simbolo->atrib & extrn)
                    pilha[ppilha].valor = 0;
                else
                    pilha[ppilha].valor = simbolo->valor;
                pilha[ppilha].atr = (simbolo->atrib & (ABS | COD | DAT | extrn | definido)) & ~_MACLIB;
                pilha[ppilha++].s = simbolo;
            }
            break;

        case STRING:
            if (valor == 1)
            {
                pilha[ppilha].valor = string[0];
                pilha[ppilha++].atr = ABS | definido;
            }
            else
                longjmp(sys_pilha, NUMERO_ESPERADO);
            break;

        case PC:
            pilha[ppilha].valor = pc;
            pilha[ppilha++].atr = alocatual | definido;
            break;

        case NUL:
            if ((atomo = analex()) == EOL || atomo == EOA)
                pilha[ppilha].valor = ~0;
            else
                pilha[ppilha].valor = 0;
            pilha[ppilha++].atr = ABS | definido;
            volta_atomo(atomo);
            fim_da_linha();
            break;
        }
}

/*****************************************************************************
    multiplica ():

    Faz operaroes com mesma precedencia que multiplicacao na pilha de
operadores.
*****************************************************************************/

void multiplica(int m)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        if ((pilha[ppilha - 1].atr | pilha[ppilha - 2].atr) & (COD | DAT | extrn))
        {
            switch (m)
            {
            case VEZES:
                m = MULTIPLICACAO_INVALIDA;
                break;

            case DIVIDE:
            case MOD:
                m = DIVISAO_INVALIDA;
                break;

            default:
                m = DESLOCAMENTO_INVALIDO;
                break;
            }
            longjmp(sys_pilha, m);
        }
        else if (((pilha[ppilha - 1].atr & (definido | ABS)) == (definido | ABS)) && (pilha[ppilha - 1].valor == 0) && m == DIVIDE)
            longjmp(sys_pilha, DIVISAO_POR_ZERO);
        else
        {
            pilha[--ppilha - 1].atr = 0;
            return;
        }
    }

    if (!(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & ABS))
    {
        switch (m)
        {
        case VEZES:
            m = MULTIPLICACAO_INVALIDA;
            break;

        case DIVIDE:
        case MOD:
            m = DIVISAO_INVALIDA;
            break;

        default:
            m = DESLOCAMENTO_INVALIDO;
            break;
        }
        longjmp(sys_pilha, m);
    }

    switch (m)
    {
    case VEZES:
        ppilha--;
        pilha[ppilha - 1].valor *= pilha[ppilha].valor;
        break;

    case DIVIDE:
        if (pilha[--ppilha].valor)
            pilha[ppilha - 1].valor /= pilha[ppilha].valor;
        else
            longjmp(sys_pilha, DIVISAO_POR_ZERO);
        break;

    case MOD:
        ppilha--;
        pilha[ppilha - 1].valor %= pilha[ppilha].valor;
        break;

    case SHL:
        ppilha--;
        pilha[ppilha - 1].valor <<= pilha[ppilha].valor;
        break;

    case SHR:
        ppilha--;
        pilha[ppilha - 1].valor >>= pilha[ppilha].valor;
        break;

    default:
        longjmp(sys_pilha, ERRO_INTERNO);
    }
}

#else /* bottom_up definido */

#define comp_pilha_o comp_pilha /* numero maximo de operadores pendentes */
#define comp_pilha_e comp_pilha /* numero maximo de estados pendentes */
#define U_MENOS A				/* atomo para diferenciar '-' unario de binario */

unsigned int estado;		   /* estado do parser */
int ppilha_o;				   /* ponteiro para pilha de operadores */
int ppilha_e;				   /* ponteiro de pilha de estados */
t_atomo pilha_o[comp_pilha_o]; /* pilha de operadores */
int pilha_e[comp_pilha_e];	   /* pilha de estados */
int continua;
int (*(funcao[]))(void) =
    {
        estado0,
        estado1,
        estado2,
        estado3,
        estado4,
        estado5,
        estado6};

/*****************************************************************************
    expressao ():

    Calcula expressao.
*****************************************************************************/

int expressao(void)
{
    parsing = 0;
    return expr();
}

/*****************************************************************************
    parse_expr ():

    Faz parsing de expressao.
*****************************************************************************/

int parse_expr(void)
{
    parsing = 1;
    return expr();
}

/*****************************************************************************
    expr ():

    Retorna 0 se expressao OK e !0 se algum erro.

    Expressoes pode ter os seguintes operandos, com prioridade crescente:

                                HIGH		LOW
                                OR			XOR
                                    AND
                                    NOT
                            EQ	NE	LE	GE	LT	GT
                                    +	-
                            *	/	MOD  SHL  SHR

    Observando as rotinas de expressao se bottom_up nao definido, observa-se
que elas reconhecem a seguinte gramatica:

    S: [hl] A {or A}
    A: [NOT] R {AND [NOT] R}
    R: C {r C}
    C: [-] T {<+ | -> T}
    T: <n | (S)> {m <n | (S)>}

    Nas expressoes acima < e > sao usados para agrupamento.
    hl pode ser HIGH ou LOW.
    or pode ser OR ou XOR.
    r pode ser EQ, NE, LE, GE, LT ou GT.
    m pode ser *, /, MOD, SHL ou SHR.
    n pode ser numero, identificador, etc...

    Simplificando as expressoes acima em uma maquina de estados recursiva,
o seguinte reconhecedor S e' criado:

    ESTADO		hl	NOT	-	(	n	r	AND	m	)	+	or		S
    0				1	2		3	6	4
    1					2		3	6	4
    2							3	6	4
    3								6	4
    4 (f)						3			2	1		3		3	1
    5														4
    6																		5

    O estado 4 e' o estado final.

    Com a tabela acima e' possivel criar uma maquina de pilha razoavelmente
simples. Notar que o estado 6 pode ser eliminado se o estado que receber
um '(' chamar a maquina recursivamente e colocar o estado 5 como endereco
de retorno.

    A estrategia das acoes semanticas e' a seguinte:
. ha' duas pilhas: uma para os operandos e outra para operadores. Inicialmente
a pilha de operandos esta' vazia e a pilha de operadores contem um '(', mar-
cando pilha vazia.
. caso chegue um numero ele e' imediatamente colocado na pilha de operandos
. caso chegue um operador unario (unario '-', NOT, HIGH ou LOW) ele e' colo-
cado na pilha de operadores.
. caso chegue um '(' ele e' colocado na pilha e uma nova recursao do reconhe-
cedor e' chamada.
. caso chegue um operador binario ele e' comparado com o operador do topo
da pilha de operadores. Caso a precedencia do novo operador for menor ou igual
ao operador da pilha e' feita a operacao na pilha. Isto se repete ate' que
a precedencia do novo operador for maior que o do topo da pilha. Neste caso
o novo operador e' empilhado.
. caso chegue um atomo atomo invalido no estado 4 (final) as operacoes
pendentes sao todas efetuadas e o valor da expressao fica no topo da pilha.
. no estado 5, chegando um ')' e' desempilhado o valor da pilha de operadores
(que deve ser ')')
. a precedencia dos operadores e' a seguinte (em ordem decrescente):
    m
    unario -
    + -
    r
    NOT
    AND
    or
    hl
    (		(que na verdade significa pilha vazia)

*****************************************************************************/

int expr(void)
{
    int erro; /* erro retornado por rotinas de acoes semanticas */

    estado = 0;
    ppilha = 0; /* pilha de operandos = vazia */
    ppilha_o = 1;
    pilha_o[0] = AP; /* topo da pilha de operadores = AP */
    ppilha_e = 1;
    pilha_e[0] = 6; /* pseudo estado: termina reconhecimento */
    continua = 1;

    while (continua)
    {
        if (estado > 6)
            return ERRO_INTERNO;
        if (erro = (*(funcao[estado]))())
            return erro;
    }
    return 0;
}

/*****************************************************************************
    estado0 ():
*****************************************************************************/

int estado0(void)
{
    t_atomo atomo;
    int erro;

    switch (atomo = analex())
    {
    case HIGH:
    case LOW:
        if (erro = empilha_operador(atomo))
            return erro;
        estado = 1;
        break;

    case NOT:
        if (erro = empilha_operador(atomo))
            return erro;
        estado = 2;
        break;

    case MENOS:
        if (erro = empilha_operador(U_MENOS))
            return erro;
        estado = 3;
        break;

    case AP:
        if (erro = empilha_operador(atomo))
            return erro;
        if (ppilha_e == comp_pilha_e)
            return MUITOS_NIVEIS_DE_PARENTESES;
        pilha_e[ppilha_e++] = 5; /* estado de retorno */
        estado = 0;				 /* faz outra recursao */
        break;

    case NUMERO:
    case NOME:
    case STRING:
    case PC:
    case NUL:
        if (erro = coloca_numero(atomo))
            return erro;
        estado = 4;
        break;

    case ERRO:
        return causa;

    default:
        volta_atomo(atomo);
        return NUMERO_ESPERADO;
    }
    return 0;
}

/*****************************************************************************
    estado1 ():
*****************************************************************************/

int estado1(void)
{
    t_atomo atomo;
    int erro;

    switch (atomo = analex())
    {
    case NOT:
        if (erro = empilha_operador(atomo))
            return erro;
        estado = 2;
        break;

    case MENOS:
        if (erro = empilha_operador(U_MENOS))
            return erro;
        estado = 3;
        break;

    case AP:
        if (erro = empilha_operador(atomo))
            return erro;
        if (ppilha_e == comp_pilha_e)
            return MUITOS_NIVEIS_DE_PARENTESES;
        pilha_e[ppilha_e++] = 5; /* estado de retorno */
        estado = 0;				 /* faz outra recursao */
        break;

    case NUMERO:
    case NOME:
    case STRING:
    case PC:
    case NUL:
        if (erro = coloca_numero(atomo))
            return erro;
        estado = 4;
        break;

    case ERRO:
        return causa;

    default:
        volta_atomo(atomo);
        return NUMERO_ESPERADO;
    }
    return 0;
}

/*****************************************************************************
    estado2 ():
*****************************************************************************/

int estado2(void)
{
    t_atomo atomo;
    int erro;

    switch (atomo = analex())
    {
    case MENOS:
        if (erro = empilha_operador(U_MENOS))
            return erro;
        estado = 3;
        break;

    case AP:
        if (erro = empilha_operador(atomo))
            return erro;
        if (ppilha_e == comp_pilha_e)
            return MUITOS_NIVEIS_DE_PARENTESES;
        pilha_e[ppilha_e++] = 5; /* estado de retorno */
        estado = 0;				 /* faz outra recursao */
        break;

    case NUMERO:
    case NOME:
    case STRING:
    case PC:
    case NUL:
        if (erro = coloca_numero(atomo))
            return erro;
        estado = 4;
        break;

    case ERRO:
        return causa;

    default:
        volta_atomo(atomo);
        return NUMERO_ESPERADO;
    }
    return 0;
}

/*****************************************************************************
    estado3 ():
*****************************************************************************/

int estado3(void)
{
    t_atomo atomo;
    int erro;

    switch (atomo = analex())
    {
    case AP:
        if (erro = empilha_operador(atomo))
            return erro;
        if (ppilha_e == comp_pilha_e)
            return MUITOS_NIVEIS_DE_PARENTESES;
        pilha_e[ppilha_e++] = 5; /* estado de retorno */
        estado = 0;				 /* faz outra recursao */
        break;

    case NUMERO:
    case NOME:
    case STRING:
    case PC:
    case NUL:
        if (erro = coloca_numero(atomo))
            return erro;
        estado = 4;
        break;

    case ERRO:
        return causa;

    default:
        volta_atomo(atomo);
        return NUMERO_ESPERADO;
    }
    return 0;
}

/*****************************************************************************
    estado4 ():
*****************************************************************************/

int estado4(void)
{
    t_atomo atomo;
    int erro;

    switch (atomo = analex())
    {
    case MENOS:
    case MAIS:

    case VEZES:
    case DIVIDE:
    case MOD:
    case SHL:
    case SHR:
        if (erro = reduz_expressao(atomo))
            return erro;
        estado = 3;
        break;

    case EQ:
    case NE:
    case LE:
    case GE:
    case LT:
    case GT:
        if (erro = reduz_expressao(atomo))
            return erro;
        estado = 2;
        break;

    case OR:
    case XOR:

    case AND:
        if (erro = reduz_expressao(atomo))
            return erro;
        estado = 1;
        break;

    default:
        volta_atomo(atomo);
        if (erro = termina_expressao())
            return erro;
        estado = pilha_e[--ppilha_e];
    }
    return 0;
}

/*****************************************************************************
    estado5 ():
*****************************************************************************/

int estado5(void)
{
    t_atomo atomo;

    switch (atomo = analex())
    {
    case FP:
        if (pilha_o[--ppilha_o] != AP)
            return ERRO_INTERNO;
        estado = 4;
        break;

    case ERRO:
        return causa;

    default:
        volta_atomo(atomo);
        return FECHA_PARENTESES_ESPERADO;
    }
    return 0;
}

/*****************************************************************************
    estado6 ():
*****************************************************************************/

int estado6(void)
{
    if (ppilha_o != 1 || ppilha != 1)
        return ERRO_INTERNO;
    else
        continua = 0; /* termina reconhecimento */
    return 0;
}

/*****************************************************************************
    empilha_operador ():

    Coloca operador na pilha de operadores.
*****************************************************************************/

int empilha_operador(t_atomo op)
{
    if (ppilha_o == comp_pilha_o)
        return MUITOS_OPERANDOS_PENDENTES;
    pilha_o[ppilha_o++] = op;
    return 0;
}

/*****************************************************************************
    coloca_numero ():

    Coloca numero na pilha de operadores.
*****************************************************************************/

int coloca_numero(t_atomo n)
{
    t_atomo atomo;

    if (ppilha == comp_pilha)
        return MUITOS_OPERANDOS_PENDENTES;
    else
        switch (n)
        {
        case NUMERO:
            pilha[ppilha].valor = valor;
            pilha[ppilha++].atr = ABS | definido;
            break;

        case NOME:
            if (!(simbolo->atrib & definido))
                if (parsing)
                    pilha[ppilha++].atr = 0;
                else
                    return IDENTIFICADOR_INDEFINIDO;
            else
            {
                if (simbolo->atrib & extrn)
                    pilha[ppilha].valor = 0;
                else
                    pilha[ppilha].valor = simbolo->valor;
                pilha[ppilha].atr = (simbolo->atrib & (ABS | COD | DAT | extrn | definido)) & ~_MACLIB;
                pilha[ppilha++].s = simbolo;
            }
            break;

        case STRING:
            if (valor == 1)
            {
                pilha[ppilha].valor = string[0];
                pilha[ppilha++].atr = ABS | definido;
            }
            else
                return NUMERO_ESPERADO;
            break;

        case PC:
            pilha[ppilha].valor = pc;
            pilha[ppilha++].atr = alocatual | definido;
            break;

        case NUL:
            if ((atomo = analex()) == EOL || atomo == EOA)
                pilha[ppilha].valor = ~0;
            else
                pilha[ppilha].valor = 0;
            pilha[ppilha++].atr = ABS | definido;
            volta_atomo(atomo);
            fim_da_linha();
            break;
        }
    return 0;
}

/*****************************************************************************
    reduz_expressao ():

    Reduz a expressao pendente, dadas pilhas de operadores, operandos e
atomo atual. Enquanto a prioridade da operacao atual for menor ou igual `a
do topo da pilha, continua reduzindo. No final coloca a operacao atual no topo
da pilha.
*****************************************************************************/

int reduz_expressao(t_atomo op)
{
    int erro;

    while (prioridade(pilha_o[ppilha_o - 1]) <= prioridade(op))
        if (erro = faz_op_pendente())
            return erro;
    return empilha_operador(op);
}

/*****************************************************************************
    prioridade ()

    Volta com um numero indicando a prioridade. 0 quer dizer maior prioridade
de todas.
*****************************************************************************/

int prioridade(t_atomo op)
{
    switch (op)
    {
    case VEZES:
    case DIVIDE:
    case MOD:
    case SHL:
    case SHR:
        return 0;

    case U_MENOS:
        return 1;

    case MAIS:
    case MENOS:
        return 2;

    case EQ:
    case NE:
    case GT:
    case GE:
    case LT:
    case LE:
        return 3;

    case NOT:
        return 4;

    case AND:
        return 5;

    case OR:
    case XOR:
        return 6;

    case HIGH:
    case LOW:
        return 7;

    default:
        return 8;
    }
}

/*****************************************************************************
    termina_expressao ():

    Efetiva todas as operacoes pendentes na pilha de operandos.
*****************************************************************************/

int termina_expressao(void)
{
    int erro;

    while (pilha_o[ppilha_o - 1] != AP)
        if (erro = faz_op_pendente())
            return erro;
    return 0;
}

/*****************************************************************************
    faz_op_pendente ():

    Realiza operacao pendente no topo da pilha de operadores, dada a pilha
de operandos.
*****************************************************************************/

int faz_op_pendente(void)
{
    switch (pilha_o[--ppilha_o]) /* pilha de operandos */
    {
    case U_MENOS:
        return troca_sinal();

    case VEZES:
    case DIVIDE:
    case MOD:
    case SHL:
    case SHR:
        return multiplica(pilha_o[ppilha_o]);

    case MAIS:
        return soma();

    case MENOS:
        return subtrai();

    case EQ:
    case NE:
    case GT:
    case GE:
    case LT:
    case LE:
        return relacao(pilha_o[ppilha_o]);

    case NOT:
        return complementa();

    case AND:
        return and();

    case OR:
    case XOR:
        return or_xor(pilha_o[ppilha_o]);

    case HIGH:
        return high();

    case LOW:
        return low();

    default:
        return ERRO_INTERNO;
    }
}

/*****************************************************************************
    troca_sinal ():

    Inverte sinal de numero na pilha de operadores.
*****************************************************************************/

int troca_sinal(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor = -pilha[ppilha - 1].valor;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        return UNARIO_MENOS_INVALIDO;
    return 0;
}

/*****************************************************************************
    multiplica ():

    Faz operaroes com mesma precedencia que multiplicacao na pilha de
operadores.
*****************************************************************************/

int multiplica(int m)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        if ((pilha[ppilha - 1].atr | pilha[ppilha - 2].atr) & (COD | DAT | extrn))
        {
            switch (m)
            {
            case VEZES:
                m = MULTIPLICACAO_INVALIDA;
                break;

            case DIVIDE:
            case MOD:
                m = DIVISAO_INVALIDA;
                break;

            default:
                m = DESLOCAMENTO_INVALIDO;
                break;
            }
            return m;
        }
        else if (((pilha[ppilha - 1].atr & (definido | ABS)) == (definido | ABS)) && (pilha[ppilha - 1].valor == 0) && m == DIVIDE)
            return DIVISAO_POR_ZERO;
        else
        {
            pilha[--ppilha - 1].atr = 0;
            return 0;
        }
    }

    if (!(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & ABS))
    {
        switch (m)
        {
        case VEZES:
            m = MULTIPLICACAO_INVALIDA;
            break;

        case DIVIDE:
        case MOD:
            m = DIVISAO_INVALIDA;
            break;

        default:
            m = DESLOCAMENTO_INVALIDO;
            break;
        }
        return m;
    }

    switch (m)
    {
    case VEZES:
        ppilha--;
        pilha[ppilha - 1].valor *= pilha[ppilha].valor;
        break;

    case DIVIDE:
        if (pilha[--ppilha].valor)
            pilha[ppilha - 1].valor /= pilha[ppilha].valor;
        else
            return DIVISAO_POR_ZERO;
        break;

    case MOD:
        ppilha--;
        pilha[ppilha - 1].valor %= pilha[ppilha].valor;
        break;

    case SHL:
        ppilha--;
        pilha[ppilha - 1].valor <<= pilha[ppilha].valor;
        break;

    case SHR:
        ppilha--;
        pilha[ppilha - 1].valor >>= pilha[ppilha].valor;
        break;

    default:
        return ERRO_INTERNO;
    }
    return 0;
}

/*****************************************************************************
    soma ():

    Faz soma de numeros na pilha de operadores.
*****************************************************************************/

int soma(void)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        pilha[--ppilha - 1].atr = 0;
        return 0;
    }

    if (pilha[ppilha - 2].atr == pilha[ppilha - 1].atr)
        if (pilha[ppilha - 1].atr & ABS)
        {
            ppilha--;
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor + pilha[ppilha].valor;
        }
        else
            return SOMA_INVALIDA;
    else if (pilha[ppilha - 2].atr & ABS || pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        if (pilha[ppilha - 1].atr & ABS)
        {
            pilha[ppilha - 1].s = pilha[ppilha].s;
            pilha[ppilha - 1].atr = pilha[ppilha].atr;
        }
        pilha[ppilha - 1].valor = pilha[ppilha - 1].valor + pilha[ppilha].valor;
    }
    else
        return SOMA_INVALIDA;
    return 0;
}

/*****************************************************************************
    subtrai ():

    Faz subtracao de numeros na pilha de operadores.
*****************************************************************************/

int subtrai(void)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        if (pilha[ppilha - 1].atr & extrn)
            return SUBTRACAO_INVALIDA;
        else
        {
            pilha[--ppilha - 1].atr = 0;
            return 0;
        }
    }

    if (pilha[ppilha - 2].atr == pilha[ppilha - 1].atr)
        if (pilha[ppilha - 1].atr & extrn)
            return SUBTRACAO_INVALIDA;
        else
        {
            ppilha--;
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor - pilha[ppilha].valor;
            pilha[ppilha - 1].atr = ABS | definido;
            return 0;
        }
    if (pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        pilha[ppilha - 1].valor = pilha[ppilha - 1].valor - pilha[ppilha].valor;
        return 0;
    }
    else
        return SUBTRACAO_INVALIDA;
}

/*****************************************************************************
    relacao ():

    Faz compracao de numeros na pilha de operadores.
*****************************************************************************/

int relacao(int r)
{
    if (parsing && !(pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido))
    {
        if ((pilha[ppilha - 1].atr | pilha[ppilha - 2].atr) & extrn)
            return COMPARACAO_INVALIDA;
        else
        {
            pilha[--ppilha - 1].atr = 0;
            return 0;
        }
    }

    if (pilha[ppilha - 2].atr != pilha[ppilha - 1].atr)
        return COMPARACAO_INVALIDA;
    if (pilha[ppilha - 1].atr & extrn)
        return COMPARACAO_INVALIDA;
    switch (r)
    {
    case EQ:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor == pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case NE:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor != pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case GT:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor > pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case GE:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor >= pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case LT:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor < pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    case LE:
        ppilha--;
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor <= pilha[ppilha].valor) ? ~0 : 0;
        pilha[ppilha - 1].atr = ABS | definido;
        break;

    default:
        return ERRO_INTERNO;
    }
    return 0;
}

/*****************************************************************************
    complementa ():

    Complemeta numero na pilha de operadores.
*****************************************************************************/

int complementa(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor = ~pilha[ppilha - 1].valor;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        return NOT_INVALIDO;
    return 0;
}

/*****************************************************************************
    and ():

    Faz and bit a bit de numeros na pilha de operadores.
*****************************************************************************/

int and(void)
{
    if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        pilha[ppilha - 1].valor = pilha[ppilha - 1].valor & pilha[ppilha].valor;
    }
    else if (!parsing)
        return AND_INVALIDO;
    else if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido)
        return AND_INVALIDO;
    else
        pilha[--ppilha - 1].atr = 0;
    return 0;
}

/*****************************************************************************
    or_xor ():

    Faz or ou xor bit a bit de numeros na pilha de operadores.
*****************************************************************************/

int or_xor(int op)
{
    if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & ABS)
    {
        ppilha--;
        if (op == OR)
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor | pilha[ppilha].valor;
        else
            pilha[ppilha - 1].valor = pilha[ppilha - 1].valor ^ pilha[ppilha].valor;
    }
    else if (!parsing)
        return op == OR ? OR_INVALIDO : XOR_INVALIDO;
    else if (pilha[ppilha - 2].atr & pilha[ppilha - 1].atr & definido)
        return op == OR ? OR_INVALIDO : XOR_INVALIDO;
    else
        pilha[--ppilha - 1].atr = 0;
    return 0;
}

/*****************************************************************************
    high ():

    Pega parte mais significativa de numero na pilha de operadores.
*****************************************************************************/

int high(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor = (pilha[ppilha - 1].valor >> 8) & 0xff;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        return HIGH_COM_NUMERO_RELOCAVEL;
    return 0;
}

/*****************************************************************************
    low ():

    Pega parte menos significativa de numero na pilha de operadores.
*****************************************************************************/

int low(void)
{
    if (pilha[ppilha - 1].atr & ABS)
        pilha[ppilha - 1].valor &= 0xff;
    else if ((pilha[ppilha - 1].atr & definido) || !parsing)
        return LOW_COM_NUMERO_RELOCAVEL;
    return 0;
}

#endif /* bottom_up */
