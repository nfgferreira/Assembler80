#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <setjmp.h>
#include "variaveis.h"
#include "protos.h"

/* definicao de caracteres validos para simbolos (fora letras e algarismos) */
#define isvalid(c) ((c) == '?' || (c) == '_' || (c) == '#')

char *falha_de_abertura = "NAO CONSEGUIU ABRIR ARQUIVO %s\n";

/******************* tabela de palavras reservadas **************************/

static preservada resa [] =
	{
	"A",			A,				i8080 | i8085 | z80 | z180,
	"ACI",		ACI,			i8080 | i8085 | z80 | z180,
	"ADC",		ADC,			i8080 | i8085 | z80 | z180,
	"ADD",		ADD,			i8080 | i8085 | z80 | z180,
	"ADI",		ADI,			i8080 | i8085 | z80 | z180,
	"ANA",		ANA,			i8080 | i8085 | z80 | z180,
	"AND",		AND,			i8080 | i8085 | z80 | z180,
	"ANI",		ANI,			i8080 | i8085 | z80 | z180,
	"ASEG",		ASEG,			i8080 | i8085 | z80 | z180
	};

static preservada resb [] =
	{
	"B",			B,				i8080 | i8085 | z80 | z180,
	"BIT",		BIT,			z80 | z180,
	"BSET",		BSET,			z80 | z180
	};

static preservada resc [] =
	{
	"C",			C,				i8080 | i8085 | z80 | z180,
	"CALL",		CALL,			i8080 | i8085 | z80 | z180,
	"CC",			CC,			i8080 | i8085 | z80 | z180,
	"CM",			CM,			i8080 | i8085 | z80 | z180,
	"CMA",		CMA,			i8080 | i8085 | z80 | z180,
	"CMC",		CMC,			i8080 | i8085 | z80 | z180,
	"CMD",		CMD,			z80 | z180,
	"CMDR",		CMDR,			z80 | z180,
	"CMI",		CMI,			z80 | z180,
	"CMIR",		CMIR,			z80 | z180,
	"CMP",		CMP,			i8080 | i8085 | z80 | z180,
	"CNC",		CNC,			i8080 | i8085 | z80 | z180,
	"CNV",		CPO,			z80 | z180,
	"CNZ",		CNZ,			i8080 | i8085 | z80 | z180,
	"CP",			CP,			i8080 | i8085 | z80 | z180,
	"CPE",		CPE,			i8080 | i8085 | z80 | z180,
	"CPI",		CPI,			i8080 | i8085 | z80 | z180,
	"CPO",		CPO,			i8080 | i8085 | z80 | z180,
	"CSEG",		CSEG,			i8080 | i8085 | z80 | z180,
	"CV",			CPE,			z80 | z180,
	"CZ",			CZ,			i8080 | i8085 | z80 | z180
	};

static preservada resd [] =
	{
	"D",			D,				i8080 | i8085 | z80 | z180,
	"DAA",		DAA,			i8080 | i8085 | z80 | z180,
	"DAD",		DAD,			i8080 | i8085 | z80 | z180,
	"DADC",		DADC,			z80 | z180,
	"DADX",		DADX,			z80 | z180,
	"DADY",		DADY,			z80 | z180,
	"DB",			DB,			i8080 | i8085 | z80 | z180,
	"DCR",		DCR,			i8080 | i8085 | z80 | z180,
	"DCX",		DCX,			i8080 | i8085 | z80 | z180,
	"DI",			DI,			i8080 | i8085 | z80 | z180,
	"DJNZ",		DJNZ,			z80 | z180,
	"DS",			DS,			i8080 | i8085 | z80 | z180,
	"DSBB",		DSBB,			z80 | z180,
	"DSEG",		DSEG,			i8080 | i8085 | z80 | z180,
	"DW",			DW,			i8080 | i8085 | z80 | z180
	};

static preservada rese [] =
	{
	"E",			E,				i8080 | i8085 | z80 | z180,
	"EI",			EI,			i8080 | i8085 | z80 | z180,
	"ELSE",		ELSE,			i8080 | i8085 | z80 | z180,
	"END",		END,			i8080 | i8085 | z80 | z180,
	"ENDIF",		ENDIF,		i8080 | i8085 | z80 | z180,
	"ENDM",		ENDM,			i8080 | i8085 | z80 | z180,
	"EQ",			EQ,			i8080 | i8085 | z80 | z180,
	"EQU",		EQU,			i8080 | i8085 | z80 | z180,
	"EXITM",		EXITM,		i8080 | i8085 | z80 | z180,
	"EXTRN",		EXTRN,		i8080 | i8085 | z80 | z180
	};

static preservada resg [] =
	{
	"GE",			GE,			i8080 | i8085 | z80 | z180,
	"GT",			GT,			i8080 | i8085 | z80 | z180
	};

static preservada resh [] =
	{
	"H",			H,				i8080 | i8085 | z80 | z180,
	"HIGH",		HIGH,			i8080 | i8085 | z80 | z180,
	"HLT",		HLT,			i8080 | i8085 | z80 | z180
	};

static preservada resi [] =
	{
	"I",			I,				z80 | z180,
	"IF",			IF,			i8080 | i8085 | z80 | z180,
	"IFDEF",		IFDEF,		i8080 | i8085 | z80 | z180,
	"IFNDEF",	IFNDEF,		i8080 | i8085 | z80 | z180,
	"IM",			IM,			z80 | z180,
	"IN",			IN,			i8080 | i8085 | z80 | z180,
	"IN0",		IN0,			z180,
	"INC",		INC,			z80 | z180,
	"IND",		IND,			z80 | z180,
	"INDR",		INDR,			z80 | z180,
	"INI",		INI,			z80 | z180,
	"INIR",		INIR,			z80 | z180,
	"INR",		INR,			i8080 | i8085 | z80 | z180,
	"INX",		INX,			i8080 | i8085 | z80 | z180,
	"IRP",		IRP,			i8080 | i8085 | z80 | z180,
	"IX",			IX,			z80 | z180,
	"IY",			IY,			z80 | z180
	};

static preservada resj [] =
	{
	"JC",			JC,			i8080 | i8085 | z80 | z180,
	"JCR",		JCR,			z80 | z180,
	"JM",			JM,			i8080 | i8085 | z80 | z180,
	"JMP",		JMP,			i8080 | i8085 | z80 | z180,
	"JNC",		JNC,			i8080 | i8085 | z80 | z180,
	"JNCR",		JNCR,			z80 | z180,
	"JNV",		JPO,			z80 | z180,
	"JNZ",		JNZ,			i8080 | i8085 | z80 | z180,
	"JNZR",		JNZR,			z80 | z180,
	"JP",			JP,			i8080 | i8085 | z80 | z180,
	"JPE",		JPE,			i8080 | i8085 | z80 | z180,
	"JPO",		JPO,			i8080 | i8085 | z80 | z180,
	"JR",			JR,			z80 | z180,
	"JV",			JPE,			z80 | z180,
	"JZ",			JZ,			i8080 | i8085 | z80 | z180,
	"JZR",		JZR,			z80 | z180
	};

static preservada resl [] =
	{
	"L",			L,				i8080 | i8085 | z80 | z180,
	"LBCD",		LBCD,			z80 | z180,
	"LDA",		LDA,			i8080 | i8085 | z80 | z180,
	"LDAX",		LDAX,			i8080 | i8085 | z80 | z180,
	"LDD",		LDD,			z80 | z180,
	"LDDR",		LDDR,			z80 | z180,
	"LDED",		LDED,			z80 | z180,
	"LDI",		LDI,			z80 | z180,
	"LDIR",		LDIR,			z80 | z180,
	"LE",			LE,			i8080 | i8085 | z80 | z180,
	"LHLD",		LHLD,			i8080 | i8085 | z80 | z180,
	"LIXD",		LIXD,			z80 | z180,
	"LIYD",		LIYD,			z80 | z180,
	"LOCAL",		LOCAL,		i8080 | i8085 | z80 | z180,
	"LOW",		LOW,			i8080 | i8085 | z80 | z180,
	"LSPD",		LSPD,			z80 | z180,
	"LT",			LT,			i8080 | i8085 | z80 | z180,
	"LXI",		LXI,			i8080 | i8085 | z80 | z180
	};

static preservada resm [] =
	{
	"M",			M,				i8080 | i8085 | z80 | z180,
	"MACLIB",	MACLIB,		i8080 | i8085 | z80 | z180,
	"MACRO",		MACRO,		i8080 | i8085 | z80 | z180,
	"MLT",		MLT,			z180,
	"MOD",		MOD,			i8080 | i8085 | z80 | z180,
	"MOV",		MOV,			i8080 | i8085 | z80 | z180,
	"MVI",		MVI,			i8080 | i8085 | z80 | z180
	};

static preservada resn [] =
	{
	"NAME",		NAME,			i8080 | i8085 | z80 | z180,
	"NE",			NE,			i8080 | i8085 | z80 | z180,
	"NEG",		NEG,			z80 | z180,
	"NOP",		NOP,			i8080 | i8085 | z80 | z180,
	"NOT",		NOT,			i8080 | i8085 | z80 | z180,
	"NUL",		NUL,			i8080 | i8085 | z80 | z180
	};

static preservada reso [] =
	{
	"OR",			OR,			i8080 | i8085 | z80 | z180,
	"ORA",		ORA,			i8080 | i8085 | z80 | z180,
	"ORG",		ORG,			i8080 | i8085 | z80 | z180,
	"ORI",		ORI,			i8080 | i8085 | z80 | z180,
	"OTDM",		OTDM,			z180,
	"OTDMR",		OTDMR,		z180,
	"OTDR",		OTDR,			z80 | z180,
	"OTIM",		OTIM,			z180,
	"OTIMR",		OTIMR,		z180,
	"OTIR",		OTIR,			z80 | z180,
	"OUT",		OUT,			i8080 | i8085 | z80 | z180,
	"OUT0",		OUT0,			z180,
	"OUTC",		OUTC,			z80 | z180,
	"OUTD",		OUTD,			z80 | z180,
	"OUTI",		OUTI,			z80 | z180
	};

static preservada resp [] =
	{
	"PCHL",		PCHL,			i8080 | i8085 | z80 | z180,
	"PCIX",		PCIX,			z80 | z180,
	"PCIY",		PCIY,			z80 | z180,
	"POP",		POP,			i8080 | i8085 | z80 | z180,
	"PSW",		PSW,			i8080 | i8085 | z80 | z180,
	"PUBLIC",	PUBLIC,		i8080 | i8085 | z80 | z180,
	"PUSH",		PUSH,			i8080 | i8085 | z80 | z180
	};

static preservada resr [] =
	{
	"R",			R,				z80 | z180,
	"RAL",		RAL,			i8080 | i8085 | z80 | z180,
	"RAR",		RAR,			i8080 | i8085 | z80 | z180,
	"RC",			RC,			i8080 | i8085 | z80 | z180,
	"REPT",		REPT,			i8080 | i8085 | z80 | z180,
	"RES",		RES,			z80 | z180,
	"RET",		RET,			i8080 | i8085 | z80 | z180,
	"RETI",		RETI,			z80 | z180,
	"RETN",		RETN,			z80 | z180,
	"RIM",		RIM,			i8085,
	"RLC",		RLC,			i8080 | i8085 | z80 | z180,
	"RLD",		RLD,			z80 | z180,
	"RM",			RM,			i8080 | i8085 | z80 | z180,
	"RNC",		RNC,			i8080 | i8085 | z80 | z180,
	"RNV",		RPO,			z80 | z180,
	"RNZ",		RNZ,			i8080 | i8085 | z80 | z180,
	"RP",			RP,			i8080 | i8085 | z80 | z180,
	"RPE",		RPE,			i8080 | i8085 | z80 | z180,
	"RPO",		RPO,			i8080 | i8085 | z80 | z180,
	"RRC",		RRC,			i8080 | i8085 | z80 | z180,
	"RRD",		RRD,			z80 | z180,
	"RST",		RST,			i8080 | i8085 | z80 | z180,
	"RV",			RPE,			z80 | z180,
	"RZ",			RZ,			i8080 | i8085 | z80 | z180
	};

static preservada ress [] =
	{
	"SBB",		SBB,			i8080 | i8085 | z80 | z180,
	"SBCD",		SBCD,			z80 | z180,
	"SBI",		SBI,			i8080 | i8085 | z80 | z180,
	"SDED",		SDED,			z80 | z180,
	"SET",		SET,			i8080 | i8085 | z80 | z180,
	"SHL",		SHL,			i8080 | i8085 | z80 | z180,
	"SHLD",		SHLD,			i8080 | i8085 | z80 | z180,
	"SHR",		SHR,			i8080 | i8085 | z80 | z180,
	"SIM",		SIM,			i8085,
	"SIXD",		SIXD,			z80 | z180,
	"SIYD",		SIYD,			z80 | z180,
	"SLA",		SLA,			z80 | z180,
	"SLP",		SLP,			z180,
	"SP",			SP,			i8080 | i8085 | z80 | z180,
	"SPHL",		SPHL,			i8080 | i8085 | z80 | z180,
	"SPIX",		SPIX,			z80 | z180,
	"SPIY",		SPIY,			z80 | z180,
	"SRA",		SRA,			z80 | z180,
	"SRL",		SRL,			z80 | z180,
	"SSPD",		SSPD,			z80 | z180,
	"STA",		STA,			i8080 | i8085 | z80 | z180,
	"STAX",		STAX,			i8080 | i8085 | z80 | z180,
	"STC",		STC,			i8080 | i8085 | z80 | z180,
	"SUB",		SUB,			i8080 | i8085 | z80 | z180,
	"SUI",		SUI,			i8080 | i8085 | z80 | z180
	};

static preservada rest [] =
	{
	"TST",		TST,			z180,
	"TSTIO",		TSTIO,		z180
	};

static preservada resx [] =
	{
	"XCHG",		XCHG,			i8080 | i8085 | z80 | z180,
	"XCHX",		XCHX,			z80 | z180,
	"XOR",		XOR,			i8080 | i8085 | z80 | z180,
	"XPSW",		XPSW,			z80 | z180,
	"XRA",		XRA,			i8080 | i8085 | z80 | z180,
	"XRI",		XRI,			i8080 | i8085 | z80 | z180,
	"XTHL",		XTHL,			i8080 | i8085 | z80 | z180
	};

static struct
	{
	preservada *pr;
	int h;
	} tppr ['Z' - 'A' + 1] =
		{
		resa, sizeof (resa) / sizeof (preservada) - 1,
		resb, sizeof (resb) / sizeof (preservada) - 1,
		resc, sizeof (resc) / sizeof (preservada) - 1,
		resd, sizeof (resd) / sizeof (preservada) - 1,
		rese, sizeof (rese) / sizeof (preservada) - 1,
		NULL, 0,
		resg, sizeof (resg) / sizeof (preservada) - 1,
		resh, sizeof (resh) / sizeof (preservada) - 1,
		resi, sizeof (resi) / sizeof (preservada) - 1,
		resj, sizeof (resj) / sizeof (preservada) - 1,
		NULL, 0,
		resl, sizeof (resl) / sizeof (preservada) - 1,
		resm, sizeof (resm) / sizeof (preservada) - 1,
		resn, sizeof (resn) / sizeof (preservada) - 1,
		reso, sizeof (reso) / sizeof (preservada) - 1,
		resp, sizeof (resp) / sizeof (preservada) - 1,
		NULL, 0,
		resr, sizeof (resr) / sizeof (preservada) - 1,
		ress, sizeof (ress) / sizeof (preservada) - 1,
		rest, sizeof (rest) / sizeof (preservada) - 1,
		NULL, 0,
		NULL, 0,
		NULL, 0,
		resx, sizeof (resx) / sizeof (preservada) - 1,
		NULL, 0,
		NULL, 0
		};

/*****************************************************************************

	analex ():

	Devolve tipo de atomo da entrada (ver comentario em analex1 ())

*****************************************************************************/

t_atomo analex (void)
	{
	t_atomo atomo;
	int i;
	char nome [comp_max + 1];
	uintptr_t ls;

	if (voltou_atomo)
		{
		voltou_atomo = 0;
		if (!ppmac && atomo_devolvido == EOL && !fim_da_linha_falso)
			linha++;
		return atomo_devolvido;
		}

	if (passo_1)
		{
		atomo = analex1 ();
		if (!maclib && !expressao_em_parametro && ((!definindo_macro && monta) || (atomo == EOL) || (atomo == EOA)))
			{
			salva ((ppmac && (atomo == EOL)) ? EOLM : atomo);
			switch (atomo)
				{
			case STRING:
				salva ((t_atomo) (valor & 0xff));
				salva ((t_atomo) ((valor >> 8) & 0xff));
				for (i = 0; i < valor; i++)
					salva (string [i]);
				break;

/*			case EQU:
/*			case MACRO:
/*			case SET:
/*				if (simbolo == NULL)
/*					{
/*					salva ('\0');
/*					break;
/*					}
/*
/*			case LABEL:
/*			case NOME:
/*				i = 0;
/*				do
/*					salva (simbolo -> nome [i]);
/*				while (simbolo -> nome [i++] != '\0');
/*				break;
*/
			case EQU:
			case LABEL:
			case MACRO:
			case NOME:
			case SET:
				for (i = 0, ls = (uintptr_t) simbolo; i < sizeof (simb *); i++, ls >>= 8)
					salva ((t_atomo) ls);
				break;

			case NUMERO:
				salva ((t_atomo) (valor & 0xff));
				salva ((t_atomo) ((valor >> 8) & 0xff));
				break;

			case ERRO:
				salva ((t_atomo) (causa & 0xff));
				salva ((t_atomo) ((causa >> 8) & 0xff));
				break;
				}
			}
		return atomo;
		}

/******************************** passo 2 ***********************************/

	fim_da_linha_falso = 0;
	switch (atomo = pega_atomo ())
		{
	case STRING:
		valor = 0xff & pega_atomo ();
		valor += ((int) pega_atomo ()) << 8;
		for (i = 0; i < valor; i++)
			string [i] = pega_atomo ();
		break;

	case EQU:
	case MACRO:
	case SET:
	case LABEL:
	case NOME:
/*		i = 0;
/*		do
/*			nome [i] = pega_atomo ();
/*		while (nome [i++] != '\0');
/*		if (nome [0] != '\0')
/*			ver_nome (nome, i - 1);
/*		else
/*			simbolo = NULL;
/*		break;
*/
		for (i = 0, ls = 0L; i < sizeof (simb *); i++)
			ls = (ls >> 8) | (((uintptr_t) pega_atomo ()) << (sizeof(uintptr_t) * 8 - 8));
		simbolo = (simb *) ls;
		break;

	case NUMERO:
		valor = 0xff & pega_atomo ();
		valor += ((int) pega_atomo ()) << 8;
		break;

	case EOLM:
		fim_da_linha_falso = 1;
		atomo = EOL;
		break;

	case EOL:
		linha++;
		break;

	case ERRO:
		causa = 0xff & pega_atomo ();
		causa += ((int) pega_atomo ()) << 8;
		break;
		}
	return atomo;
	}

/*****************************************************************************
	salva ():

	Salva atomo na fila.

*****************************************************************************/

void salva (t_atomo atomo)
	{
	if (!(atomos_colocados & ((1 << n_rot_aloc_ats) - 1)))
		{
		if (naloc >= max_aloc)
			erro_fatal (PROGRAMA_MUITO_GRANDE);
		if ((tab_ats [naloc] = (t_atomo *) malloc (1 << n_rot_aloc_ats)) == NULL)
			erro_fatal (MEMORIA_INSUFICIENTE);
		atomos_colocados = 0;
		naloc++;
		}

	*(tab_ats [naloc - 1] + atomos_colocados++) = atomo;
	}

/*****************************************************************************
	pega_atomo ():

	Recupera atomo salvo na memoria.

*****************************************************************************/

t_atomo pega_atomo (void)
	{
	if (!(atomos2_colocados & ((1 << n_rot_aloc_ats) - 1)))
		{
		n2aloc++;
		atomos2_colocados = 0;
		}
	if (n2aloc >= naloc && atomos2_colocados > atomos_colocados)
		erro_fatal (ERRO_INTERNO);
	return *(tab_ats [n2aloc - 1] + atomos2_colocados++);
	}

/*****************************************************************************
	desaloca ():

	Desaloca memoria e fecha arquivos.

*****************************************************************************/

void desaloca (void)
	{
	int i;

	for (i = 0; i < naloc; i++)
		free ((void *) tab_ats [i]);
	for (i = 0; i < nmem_aloc; i++)
		free ((void *) tab_mem_aloc [i]);
	for (i = 0; i < nset_simb; i++)
		free ((void *) aloc_simb [i]);
	for (i = 0; i < nmcaloc; i++)
		free ((void *) tab_end_chars [i]);

	if (arqrel)
		close (arqrel);
	if (arq_sym)
		close (arq_sym);
	if (arq)
		close (arq);
	if (maclib)
		close (sarq);
	if (tratando_pch)
		close (arq_pch);
	}

/*****************************************************************************
	analex1 ():

	Devolve tipo de atomo da entrada:
		(								AP
		)								FP
		*								VEZES
		/								DIVIDE
		+								MAIS
		-								MENOS
		,								VIRGULA
		\n								EOL
		_EOF							EOA
		$								PC
		string						STRING (string em string e numero de bytes em valor)
		numero						NUMERO (valor voltado em valor)
		palavra reservada			numero correspondente a palavra (ver res)
		identificador				NOME (ponteiro para nome volta em simbolo)
		erro							ERRO (causa volta em causa)
											NUMERO_MUITO_GRANDE
											TABELA_DE_SIMBOLOS_CHEIA

*****************************************************************************/

t_atomo analex1 (void)
	{
	char c, tipo_string;
	t_atomo atomo;
	int i;
	char palavra [comp_max + 1];

	while ((c = le_car ()) == '\t' || c == ' ');
	switch (c)
		{
	case '(':
		return AP;

	case ')':
		return FP;

	case '*':
		return VEZES;

	case '/':
		return DIVIDE;

	case '+':
		return MAIS;

	case '-':
		return MENOS;

	case ',':
		return VIRGULA;

	case '\n':
		return EOL;

	case _EOF:
		return EOA;

	case '$':
		return PC;

	case '\"':
	case '\'':
		tipo_string = c;
		valor = 0;
		while (1)
			{
			converte = 0;
			while (valor < (sizeof string) - 1 && (c = le_car ()) != '\\' && c != '\'' && c != '\"' && c != '\n')
				string [valor++] = c;
			converte = 1;
			if (valor >= (sizeof string) - 1)
				{
				causa = STRING_MUITO_GRANDE;
				return ERRO;
				}
			switch (c)
				{
			case '\"':
			case '\'':
				if (c == tipo_string)
					{
					if (c == '\"')
						string [valor++] = '\0';
					return STRING;
					}
				string [valor++] = c;
				break;

			case '\n':
				devolve ();
				causa = STRING_NAO_FOI_FECHADA;
				return ERRO;

			case '\\':
				converte = 0;
				switch (c = le_car ())
					{
				case '0':
					string [valor++] = '\0';
					break;

				case 'a':
					string [valor++] = '\a';
					break;

				case 'b':
					string [valor++] = '\b';
					break;

				case 'f':
					string [valor++] = '\f';
					break;

				case 'n':
					string [valor++] = '\n';
					break;

				case 'r':
					string [valor++] = '\r';
					break;

				case 't':
					string [valor++] = '\t';
					break;

				case 'v':
					string [valor++] = '\v';
					break;

				case '\'':
					string [valor++] = '\'';
					break;

				case '\"':
					string [valor++] = '\"';
					break;

				case '\\':
					string [valor++] = '\\';
					break;

				case '\n':
					devolve ();
					converte = 1;
					causa = STRING_NAO_FOI_FECHADA;
					return ERRO;

				default:
					string [valor++] = c;
					break;
					}
				}
			}

	case ';':
		despreza_linha = 1;
		while (le_car () != '\n');
		return EOL;

	default:
		if (isdigit (c))
			return numero (c);

		if (isalnum (c) || isvalid (c))
			return nome (c);

		causa = CARACTER_INESPERADO;
		return ERRO;
		}
	}

/*****************************************************************************
	nome ():

	Trata nome, voltando palavra reservada ou identificador.

*****************************************************************************/

t_atomo nome (char c)
	{
	char palavra [comp_max + 1];
	int i;
	int rc, pc;
	int line;
	simb *s;
	t_atomo tipo, at;
	unsigned int mc;

	palavra [0] = c;
	i = 1;
	while (isalnum (c = le_car ()) || isvalid (c))
 		if (i < comp_max)
			palavra [i++] = c;
	devolve ();
	palavra [i] = '\0';
	if (reservada (palavra, &tipo))
		return tipo;
	ver_nome (palavra, i);

	if ((c = le_car ()) == ':')
		return LABEL;
	else
		{
		devolve ();
		if (!entra)
			return NOME;
		else
			{
			entra = 0;
			rc = resta_car;
			pc = prox_car;
			line = linha;
			mc = mac_char;
			s = simbolo;
			at = analex1 ();
			entra = 1;
			if (at == EQU || at == MACRO || at == SET)
				{
				simbolo = s;
				return at;
				}
			else
				{
				resta_car = rc;
				prox_car = pc;
				linha = line;
				mac_char = mc;
				simbolo = s;
				return NOME;
				}
			}
		}
	}

/*****************************************************************************
	numero ():

	Trata numero, voltando o valor.

*****************************************************************************/

t_atomo numero (char c)
	{
	char num [17];
	int i;

	if (c != '0')
		{
		num [0] = c;
		i = 1;
		}
	else
		i = 0;
	while (isalnum (c = le_car ()) && i < (sizeof num))
		if (i || c != '0')
			num [i++] = c;
	devolve ();
	if (i == (sizeof num) && isalnum (c))
		{
		causa = NUMERO_MUITO_GRANDE;
		return ERRO;
		}
	if (!i)
		{
		num [0] = '0';
		i = 1;
		}
	if (conv_num (num, i, &valor))
		return NUMERO;
	return ERRO;
	}

/*****************************************************************************
	conv_num ():

	Converte numero da entrada para binario.

*****************************************************************************/

int conv_num (char *num, int comp, unsigned int *valor)
	{
	switch (num [comp - 1])
		{
	case 'B':
		return num_bin (num, comp - 1, valor);

	case 'O':
	case 'Q':
		return num_oct (num, comp - 1, valor);

	case 'D':
		return num_dec (num, comp - 1, valor);

	case 'H':
		return num_hex (num, comp - 1, valor);

	default:
		return num_dec (num, comp, valor);
		}
	}

/*****************************************************************************
	num_bin ():

	Converte numero em ascii binario.

*****************************************************************************/

int num_bin (char *num, int comp, unsigned int *valor)
	{
	int i;
	unsigned long x;

	x = 0L;
	for (i = 0; i < comp; i++)
		{
		if (num [i] == '0' || num [i] == '1')
			x = (x << 1) + num [i] - '0';
		else
			{
			causa = NUMERO_INVALIDO;
			return 0;
			}
		if (x & ~0xffffL)
			{
			causa = NUMERO_MUITO_GRANDE;
			return 0;
			}
		}
	*valor = x;
	return 1;
	}

/*****************************************************************************
	num_oct ():

	Converte numero em ascii octal.

*****************************************************************************/

int num_oct (char *num, int comp, unsigned int *valor)
	{
	int i;
	unsigned long x;

	x = 0L;
	for (i = 0; i < comp; i++)
		{
		if (num [i] >= '0' && num [i] <= '7')
			x = (x << 3) + num [i] - '0';
		else
			{
			causa = NUMERO_INVALIDO;
			return 0;
			}
		if (x & ~0xffffL)
			{
			causa = NUMERO_MUITO_GRANDE;
			return 0;
			}
		}
	*valor = x;
	return 1;
	}

/*****************************************************************************
	num_dec ():

	Converte numero em ascii decimal.

*****************************************************************************/

int num_dec (char *num, int comp, unsigned int *valor)
	{
	int i;
	unsigned long x;

	x = 0L;
	for (i = 0; i < comp; i++)
		{
		if (isdigit (num [i]))
			x = x * 10 + num [i] - '0';
		else
			{
			causa = NUMERO_INVALIDO;
			return 0;
			}
		if (x & ~0xffffL)
			{
			causa = NUMERO_MUITO_GRANDE;
			return 0;
			}
		}
	*valor = x;
	return 1;
	}

/*****************************************************************************
	num_hex ():

	Converte numero em ascii hexa-decimal.

*****************************************************************************/

int num_hex (char *num, int comp, unsigned int *valor)
	{
	int i;
	unsigned long x;

	x = 0L;
	for (i = 0; i < comp; i++)
		{
		if (isdigit(num [i]))
			x = (x << 4) + num [i] - '0';
		else
			if (num [i] >= 'A' && num [i] <= 'F')
				x = (x << 4) + num [i] + 10 - 'A';
			else
				{
				causa = NUMERO_INVALIDO;
				return 0;
				}
		if (x & ~0xffffL)
			{
			causa = NUMERO_MUITO_GRANDE;
			return 0;
			}
		}
	*valor = x;
	return 1;
	}

/*****************************************************************************
	reservada ():

	Verifica se palavra e' palavra reservada.

*****************************************************************************/

int reservada (char palavra [], t_atomo *tipo)
	{
	preservada *p;
	int l, h, m;
	int comp;

	if (*palavra > 'Z' || *palavra < 'A')
		return 0;		/* garantia */

	if ((p = tppr [h = *palavra - 'A'].pr) == NULL)
		return 0;
	l = 0;
	h = tppr [h].h;
	while (h >= l)
		{
		if (!(comp = strcmp (palavra, p [m = (h + l) >> 1].pr)))
			{
			*tipo = p [m].i;
			return cpu & p [m].permissao;
			}
		else if (comp < 0)
			h = m - 1;
		else
			l = m + 1;
		}
	return 0;
	}

/*****************************************************************************
	limpa_ts ():

	Inicializa tabela de simbolos.

*****************************************************************************/

void limpa_ts (void)
	{
	int i;

	for (i = 0; i < inic_simb_size; i++)
		inic_simbolo [i] = NULL;
	}

/*****************************************************************************
	ver_nome ():

	Procura ou coloca nome na tabela de simbolos.

*****************************************************************************/

void ver_nome (char *s, int comp)
	{
/* 1.30 */
/*	int i, ent;
*/
	int i;
	unsigned int ent;

	simb *smb;

	if (!monta)
		{
		simbolo = NULL;
		return;			/* nao gasta memoria se nao vai usar simbolo */
		}

/* 1.30 */
/*	for (ent = i = 0; i < comp; i++)
/*		ent = hash (ent, s[i]);
*/
	for (ent = i = 0; i < comp; i++)
		ent = _rotr (ent + s [i], 6);

/* 1.30 */
/*	if (inic_simbolo [ent &= (inic_simb_size - 1)] != NULL)
*/
	if (inic_simbolo [ent %= inic_simb_size] != NULL)

		{
		for (smb = inic_simbolo [ent]; smb -> next_simbolo != NULL; smb = smb -> next_simbolo)
			if (!strcmp (s, smb -> nome))
				{
				simbolo = smb;
				return;
				}
		if (!strcmp (s, smb -> nome))
			simbolo = smb;
		else
			{

/* 1.30 */
/*			simbolo = smb -> next_simbolo = get_simb ();*/
			simbolo = get_simb ();
			simbolo -> next_simbolo = inic_simbolo [ent];
			inic_simbolo [ent] = simbolo;

			strcpy (simbolo -> nome, s);
			}
		}
	else
		{
		simbolo = inic_simbolo [ent] = get_simb ();
		strcpy (simbolo -> nome, s);
		}
	}

/*****************************************************************************
	get_simb ():

	Devolve ponteiro para novo simbolo.

*****************************************************************************/

simb *get_simb (void)
	{
	simb *smb;

	if (!resta_simb)
		{
		if (nset_simb >= max_simb_aloc)
			erro_fatal (TABELA_DE_SIMBOLOS_CHEIA);
		if ((aloc_simb [nset_simb] = (simb *) malloc ((sizeof (simb)) * nsimb_aloc)) == NULL)
			erro_fatal (MEMORIA_INSUFICIENTE);
		resta_simb = nsimb_aloc;
		nset_simb++;
		}
	smb = aloc_simb [nset_simb - 1] + nsimb_aloc - resta_simb--;
	smb -> atrib = 0;
	smb -> next_simbolo = NULL;
	return smb;
	}

/* 1.30 */
/*/*****************************************************************************
/*	le_car ():
/*
/*	Le proximo caracter do arquivo de entrada.
/*
/******************************************************************************/
/*
/*char le_car (void)
/*	{
/*	char c;
/*
/*	if (despreza_linha)
/*		{
/*		prox_car += resta_car;
/*		despreza_linha = resta_car = 0;
/*		if (!ppmac)
/*			linha++;
/*		return '\n';
/*		}
/*
/*	if (!resta_car)
/*		{
/*		prox_car = 0;
/*		next_linha = buf_leit;
/*		do
/*			{
/*			c = *(next_linha++) = get_car ();		/* le do arquivo */
/*			resta_car++;
/*			}
/*		while (c != '\n' && c != _EOF && resta_car < sizeof buf_leit);
/*		if (c == _EOF && resta_car > 1)
/*			*(next_linha - 1) = '\n';	/* linha sempre termina com fim de linha */
/*		if (c != '\n' && c != _EOF)
/*			{
/*			buf_leit [(sizeof buf_leit) - 1] = '\n';
/*			while ((c = get_car ()) != '\n' && c != _EOF);
/*			}
/*		next_linha = buf_leit;
/*		if (ppmac)					/* expande linha para colocar parametros de macro */
/*			expande_linha ();
/*		}
/*
/*	resta_car--;
/*	if (!ppmac && next_linha [prox_car] == '\n')
/*		linha++;
/*	if ((c = next_linha [prox_car++]) >= 'a' && c <= 'z' && converte)
/*		c += ('A' - 'a');
/*	return c;
/*	}
*/

/*****************************************************************************
	expande_linha ():

	Transforma linha, pegando parametros de macro.

*****************************************************************************/

void expande_linha (void)
	{
	char *ce, *par;
	int resta, nc;
	char n [comp_max + 1];
	int i;
	int concatena;

	ce = buf_leit;				/* ponteiro para caracteres a serem lidos */
	nc = 0;						/* numero de caracteres de saida */
	resta = resta_car;		/* numero de caracteres que resta para serem lidos */
	concatena = 0;				/* nao chegou '&' */

	while (resta && (nc < (sizeof linha_macro) - (sizeof buf_leit)))
		if (*(ce) == '&')		/* concatenacao: pode vir nome */
			{
			if (concatena)
				linha_macro [nc++] = '&';
			concatena = 1;
			ce++;
			resta--;
			}
		else
			{
			if (isalpha (*ce) || isvalid (*ce))
				{
				for (i = 0; (isalnum (ce [i]) || isvalid (ce [i])) && i <= comp_max; i++)
					n [i] = (ce [i] >= 'a' && ce [i] <= 'z') ? ce [i] + 'A' - 'a' : ce [i];
				if (i > comp_max)		/* muitos caracteres: nao e' parametro */
					{
					if (concatena)
						linha_macro [nc++] = '&';
					do
						{
						linha_macro [nc++] = *(ce++);
						resta--;
						}
					while (isalnum (*ce) || isvalid (*ce));
					}
				else
					{
					n [i] = '\0';
					if ((par = m_parametro (n)) != NULL)	/* e' parametro */
						{
						resta -= i;
						ce += i;
						while (*par != '\0')
							linha_macro [nc++] = *(par++);
						if (resta && *ce == '&')
							{
							ce++;			/* tira & de concatenacao se foi parametro */
							resta--;
							}
						}
					else
						{
						if (concatena)
							linha_macro [nc++] = '&';
						do
							{
							linha_macro [nc++] = *(ce++);
							resta--;
							}
						while (isalnum (*ce) || isvalid (*ce));
						}
					}
				}
			else
				{
				if (concatena)
					linha_macro [nc++] = '&';
				linha_macro [nc++] = *(ce++);
				resta--;
				}
			concatena = 0;
			}

	if (concatena)
		linha_macro [nc++] = '&';

	if (linha_macro [nc - 1] != '\n')
		linha_macro [nc] = '\n';

	resta_car = nc;
	next_linha = linha_macro;
	}

/*****************************************************************************
	m_parametro ():

	Indica se string dada e' parametro da macro sendo expandida. Caso seja vol-
ta com localizacao na fila de parametros. Caso nao seja, volta -1.

*****************************************************************************/

char *m_parametro (char *n)
	{
	int i, j;
	static char l [7];

	for (i = 0, j = mac_pp; i < mac_np; i++)
		if (!strcmp (n, &macro_par [j]))
			if (i + 1 > nparm)
				{
				l [0] = '\0';
				return l;
				}
			else
				{
				if (exp_tipo_macro == 1)
					i += m_loops;
				for (j = 0; i; i--)
					while (mpar [j++] != '\0');
				return &mpar [j];
				}
		else
			while (macro_par [j++] != '\0');

	for (i = 0, j = 0; i < mac_nl; i++)
		if (!strcmp (n, &nomes_locais [j]))
			{
			while (nomes_locais [j++] != '\0');
			hexa (l + 2, (((int) nomes_locais [j]) & 0xff) + ((((int) nomes_locais [j + 1]) << 8) & 0xff00));
			l [0] = l [1] = '?';
			return l;
			}
		else
			{
			while (nomes_locais [j++] != '\0');
			j += 2;					/* pula numero da variavel local */
			}
	return NULL;
	}

/*****************************************************************************
	hexa ():

	Dado buffer e numero inteiro, converte em formato hexa-decimal.

*****************************************************************************/

void hexa (char *s, int n)
	{
	int i;

	i = 4;
	do
		{
		s [--i] = (n & 0xf) >= 0xa ? (n & 0xf) + 'A' - 0xa : (n & 0xf) + '0';
		n >>= 4;
		}
	while (i);
	s [4] = '\0';
	}

/* 1.30 */
/*/*****************************************************************************
/*	get_car ():
/*
/*	Le proximo caracter do arquivo de entrada.
/*
/******************************************************************************/
/*
/*char get_car (void)
/*	{
/*	char c;
/*
/*	if (ppmac)
/*		{
/*		c = *(tab_end_chars [(mac_char >> n_rot_macro_char) & cmsk_rot_macro_char]
/*				+ (mac_char & msk_rot_macro_char));
/*		mac_char++;
/*		return c;
/*		}
/*
/*	if (!num_car)
/*		{
/*		num_car = read (arq, buffer_leitura, sizeof buffer_leitura);
/*		le_cont = 0;
/*		if (!num_car)
/*			return _EOF;
/*		}
/*	num_car--;
/*	return buffer_leitura [le_cont++];
/*	}
*/

/*****************************************************************************
	devolve ():

	Devolve caracter para ser lido novamente.

*****************************************************************************/

void devolve (void)
	{
	resta_car++;
	if (next_linha [--prox_car] == '\n' && !ppmac)
		linha--;
	}

/*****************************************************************************
	volta_atomo ()

	Devolve atomo.

*****************************************************************************/

void volta_atomo (t_atomo atomo)
	{
	atomo_devolvido = atomo;

	if (!ppmac && atomo_devolvido == EOL && !fim_da_linha_falso)
		linha--;
	voltou_atomo = 1;
	}

/*****************************************************************************
	inic_lex ()

	Inicializa analisador lexico.

*****************************************************************************/

void inic_lex (char *nome)
	{
	tratando_pch = num_car = le_cont = 0;
	voltou_atomo = 0;
	entra = converte = 1;
	resta_car = 0;
	if (passo_1)
		{
		fim_da_linha_falso = 0;

#ifdef FAST_READ
		if ((arq = open (nome, O_BINARY | O_RDONLY)) == -1)
#else
		if ((arq = open (nome, O_TEXT | O_RDONLY)) == -1)
#endif

			{
			arq = 0;
			mprintf (falha_de_abertura, strupr (nome));
			grave ();
			}
		}
	else
		{
		n2aloc = 0;
		atomos2_colocados = 0;
		}
	}

/*****************************************************************************
	salva_lex ()

	Salva estado atual do analisador lexico.

*****************************************************************************/

void salva_lex (void)
	{
	int i;

	sarq = arq;
	snum_car = num_car;
	sle_cont = le_cont;
	svoltou_atomo = voltou_atomo;
	sconverte = converte;
	satomo_devolvido = atomo_devolvido;
	spguard = pguard;

	sresta_car = resta_car;
	snext_linha = next_linha;
	sprox_car = prox_car;
	for (i = 0; i < (resta_car + prox_car); i++)
		slinha_macro [i] = next_linha [i];

	for (i = 0; i < sizeof buffer_leitura; i++)
		sbuffer_leitura [i] = buffer_leitura [i];
	}

/*****************************************************************************
	rec_lex ()

	Recupera estado atual do analisador lexico.

*****************************************************************************/

void rec_lex (void)
	{
	int i;

	arq = sarq;
	num_car = snum_car;
	le_cont = sle_cont;
	voltou_atomo = svoltou_atomo;
	converte = sconverte;
	atomo_devolvido = satomo_devolvido;
	pguard = spguard;

	resta_car = sresta_car;
	next_linha = snext_linha;
	prox_car = sprox_car;
	for (i = 0; i < (resta_car + prox_car); i++)
		next_linha [i] = slinha_macro [i];

	for (i = 0; i < sizeof buffer_leitura; i++)
		buffer_leitura [i] = sbuffer_leitura [i];
	}

/*****************************************************************************
	mprintf ()

	Emula printf, so que muito menor.

*****************************************************************************/

void cdecl mprintf (char *s, ...)
	{
	int escape, pos_numero;
	char *aux, numero [(sizeof (long)) * 8 + 1];
	va_list arg;

	va_start (arg, s);		/* inicializa ponteiro p/ argumentos */
	for (escape = 0; *s != '\0'; s++)
		{
		switch (*s)
			{
		case '\\':
			if (!(escape = !escape))
				putchar (*s);
			break;

		case '%':
			if (escape)
				{
				putchar (*s);
				escape = 0;
				}
			else
				switch (*(++s))
					{
				case 'u':
				case 'i':
					aux = ultoa (((unsigned long) va_arg (arg, int)) & ((1L << (8 * (sizeof (int)))) - 1), numero, 10);
					pos_numero = 0;
					while (*aux != '\0')
						{
						if (pos_numero || *aux != '0')
							{
							pos_numero = 1;
							putchar (*aux);
							}
						aux++;
						}
					if (!pos_numero)
						putchar ('0');
					break;

				case 'c':
					putchar (va_arg (arg, int));
					break;

				case '\0':
				case '%':
					putchar ('%');
					break;

				case 's':
					aux = va_arg (arg, char *);
					while (*aux != '\0')
						putchar (*(aux++));
					break;

				default:
					putchar ('%');
					putchar (*s);
					}
			break;

		default:
			if (!escape)
				putchar (*s);
			else
				{
				escape = 0;
				switch (*(++s))
					{
				case 'a':
					putchar ('\a');
					break;

				case 'b':
					putchar ('\b');
					break;

				case 'f':
					putchar ('\f');
					break;

				case 'n':
					putchar ('\n');
					break;

				case 'r':
					putchar ('\r');
					break;

				case 't':
					putchar ('\t');
					break;

				case 'v':
					putchar ('\v');
					break;

				case '\0':
					putchar ('\\');
					break;

				default:
					putchar (*s);
					}
				}
			}
		}
	}

/* 1.30 - Estas duas rotinas abaixo substituem le_car antiga e get_car */

#ifdef FAST_READ

/*****************************************************************************
	le_car ():

	Le proximo caracter do arquivo de entrada.

*****************************************************************************/

char le_car (void)
	{
	char *paux0, *paux1, c;
	unsigned int rest_buf;

	if (despreza_linha)
		{
		prox_car += resta_car;
		despreza_linha = resta_car = 0;
		if (!ppmac)
			linha++;
		return '\n';
		}

	if (!resta_car)					/* caso nao ha' mais caracteres na linha atual */
		{
		while (1)						/* loop infinito: so' sai com break */
			{
			if (ppmac)					/* caso expandindo macro */
				{
				paux0 = tab_end_chars [(mac_char >> n_rot_macro_char) & cmsk_rot_macro_char]
					+ (mac_char & msk_rot_macro_char);
				rest_buf = msk_rot_macro_char + 1 - (mac_char & msk_rot_macro_char);
				}
			else							/* pega linha do buffer de leitura */
				{
				if (!num_car)			/* tem que fazer leitura do disco */
					{
					le_cont = 0;	/* indica do proximo caracter no buffer de leitura */
					if (!(num_car = read (arq, buffer_leitura, sizeof buffer_leitura)))
						{				/* caso nao conseguiu ler nada do disco */
						buf_leit [resta_car] = resta_car ? '\n' : _EOF;
						resta_car++;
						break;
						}
					}
				paux0 = buffer_leitura + le_cont;	/* aponta para primeiro caracter a ser lido */
				rest_buf = num_car;						/* numero de caracteres a pesquisar */
				}
			if ((paux1 = memchr (paux0, ppmac ? '\n' : '\r', rest_buf)) == NULL)	/* nao tem fim de linha no buffer */
				if (rest_buf + resta_car >= sizeof buf_leit - 1)	/* vai ultrapassar: limita */
					{
					memcpy (buf_leit + resta_car, paux0, sizeof buf_leit - resta_car - 1);
					buf_leit [(resta_car = sizeof buf_leit) - 1] = '\n';
					if (ppmac)
						mac_char += rest_buf;	/* aponta para proximo caracter */
					else
						num_car = 0;				/* indica que nao tem caracteres a serem lidos */
					final_de_linha ();			/* vai para o fim de linha no buffer de leitura */
					break;
					}
				else						/* nao tem fim de linha mas cabe */
					{
					memcpy (buf_leit + resta_car, paux0, rest_buf);
					resta_car += rest_buf;
					if (ppmac)
						mac_char += rest_buf;	/* aponta para proximo caracter */
					else
						num_car = 0;				/* indica que nao tem caracteres a serem lidos */
					}
			else							/* tem final de linha */
				{
				if (paux1 - paux0 + 1 + resta_car >= sizeof buf_leit - 1)
					{						/* limita */
					memcpy (buf_leit + resta_car, paux0, sizeof buf_leit - resta_car - 1);
					buf_leit [(resta_car = sizeof buf_leit) - 1] = '\n';
					}
				else						/* nao limita e termina: achou linha */
					{
					memcpy (buf_leit + resta_car, paux0, paux1 - paux0 + 1);
					resta_car += paux1 - paux0 + 1;
					if (!ppmac)
						buf_leit [resta_car - 1] = '\n';
					}
				if (ppmac)
					mac_char += paux1 - paux0 + 1;
				else
					{
					le_cont += paux1 - paux0 + 1;
					if (num_car -= paux1 - paux0 + 1)
						{
						num_car--;							/* pula '\n' */
						le_cont++;
						}
					else
						final_de_linha ();				/* vai ate' final de linha real */
					}
				break;
				}
			}
		prox_car = 0;					/* indice para proximo caracter na linha */
		next_linha = buf_leit;		/* ponteiro de onde colocar a leitura */
		if (ppmac)					/* expande linha para colocar parametros de macro */
			expande_linha ();
		}

/*	resta_car--;
/*	if (!ppmac && next_linha [prox_car] == '\n')
/*		linha++;
/*	if ((c = next_linha [prox_car++]) >= 'a' && c <= 'z' && converte)
/*		c += ('A' - 'a');
/*	return c;
*/

	resta_car--;
	if ((c = next_linha [prox_car++]) == '\n' && !ppmac)
		linha++;
	if (converte && c >= 'a' && c <= 'z')
		c += ('A' - 'a');
	return c;

	}

#else

/*****************************************************************************
	le_car ():

	Le proximo caracter do arquivo de entrada.

*****************************************************************************/

char le_car (void)
	{
	char *paux0, *paux1, c;
	unsigned int rest_buf;

	if (despreza_linha)
		{
		prox_car += resta_car;
		despreza_linha = resta_car = 0;
		if (!ppmac)
			linha++;
		return '\n';
		}

	if (!resta_car)					/* caso nao ha' mais caracteres na linha atual */
		{
		while (1)						/* loop infinito: so' sai com break */
			{
			if (ppmac)					/* caso expandindo macro */
				{
				paux0 = tab_end_chars [(mac_char >> n_rot_macro_char) & cmsk_rot_macro_char]
					+ (mac_char & msk_rot_macro_char);
				rest_buf = msk_rot_macro_char + 1 - (mac_char & msk_rot_macro_char);
				}
			else							/* pega linha do buffer de leitura */
				{
				if (!num_car)			/* tem que fazer leitura do disco */
					{
					le_cont = 0;	/* indica do proximo caracter no buffer de leitura */
					if (!(num_car = read (arq, buffer_leitura, sizeof buffer_leitura)))
						{				/* caso nao conseguiu ler nada do disco */
						buf_leit [resta_car] = resta_car ? '\n' : _EOF;
						resta_car++;
						break;
						}
					}
				paux0 = buffer_leitura + le_cont;	/* aponta para primeiro caracter a ser lido */
				rest_buf = num_car;						/* numero de caracteres a pesquisar */
				}
			if ((paux1 = memchr (paux0, '\n', rest_buf)) == NULL)	/* nao tem fim de linha no buffer */
				if (rest_buf + resta_car >= sizeof buf_leit - 1)	/* vai ultrapassar: limita */
					{
					memcpy (buf_leit + resta_car, paux0, sizeof buf_leit - resta_car - 1);
					buf_leit [(resta_car = sizeof buf_leit) - 1] = '\n';
					if (ppmac)
						mac_char += rest_buf;	/* aponta para proximo caracter */
					else
						num_car = 0;				/* indica que nao tem caracteres a serem lidos */
					final_de_linha ();			/* vai para o fim de linha no buffer de leitura */
					break;
					}
				else						/* nao tem fim de linha mas cabe */
					{
					memcpy (buf_leit + resta_car, paux0, rest_buf);
					resta_car += rest_buf;
					if (ppmac)
						mac_char += rest_buf;	/* aponta para proximo caracter */
					else
						num_car = 0;				/* indica que nao tem caracteres a serem lidos */
					}
			else							/* tem final de linha */
				{
				if (paux1 - paux0 + 1 + resta_car >= sizeof buf_leit - 1)
					{						/* limita */
					memcpy (buf_leit + resta_car, paux0, sizeof buf_leit - resta_car - 1);
					buf_leit [(resta_car = sizeof buf_leit) - 1] = '\n';
					}
				else						/* nao limita e termina: achou linha */
					{
					memcpy (buf_leit + resta_car, paux0, paux1 - paux0 + 1);
					resta_car += paux1 - paux0 + 1;
					}
				if (ppmac)
					mac_char += paux1 - paux0 + 1;
				else
					{
					num_car -= paux1 - paux0 + 1;
					le_cont += paux1 - paux0 + 1;
					}
				break;
				}
			}
		prox_car = 0;					/* indice para proximo caracter na linha */
		next_linha = buf_leit;		/* ponteiro de onde colocar a leitura */
		if (ppmac)					/* expande linha para colocar parametros de macro */
			expande_linha ();
		}

	resta_car--;
	if (!ppmac && next_linha [prox_car] == '\n')
		linha++;
	if ((c = next_linha [prox_car++]) >= 'a' && c <= 'z' && converte)
		c += ('A' - 'a');
	return c;
	}

#endif				/* FAST_READ */

/*****************************************************************************
	final_de_linha ()

	Busca final de linha no buffer de entrada atual, desprezando caracteres

*****************************************************************************/

void final_de_linha (void)
	{
	if (ppmac)					/* caso expandindo macro */
		{
		while (*(tab_end_chars [(mac_char >> n_rot_macro_char) & cmsk_rot_macro_char]
				+ (mac_char & msk_rot_macro_char)) != '\n')
			mac_char++;
		mac_char++;
		}
	else							/* caso lendo arquivo */
		while (1)
			{
			if (!num_car)
				{
				le_cont = 0;
				if (!(num_car = read (arq, buffer_leitura, sizeof buffer_leitura)))
					return;			/* atingiu final do arquivo: desista */
				}
			while (num_car)
				{
				num_car--;
				if (buffer_leitura [le_cont++] == '\n')
					return;
				}
			}
	}

