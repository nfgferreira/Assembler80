#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "variaveis.h"
#include "protos.h"

extern char *falha_de_abertura;

#define special_link 0
#define absolute 0
#define program_relative 1
#define data_relative 2
#define entry_symbol 0
#define program_name 2
#define chain_external 6
#define define_entry_point 7
#define external_plus_offset 9
#define define_data_size 10
#define set_location_counter 11
#define define_program_size 13
#define end_module 14
#define snome(i) ((aloc_simb [((i) >> nrot_aloc) & c_mask_aloc] + ((i) & mask_aloc)) -> nome)

void passo2 ()
	{
	mprintf ("PASSO 2\n");
	inicia_saida ();
	define_nome ();
	manda_nome ();
	manda_entry_symbols ();
	manda_comprimentos ();
	pgm_passo2 ();
	manda_publics ();
	manda_externos ();
	}

void pgm_passo2 (void)
	{
	t_atomo atomo;
	int aux;
	simb *memsimb;
	int passou;

	while (1)
		{
		while ((atomo = analex ()) == EOL);

		pula_linha = 0;

		switch (atomo)
			{
		case IFDEF:
		case IFNDEF:
		case IF:
		case ELSE:
		case ENDIF:
			fim_da_linha ();
			break;

		case ERRO:
			erro (causa);
			break;

		case END:
		case EOA:
			return;

		case IRP:
		case MACRO:
		case REPT:
			fim_da_linha ();
			break;

		case EQU:
			if ((memsimb = simbolo) == NULL)
				{
				erro (ERRO_DE_FASE);
				break;
				}
			if (!expressao ())	/* trata expressao */
				if (pilha [0].atr & extrn)
					erro (ERRO_DE_FASE);
				else
					{
					if (!(memsimb -> atrib & pilha [0].atr) || (memsimb -> valor != pilha [0].valor))
						erro (ERRO_DE_FASE);
					}
			else
				erro (ERRO_DE_FASE);
			break;

		case LABEL:
			if (!(simbolo -> atrib & definido) || !(simbolo -> atrib & alocatual) || (simbolo -> valor != pc))
				erro_linha (ERRO_DE_FASE);
			switch (atomo = analex ())
				{
			case END:
				return;

			case IRP:
			case REPT:
				fim_da_linha ();
				break;

			default:
				trata_ins2 (atomo);			/* trata possivel instrucao da linha */
				break;
				}
			break;

		case SET:
			if ((memsimb = simbolo) == NULL)
				{
				erro (INSTRUCAO_INVALIDA);
				break;
				}
			if (!(aux = expressao ()))	/* trata expressao */
				define_simbolo (memsimb, pilha [0].valor, pilha [0].atr);
			else
				erro (aux);
			break;

		default:
			trata_ins2 (atomo);			/* trata possivel instrucao da linha */
			break;
			}

		if (!pula_linha)
			if ((atomo = analex ()) != EOL && atomo != EOA)
				erro (FIM_DE_LINHA_ESPERADO);
			else
				if (atomo == EOA)
					volta_atomo (atomo);
		}
	}

void trata_ins2 (t_atomo atomo)
	{
	int comp;
	int aux;

	switch (atomo)
		{
	case ERRO:
		erro (causa);
		break;

	case EOA:
		volta_atomo (atomo);
		break;

	case EOL:
		volta_atomo (atomo);
		break;

	case ASEG:
		acerta_pont_old ();
		alocatual = ABS;
		acerta_pc ();
		manda_end ();
		break;

	case CSEG:
		acerta_pont_old ();
		alocatual = COD;
		acerta_pc ();
		manda_end ();
		break;

	case DSEG:
		acerta_pont_old ();
		alocatual = DAT;
		acerta_pc ();
		manda_end ();
		break;

	case MACLIB:
	case EXTRN:
	case PUBLIC:
	case ENDM:
	case EXITM:
	case LOCAL:
	case NAME:
		fim_da_linha ();
		break;

	case ORG:
		if (aux = expressao ())	/* trata expressao */
			erro (aux);
		else
			if (pilha [0].atr & ABS)
				{
				pc = pilha [0].valor;
				manda_end ();
				}
			else
				{
				erro (EXPRESSAO_INVALIDA);
				erro (PASSO_2_DIFERENTE_DO_1);
				}
		break;

	case DB:
		comp = 0;
		do
			{
			if ((atomo = analex ()) == STRING && valor != 1)
				{
				for (aux = 0; aux < valor; aux++)
					manda_byte ((int) string [aux]);
				comp += valor;
				pc += valor;
				}
			else
				{
				volta_atomo (atomo);
				if (aux = expressao ())	/* trata expressao */
					{
					erro_linha (aux);
					while ((atomo = analex ()) != VIRGULA && atomo != EOA && atomo != EOL);
					volta_atomo (atomo);
					}
				else
					if (!((pilha [0].atr & ABS) && (!(pilha [0].valor & ~0xff) || ((pilha [0].valor & ~0x7f) == ~0x7f))))
						erro_linha (PARAMETRO_INVALIDO);
					else
						manda_byte (pilha [0].valor);
				comp++;
				pc++;
				}
			}
		while ((atomo = analex ()) == VIRGULA);
		volta_atomo (atomo);
		pmem += comp;
		break;

	case DS:
		comp = 0;
		if (aux = expressao ())	/* trata expressao */
			erro (aux);
		else
			if (pilha [0].atr & ABS)
				{
				pc += pilha [0].valor;
				manda_end ();
				pc -= pilha [0].valor;
				comp = pilha [0].valor;
				}
			else
				{
				erro (EXPRESSAO_INVALIDA);
				erro (PASSO_2_DIFERENTE_DO_1);
				}
		pc += comp;
		break;

	case DW:
		comp = 0;
		do
			{
			if (!manda_endereco (0))
				{
				while ((atomo = analex ()) != VIRGULA && atomo != EOA && atomo != EOL);
				volta_atomo (atomo);
				}
			comp += 2;
			pc +=2;
			}
		while ((atomo = analex ()) == VIRGULA);
		volta_atomo (atomo);
		pmem += comp;
		break;

	case NOME:
		if (busca_macro (simbolo -> nome) -> atrib & definido)	/* e' macro */
			{
			pula_linha = 1;
			break;
			}
		else
			{
			erro (INSTRUCAO_INVALIDA);
			return;
			}

	default:
		manda_byte ((int) *(tab_mem_aloc [(pmem >> bit_rot_mem) & ((1 << (16 - bit_rot_mem)) - 1)] + (pmem & masc_mem)));
		if (inst_z80 ())
			manda_byte ((int) *(tab_mem_aloc [((pmem + 1) >> bit_rot_mem) & ((1 << (16 - bit_rot_mem)) - 1)] + ((pmem + 1) & masc_mem)));
		switch (atomo)
			{
		case MOV:
			if ((atomo = analex ()) == I || atomo == R)
				{
				analex ();		/* virgula */
				analex ();		/* A */
				comp = 2;
				break;
				}

			if (atomo == IX || atomo == IY)
				{
				if ((atomo = analex ()) == MAIS || atomo == MENOS)
					{
					if (atomo == MENOS)
						volta_atomo (atomo);
					exp_s_byte ();
					analex ();
					}
				else
					manda_byte (0);
				analex ();
				comp = 3;
				break;
				}

			analex ();		/* virgula */

			if ((atomo = analex ()) == I || atomo == R)
				{
				comp = 2;
				break;
				}

			if (atomo == IX || atomo == IY)
				{
				if ((atomo = analex ()) == MAIS || atomo == MENOS)
					{
					if (atomo == MENOS)
						volta_atomo (atomo);
					exp_s_byte ();
					}
				else
					{
					volta_atomo (atomo);
					manda_byte (0);
					}
				comp = 3;
				break;
				}

			comp = 1;
			break;

		case RST:
			while ((atomo = analex ()) != EOL && atomo != EOA);
			volta_atomo (atomo);
			comp = 1;
			break;

		case DAD:
		case LDAX:
		case STAX:
			analex ();				/* pula registrador */

		case CMA:
		case CMC:
		case DAA:
		case DI:
		case EI:
		case HLT:
		case NOP:
		case PCHL:
		case RC:
		case RET:
		case RIM:
		case RM:
		case RNC:
		case RNZ:
		case RP:
		case RPE:
		case RPO:
		case RZ:
		case SIM:
		case SPHL:
		case STC:
		case XCHG:
		case XCHX:
		case XPSW:
		case XTHL:
			comp = 1;
			break;

		case ACI:
		case ADI:
		case ANI:
		case CPI:
		case ORI:
		case SBI:
		case SUI:
		case XRI:
			exp_1_byte ();
			comp = 2;
			break;

		case IN:
		case OUT:
			pilha [0].valor = 0;
			if (!(aux = expressao ()))
				{
				if (!((pilha [0].atr & ABS) && !(pilha [0].valor & ~0xff)))
					erro (PARAMETRO_INVALIDO);
				}
			else
				erro (aux);
			manda_byte (pilha [0].valor);
			comp = 2;
			break;

		case IN0:
			analex ();				/* pula registrador */
			analex ();				/* pula virgula */
			pilha [0].valor = 0;
			if (!(aux = expressao ()))
				{
				if (!((pilha [0].atr & ABS) && !(pilha [0].valor & ~0xff)))
					erro (PARAMETRO_INVALIDO);
				}
			else
				erro (aux);
			manda_byte (pilha [0].valor);
			comp = 3;
			break;

		case OUT0:
			pilha [0].valor = 0;
			if (!(aux = expressao ()))
				{
				if (!((pilha [0].atr & ABS) && !(pilha [0].valor & ~0xff)))
					erro (PARAMETRO_INVALIDO);
				else
					{
					analex ();				/* pula virgula */
					analex ();				/* pula registrador */
					}
				}
			else
				erro (aux);
			manda_byte (pilha [0].valor);
			comp = 3;
			break;

		case TST:
			if ((atomo = analex ()) >= B && atomo <= A)
				comp = 2;
			else
				{
				volta_atomo (atomo);
				exp_1_byte ();
				comp = 3;
				}
			break;

		case TSTIO:
			exp_1_byte ();
			comp = 3;
			break;

		case CALL:
		case CC:
		case CM:
		case CNC:
		case CNZ:
		case CP:
		case CPE:
		case CPO:
		case CZ:
		case JC:
		case JM:
		case JMP:
		case JNC:
		case JNZ:
		case JP:
		case JPE:
		case JPO:
		case JZ:
		case LDA:
		case LHLD:
		case SHLD:
		case STA:
			if (!manda_endereco (1))
				fim_da_linha ();
			comp = 3;
			break;

		case RLC:
			comp = expande_rot (0x0);
			break;

		case RRC:
			comp = expande_rot (0x8);
			break;

		case RAL:
			comp = expande_rot (0x10);
			break;

		case RAR:
			comp = expande_rot (0x18);
			break;

		case ADC:
		case ADD:
		case ANA:
		case CMP:
		case DCR:
		case INR:
		case ORA:
		case SBB:
		case SUB:
		case XRA:
			analex ();				/* pula registrador */
			if (inst_z80 ())
				{
				if ((atomo = analex ()) == MAIS || atomo == MENOS)
					{
					if (atomo == MENOS)
						volta_atomo (atomo);
					exp_s_byte ();
					}
				else
					{
					volta_atomo (atomo);
					manda_byte (0);
					}
				comp = 3;
				}						
			else
				comp = 1;
			break;

		case MVI:
			analex ();				/* pula registrador */
			if (inst_z80 ())
				{
				if ((atomo = analex ()) == MAIS || atomo == MENOS)
					{
					if (atomo == MENOS)
						volta_atomo (atomo);
					exp_s_byte ();
					}
				else
					{
					volta_atomo (atomo);
					manda_byte (0);
					}
				comp = 4;
				}						
			else
				comp = 2;
			analex ();				/* pula virgula */
			exp_1_byte ();
			break;

		case LXI:
			analex ();		/* pula registradores */
			analex ();		/* pula virgula */

/*1.21*/
			if (!manda_endereco (inst_z80 () ? 2 : 1))
				fim_da_linha ();
			if (inst_z80 ())
				comp = 4;
			else
				comp = 3;
			break;

		case DCX:
		case INX:
		case POP:
		case PUSH:
			analex ();			/* pula registrador */
			if (inst_z80 ())
				comp = 2;
			else
				comp = 1;
			break;

		case BIT:
			expressao ();		/* recalcule bit */
			analex ();			/* pula virgula */
			comp = expande_shift (analex (), 0x40 | (pilha [0].valor << 3));
			break;

		case BSET:
			expressao ();		/* recalcule bit */
			analex ();			/* pula virgula */
			comp = expande_shift (analex (), 0xc0 | (pilha [0].valor << 3));
			break;

		case RES:
			expressao ();		/* recalcule bit */
			analex ();			/* pula virgula */
			comp = expande_shift (analex (), 0x80 | (pilha [0].valor << 3));
			break;

		case SLA:
			comp = expande_shift (analex (), 0x20);
			break;

		case SRA:
			comp = expande_shift (analex (), 0x28);
			break;

		case SRL:
			comp = expande_shift (analex (), 0x38);
			break;

		case LBCD:
		case LDED:
		case LIXD:
		case LIYD:
		case LSPD:
		case SBCD:
		case SDED:
		case SIXD:
		case SIYD:
		case SSPD:
/*1.21*/
			if (!manda_endereco (2))
				fim_da_linha ();
			comp = 4;
			break;

		case INC:
		case OUTC:
			analex ();			/* pula registrador */

		case CMD:
		case CMDR:
		case CMI:
		case CMIR:
		case IND:
		case INDR:
		case INI:
		case INIR:
		case LDD:
		case LDDR:
		case LDI:
		case LDIR:
		case NEG:
		case OTDM:
		case OTDMR:
		case OTDR:
		case OTIM:
		case OTIMR:
		case OTIR:
		case OUTD:
		case OUTI:
		case PCIX:
		case PCIY:
		case RETI:
		case RETN:
		case RLD:
		case RRD:
		case SLP:
		case SPIX:
		case SPIY:
			comp = 2;
			break;

		case IM:
			fim_da_linha ();			/* pula expressao */
			comp = 2;
			break;

		case DADC:
		case DADX:
		case DADY:
		case DSBB:
		case MLT:
			analex ();
			comp = 2;
			break;

		case JR:
		case JNZR:
		case JZR:
		case JNCR:
		case JCR:
		case DJNZ:
			pilha [0].valor = 0;
			if (!(comp = expressao ()))
				{
				pilha [0].valor -= pc + 2;
				if (!(pilha [0].atr & alocatual) || (pilha [0].valor & ~0x7f && (pilha [0].valor & ~0x7f) != ~0x7f))
					erro (PARAMETRO_INVALIDO);
				}
			else
				erro (comp);
			manda_byte (pilha [0].valor);
			comp = 2;
			break;

		default:
			erro (INSTRUCAO_INVALIDA);
			return;
			}

		pc += comp;
		pmem += comp;
		}
	}

void manda_byte (int byte)
	{
	manda_bit (0);
	manda (byte, 8);
	}

int manda_endereco (int offset)
	{
	unsigned int aux;

	if (aux = expressao ())
		{
		erro_linha (aux);
		return 0;
		}

	if (pilha [0].atr & extrn)
		{
		if (pilha [0].valor)
			{
			manda_bit (1);
			manda (special_link, 2);
			manda (external_plus_offset, 4);
			manda (absolute, 2);
			manda (pilha [0].valor, 8);
			manda (pilha [0].valor >> 8, 8);
			}
		pilha [0].atr = extrn_chain [pilha [0].s -> valor].a;
		pilha [0].valor = extrn_chain [pilha [0].s -> valor].os;
		extrn_chain [pilha [0].s -> valor].a = alocatual;
		extrn_chain [pilha [0].s -> valor].os = pc + offset;
		}

	if (!(pilha [0].atr & ABS))
		{
		manda_bit (1);
		manda (pilha [0].atr & COD ? program_relative : data_relative, 2);
		manda (pilha [0].valor, 8);
		manda (pilha [0].valor >> 8, 8);
		}
	else
		{
		manda_byte (pilha [0].valor);
		manda_byte (pilha [0].valor >> 8);
		}
	return 1;
	}

void define_nome (void)
	{
	int i;
	char *caux;

	if (mod_name != NULL)
		{
		for (i = 0; (mod_name -> nome) [i] != '\0' && i < (sizeof nome_rel) - 1; i++)
			nome_rel [i] = (mod_name -> nome) [i];
		if (i < sizeof nome_rel)
			nome_rel [i] = '\0';
		else
			nome_rel [(sizeof nome_rel) - 1] = '\0';
		}
	else
		{
		caux = tira_path (nome_arq_rel);
		for (i = 0; caux [i] != '\0' && caux [i] != '.' && i < (sizeof nome_rel) - 1; i++)
			nome_rel [i] = caux [i];
		if (i < sizeof nome_rel)
			nome_rel [i] = '\0';
		else
			nome_rel [(sizeof nome_rel) - 1] = '\0';
		}
	strupr (nome_rel);
	}

void inicia_saida (void)
	{
	csaida = 0;
	masksaida = 0x80;
	if ((arqrel = open (nome_arq_rel, O_TRUNC | O_CREAT | O_BINARY | O_WRONLY, S_IWRITE)) == -1)
		{
		arqrel = 0;
		mprintf (falha_de_abertura, strupr (nome_arq_rel));
		grave ();
		}
	}

void manda (int num, int n)
	{
	unsigned int mask;

	mask = 1 << (n - 1);
	while (mask)
		{
		manda_bit (num & mask);
		mask >>= 1;
		}
	}

void manda_bit (int bit)
	{
	if (bit)
		saida [csaida] |= masksaida;
	else
		saida [csaida] &= ~masksaida;

	if (masksaida == 1)
		{
		masksaida = 0x80;
		if (++csaida >= sizeof saida)
			{
			escreve (arqrel, saida, sizeof saida);
			csaida = 0;
			}
		}
	else
		masksaida >>= 1;
	}

void fecha_arqs (void)
	{
	manda_bit (1);
	manda (special_link, 2);
	manda (end_module, 4);
	manda (absolute, 2);
	manda (0, 8);
	manda (0, 8);
	while (masksaida != 0x80)
		manda_bit (0);
	manda (0x9e, 8);
	if (csaida)
		escreve (arqrel, saida, csaida);	/* para compatibilidade com linc80 */
	close (arqrel);
	arqrel = 0;
	}

void manda_nome (void)
	{
	int i;

	for (i = 0; nome_rel [i] != '\0'; i++);
	manda_bit (1);
	manda (special_link, 2);
	manda (program_name, 4);
	manda (i, 3);
	for (i = 0; nome_rel [i] != '\0'; i++)
		manda (nome_rel [i], 8);
	}

void manda_comprimentos (void)
	{
	manda_bit (1);
	manda (special_link, 2);
	manda (define_data_size, 4);
	manda (absolute, 2);
	manda (pcdata, 8);
	manda (pcdata >> 8, 8);
	manda_bit (1);
	manda (special_link, 2);
	manda (define_program_size, 4);
	manda (program_relative, 2);
	manda (pccod, 8);
	manda (pccod >> 8, 8);
	pcabs = pccod = pcdata = 0;
	}

void manda_end (void)
	{
	manda_bit (1);
	manda (special_link, 2);
	manda (set_location_counter, 4);
	switch (alocatual)
		{
	case ABS:
		manda (absolute, 2);
		break;

	case COD:
		manda (program_relative, 2);
		break;

	case DAT:
		manda (data_relative, 2);
		break;
		}
	manda (pc, 8);
	manda (pc >> 8, 8);
	}

void manda_externos (void)
	{
	int i, j;
	simb *smb;

	for (i = 0; i < inic_simb_size; i++)
		for (smb = inic_simbolo [i]; smb != NULL; smb = smb -> next_simbolo)
			if (smb -> atrib & extrn)
				{
				manda_bit (1);
				manda (special_link, 2);
				manda (chain_external, 4);
				if (extrn_chain [smb -> valor].a & ABS)
					manda (absolute, 2);
				else if (extrn_chain [smb -> valor].a & COD)
					manda (program_relative, 2);
				else if (extrn_chain [smb -> valor].a & DAT)
					manda (data_relative, 2);
				manda (extrn_chain [smb -> valor].os, 8);
				manda (extrn_chain [smb -> valor].os >> 8, 8);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++);
				manda (j, 3);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++)
					manda ((smb -> nome) [j], 8);
				}
	}

void manda_entry_symbols (void)
	{
	int i, j;
	simb *smb;

	for (i = 0; i < inic_simb_size; i++)
		for (smb = inic_simbolo [i]; smb != NULL; smb = smb -> next_simbolo)
			if (smb -> atrib & public)
				if (smb -> atrib & definido)
					{
					manda_bit (1);
					manda (special_link, 2);
					manda (entry_symbol, 4);
					for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++);
					manda (j, 3);
					for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++)
						manda ((smb -> nome) [j], 8);
					}
				else
					{
					erros++;
					mprintf ("%s declarado publico mas nao definido.\n", smb -> nome);
					}
	}

void manda_publics (void)
	{
	int i, j;
	simb *smb;

	for (i = 0; i < inic_simb_size; i++)
		for (smb = inic_simbolo [i]; smb != NULL; smb = smb -> next_simbolo)
			if (smb -> atrib & public)
				{
				manda_bit (1);
				manda (special_link, 2);
				manda (define_entry_point, 4);
				if (smb -> atrib & ABS)
					manda (absolute, 2);
				else if (smb -> atrib & COD)
					manda (program_relative, 2);
				else if (smb -> atrib & DAT)
					manda (data_relative, 2);
				manda (smb -> valor, 8);
				manda (smb -> valor >> 8, 8);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++);
				manda (j, 3);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++)
					manda ((smb -> nome) [j], 8);
				}
	}

/*****************************************************************************
	cria_sym ():

	Cria arquivo com listagem de simbolos.

*****************************************************************************/

void cria_sym (void)
	{
	int i;
	unsigned int sn;
	simb *s;

	simb_linha = simb_col = sym_saida = 0;

	if ((arq_sym = open (nome_arq_sym, O_TRUNC | O_CREAT | O_TEXT | O_WRONLY, S_IWRITE)) == -1)
		{
		arq_sym = 0;
		mprintf ("NAO CONSEGUIU ABRIR ARQUIVO DE SIMBOLOS.\n");
		return;
		}

	if (nset_simb)					/* so faz alguma coisa se existe simbolo */
		{
		ordena_tab_sym ();			/* ordena tabela de simbolos */
		for (sn = 0; sn < ((nset_simb - 1) << nrot_aloc) + (nsimb_aloc - resta_simb); sn++)
			if (((s = aloc_simb [(sn >> nrot_aloc) & c_mask_aloc] + (sn & mask_aloc)) -> atrib & (definido | extrn | set)) == definido)
				if (s -> atrib & public || *(s -> nome) != '?')
					imprime_simbolo (s);
		escreve (arq_sym, buf_sym, sym_saida);
		}

	close (arq_sym);
	arq_sym = 0;
	}

/*****************************************************************************
	ordena_tab_sym ():

	Ordena tabela de simbolos via quick sort.

*****************************************************************************/

void ordena_tab_sym (void)
	{
	sort (((long int) (nset_simb - 1) << nrot_aloc) + (nsimb_aloc - resta_simb));
	}

/*****************************************************************************
	sort ():

	Ordena tabela de simbolos entre os limites dados.

*****************************************************************************/

void sort (long int n)
	{
	char tempnome[comp_max + 1];
	unsigned int tempvalor;
	char tempatrib;
	simb *s1, *s2;
	long int i, j, l, r;
	char x [comp_max + 1];
	int s;					/* ponteiro da pilha */
	struct
		{
		long int l;
		long int r;
		} stack [20];			/* com esta pilha e' possivel ordenar ate' 2 ** 20 elementos */

	s = 0;
	stack [0].l = 0;
	stack [0].r = n - 1;
	do
		{
		l = stack [s].l;
		r = stack [s--].r;
		do
			{
			i = l;
			j = r;
			strcpy (x, snome ((l + r) >> 1));
			do
				{
				while (strcmp (snome (i), x) < 0)
					i++;
				while (strcmp (x, snome (j)) < 0)
					j--;
				if (i <= j)
					{
					s1 = aloc_simb [(i >> nrot_aloc) & c_mask_aloc] + (i & mask_aloc);
					s2 = aloc_simb [(j >> nrot_aloc) & c_mask_aloc] + (j & mask_aloc);

					strcpy (tempnome, s1 -> nome);
					tempvalor = s1 -> valor;
					tempatrib = s1 -> atrib;

					strcpy (s1 -> nome, s2 -> nome);
					s1 -> valor = s2 -> valor;
					s1 -> atrib = s2 -> atrib;

					strcpy (s2 -> nome, tempnome);
					s2 -> valor = tempvalor;
					s2 -> atrib = tempatrib;
					i++;
					j--;
					}
				}
			while (i <= j);

			if (j - l < r - i)
				{
				if (i < r)
					{
					stack [++s].l = i;
					stack [s].r = r;
					}
				r = j;
				}
			else
				{
				if (l < j)
					{
					stack [++s].l = l;
					stack [s].r = j;
					}
				l = i;
				}
			}
		while (l < r);
		}
	while (s >= 0);
	}

/*****************************************************************************
	imprime_simbolo ():

	Dado simbolo, escreve-o no arquivo de saida.

*****************************************************************************/

void imprime_simbolo (simb *s)
	{
	char n [(sizeof (long)) * 8 + 1], *num, *num0;
	int i;

	while (simb_col & 0xf)
		manda_car_sym (' ');

	num0 = num = ultoa ((unsigned long) s -> valor, n, 16);
	while (*(++num0) != '\0');
	for (i = 4 - (num0 - num); i; i--)
		manda_car_sym ('0');
	while (*num != '\0')
		{
		manda_car_sym (*num >= 'a' && *num <= 'f' ? *num + 'F' - 'f' : *num);
		num++;
		}
	manda_car_sym (' ');
	for (i = 0, num = s -> nome; *num != '\0'; i++)
		manda_car_sym (*(num++));
	if (++simb_linha == 5)
		{
		manda_car_sym ('\n');
		simb_linha = simb_col = 0;
		}
	}

/*****************************************************************************
	manda_car_sym ()

	Escreve caracter no buffer de simbolos.

*****************************************************************************/

void manda_car_sym (char c)
	{
	buf_sym [sym_saida++] = c;
	if (sym_saida == sizeof (buf_sym))
		{
		escreve (arq_sym, buf_sym, sizeof (buf_sym));
		sym_saida = 0;
		}
	simb_col++;
	}

/*****************************************************************************
	escreve ()

	Rotina semelhante ao write fo C, so que confere se nao houve erro.

*****************************************************************************/

void escreve (int handle, char *buffer, unsigned int count)
	{
	if (write (handle, buffer, count) == -1)
		{
		mprintf ("DISCO CHEIO.\n");
		grave ();
		}
	}

/*****************************************************************************
	inst_z80 ()

	Indica se instrucao atual e' de z80.

*****************************************************************************/

int inst_z80 (void)
	{
	int inst;

	return (inst = (*(tab_mem_aloc [(pmem >> bit_rot_mem) & ((1 << (16 - bit_rot_mem)) - 1)] + (pmem & masc_mem))) & 0xff)
		== 0xcb || inst == 0xdd || inst == 0xed || inst == 0xfd;
	}

/*****************************************************************************
	exp_s_byte ()

	Calcula expressao e manda byte, conferindo se valor com sinal de 1 byte.

*****************************************************************************/

void exp_s_byte (void)
	{
	int aux;

	pilha [0].valor = 0;
	if (!(aux = expressao ()))
		{
		if (!((pilha [0].atr & ABS) && (!(pilha [0].valor & ~0x7f) || ((pilha [0].valor & ~0x7f) == ~0x7f))))
			erro (PARAMETRO_INVALIDO);
		}
	else
		erro (aux);
	manda_byte (pilha [0].valor);
	}

/*****************************************************************************
	exp_1_byte ()

	Calcula expressao e manda byte, conferindo se valor apropriado.

*****************************************************************************/

void exp_1_byte (void)
	{
	int aux;

	pilha [0].valor = 0;
	if (!(aux = expressao ()))
		{
		if (!((pilha [0].atr & ABS) && (!(pilha [0].valor & ~0xff) || ((pilha [0].valor & ~0x7f) == ~0x7f))))
			erro (PARAMETRO_INVALIDO);
		}
	else
		erro (aux);
	manda_byte (pilha [0].valor);
	}

/*****************************************************************************
	expande_rot ()

	Expande instrucoes de rotacoes de registradores que originalmente eram
feitas apenas no acumulador.

*****************************************************************************/

int expande_rot (int oc)
	{
	t_atomo atomo;

	if ((atomo = analex ()) == EOL)
		{
		volta_atomo (atomo);
		return 1;
		}

	return expande_shift (atomo, oc);
	}

/*****************************************************************************
	expande_rot ()

	Expande instrucoes de shifts de registradores presentes no Z80.

*****************************************************************************/

int expande_shift (t_atomo atomo, int oc)
	{
	if (atomo == IX || atomo == IY)
		{
		if ((atomo = analex ()) == MAIS || atomo == MENOS)
			{
			if (atomo == MENOS)
				volta_atomo (atomo);
			exp_s_byte ();
			}
		else
			{
			volta_atomo (atomo);
			manda_byte (0);
			}
		manda_byte (oc | M);
		return 4;
		}
	return 2;
	}

