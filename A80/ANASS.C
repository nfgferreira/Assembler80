#include <fcntl.h>
#include <io.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "variaveis.h"
#include "protos.h"

/********************* variaveis exclusivas de anass ***********************/

unsigned int linha_atual;
char mac_arq [sizeof (string)];
unsigned int clocais;			/* numero de caracteres das variaveis locais */
unsigned int local_atual;		/* numero de nomes locais processados */
jmp_buf jmp_erro;
int if_maclib;					/* nivel de pilha de if's na entrada de maclib */
jmp_buf finaliza;				/* endereco de retorno p/ final de montagem */
int np_irp;						/* numero de parametros de macro tipo irp */
char par_irp [max_car_par];	/* parametros de macro tipo irp */
unsigned int n_rept;			/* numero de repeticoes de macro tipo rept */
unsigned int nn_rept;
int tipo_macro;					/* tipo de macro sendo definida: 0 = macro normal
  																 1 = irp
  																 2 = rept		*/

#define bytmem(byte) (coloca_byte (byte, pmem))

// Numbers match tipo_erros definitions in VARIAVEIS.H. 
static char *erro_tab[] =
	{
	"Double definition.",
	"Number is too large.",
	"Invalid number.",
	"Expecting comma.",
	"Invalid instruction.",
	"Invalid parameter.",
	"Invalid expression.",
	"More than 64K of code.",
	"Expecting closing parenthesis.",
	"Expecting number.",
	"Invalid NOT.",
	"Invalid unary -.",
	"HIGH not allowed with relocatable number.",
	"LOW not allowed with relocatable number.",
	"Invalid AND.",
	"Invalid addition.",
	"Invalid subtraction.",
	"Invalid comparison.",
	"Internal error.",
	"Too many pending operands.",
	"Undefined identifier.",
	"Symbol table full.",
	"Invalid OR.",
	"Invalid XOR.",
	"Expecting name.",
	"Too many externals defined.",
	"Step 2 is different than step 1.",
	"String too large.",
	"Expecting end of string.",
	"Too many nested IFs.",
	"Else does not match IF.",
	"Endif does not match IF.",
	"Number of IFs larger than ENDIFs.",
	"Nested insert.",
	"Too many parenthesis levels.",
	"Dividing by 0.",
	"Expecting ENDM.",
	"Maximum number of macros reached.",
	"Too much code inside macros.",
	"Number of macro local parameters and labels exceeded.",
	"Number of simultaneous macro expansions exceeded.",
	"Number of characters in macro parameters exceeded.",
	"Insufficient memory (fatal).",
	"Number of macro local names exceeded.",
	"Invalid multiplication.",
	"Phase error.",
	"Expecting end of line.",
	"PUBLIC symbol and set.",
	"Invalid division.",
	"Invalid shift.",
	"Unexpected character.",
	"Inconsistent PCH file."
	};

static char *nmacro = " macro";

int compila (char *nome)
	{
	int retorno;

	if (retorno = setjmp (finaliza))
		return retorno - 1;

	inicializa1 (nome);
	passo1 ();
	close (arq);
	arq = 0;
	if (erros)
		termina ();
	if (monta_pch)					/* caso e' para montar pre-compiled-header */
		constroi_pch ();
	else								/* caso montar .rel */
		{
		acerta_pont_old ();
		inicializa2 (nome);
		passo2 ();
		fecha_arqs ();
		}
	if (faz_simbolo)
		cria_sym ();
	termina ();

	return 0;			// Just to keep the compiler happy.
	}

void passo1 (void)
	{
	t_atomo atomo;
	int aux;
	int passou;
	int definiu_macro;
	int montava;
	simb *memsimb;
	macro_desc *mac;

	mprintf ("STEP 1\n");
	while (1)
		{
		while ((atomo = analex ()) == EOL);

		definiu_macro = 0;
		if (definindo_macro)
			volta_atomo (atomo);

		while (definindo_macro)
			{
			if ((atomo = analex ()) == LABEL)
				atomo = analex ();
			switch (atomo)
				{
			case ENDM:
				definindo_macro--;
				definiu_macro = 1;
				break;

			case IRP:
			case MACRO:
			case REPT:
				definindo_macro++;
				break;

			case EOA:
				volta_atomo (atomo);
				erro (ENDM_ESPERADO);
				definindo_macro = 0;
				definiu_macro = 1;
				pmacro -> atrib = 0;		/* desdefine macro */
				if (maclib && if_maclib != if_counter)
					{
					erro (IFS_DESANINHADOS);
					if_counter = if_maclib;
					}
				break;
				}

			aux = 0;
			do
				guarda_car (next_linha [aux] == _EOF ? '\n' : next_linha [aux]);
			while (next_linha [aux++] != '\n' && next_linha [aux - 1] != _EOF);
			if (definindo_macro)
				{
				if (!ppmac && (atomo != EOL))
					if (next_linha [aux - 1] == '\n')
						{
						linha++;
						if (!maclib)
							salva (EOL);
						}
					else
						if (!maclib)
							salva (EOA);
				resta_car = 0;
				}
			else
				{
				guarda_car ('\0');		/* indica final de macro (usado por arquivo pch) */
				if (!tipo_macro)	/* se macro normal */
					fim_da_linha ();
				}
			}

		if (definiu_macro)
			{
			switch (tipo_macro)
				{
			case 0:														/* macro normal */
				break;

			case 1:														/* irp */
				if (np_irp)
					{
					salva_estado ();
					*nmacro = ppmac + '0' - 1;
					mac = busca_macro (nmacro);
					mac_char = mac -> pchar;
					mac_pp = mac -> ppar;
					mac_np = mac -> npar;
					mac_nl = 0;					/* nenhum local por enquanto */
					m_loops = 0;
					exp_tipo_macro = 1;
					for (aux = 0; aux < max_car_par; aux++)
						mpar [aux] = par_irp [aux];
					nparm = np_irp;
					}
				else
					fim_da_linha ();
				break;

			case 2:
				if (nn_rept)
					{
					salva_estado ();
					*nmacro = ppmac + '0' - 1;
					mac = busca_macro (nmacro);
					mac_char = mac -> pchar;
					mac_pp = mac -> ppar;
					mac_np = mac -> npar;
					mac_nl = 0;					/* nenhum local por enquanto */
					m_loops = 0;
					n_rept = nn_rept;
					exp_tipo_macro = 2;
					nparm = 0;
					}
				else
					fim_da_linha ();
				break;
				}
			}

		montava = monta;
		if (!definiu_macro) switch (atomo)
			{
		case IFDEF:
			passou = 1;
			if (monta)
				if ((atomo = analex ()) == NOME)
					aux = (simbolo -> atrib & definido) ? ~0 : 0;
				else
					{
					volta_atomo (atomo);
					erro (NOME_ESPERADO);
					aux = ~0;
					}
			else
				fim_da_linha ();
			if (if_counter == if_size)
				erro_fatal (MUITOS_IFS_ANINHADOS);
			if (!if_counter)		/* primeiro IF */
				monta = if_stack [0] = (aux & 1);
			else
				monta = if_stack [if_counter] = ((aux & 1) && if_stack [if_counter - 1]);
			if_counter++;
			break;

		case IFNDEF:
			passou = 1;
			if (monta)
				if ((atomo = analex ()) == NOME)
					aux = (simbolo -> atrib & definido) ? 0 : ~0;
				else
					{
					volta_atomo (atomo);
					erro (NOME_ESPERADO);
					aux = ~0;
					}
			else
				fim_da_linha ();
			if (if_counter == if_size)
				erro_fatal (MUITOS_IFS_ANINHADOS);
			if (!if_counter)		/* primeiro IF */
				monta = if_stack [0] = (aux & 1);
			else
				monta = if_stack [if_counter] = ((aux & 1) && if_stack [if_counter - 1]);
			if_counter++;
			break;

		case IF:
			passou = 1;
			if (monta)
				{
				if (aux = expressao ())	/* trata expressao */
					{
					erro (aux);
					pilha [0].atr = ABS;
					pilha [0].valor = ~0;
					}
				else
					if (!(pilha [0].atr & ABS))
						{
						erro (EXPRESSAO_INVALIDA);
						pilha [0].atr = ABS;
						pilha [0].valor = ~0;
						}
				}
			else
				fim_da_linha ();
			if (if_counter == if_size)
				erro_fatal (MUITOS_IFS_ANINHADOS);
			if (!if_counter)		/* primeiro IF */
				monta = if_stack [0] = (pilha [0].valor & 1);
			else
				monta = if_stack [if_counter] = ((pilha [0].valor & 1) && if_stack [if_counter - 1]);
			if_counter++;
			break;

		case ELSE:
			passou = 1;
			if (!if_counter || (maclib && if_counter == if_maclib))
				{
				erro (ELSE_SEM_IF_CORRESPONDENTE);
				break;
				}
			if (if_counter == 1)
				monta = if_stack [0] = !if_stack [0];
			else
				monta = if_stack [if_counter - 1] = (if_stack [if_counter - 2] && !if_stack [if_counter - 1]);
			break;

		case ENDIF:
			passou = 1;
			if (!if_counter || (maclib && if_counter == if_maclib))
				{
				erro (ENDIF_SEM_IF_CORRESPONDENTE);
				break;
				}
			if (if_counter == 1)
				monta = 1;
			else
				monta = if_stack [if_counter - 2];
			if_counter--;
			break;

		case ENDM:
			if (ppmac)
				monta = 1;	/* obriga voltar da macro */

		default:
			passou = 0;
			break;
			}

		if (!monta)
			{
			if (!montava)
				fim_da_linha ();
			if ((atomo = analex ()) == EOA)
				{
				erro (IFS_DESANINHADOS);
				return;
				}
			else
				volta_atomo (atomo);
			}

		else if (!passou && !definiu_macro) switch (atomo)
			{
		case ERRO:
			erro (causa);
			break;

		case EOA:
			if (maclib)
				{
				close (arq);
				rec_lex ();
				linha = linha_atual;
				if (if_maclib != if_counter)
					{
					erro (IFS_DESANINHADOS);
					if_counter = if_maclib;
					}
				maclib = 0;
				break;
				}
			else
				{
				if (if_counter)
					erro (IFS_DESANINHADOS);
				return;
				}

		case END:
			if (maclib)
				{
				salva (EOL);
				salva (END);
				}
			return;

		case EQU:
			if ((memsimb = simbolo) == NULL)
				{
				erro (INSTRUCAO_INVALIDA);
				break;
				}
			if (memsimb -> atrib & definido)
				erro (DUPLA_DEFINICAO);
			else
				if (!(aux = expressao ()))	/* trata expressao */
					if (pilha [0].atr & extrn)
						erro (EXPRESSAO_INVALIDA);
					else
						define_simbolo (memsimb, pilha [0].valor, pilha [0].atr);
				else
					erro (aux);
			break;

		case IRP:
			define_irp ();
			break;

		case LABEL:
			if ((simbolo -> atrib) & definido)
				erro (DUPLA_DEFINICAO);
			else
				{
				define_simbolo (simbolo, pc, alocatual);
				switch (atomo = analex ())
					{
				case END:
					if (maclib)
						{
						salva (EOL);
						salva (END);
						}
					return;

				case IRP:
					define_irp ();
					break;

				case REPT:
					define_rept ();
					break;

				default:
					trata_inst (atomo);			/* trata possivel instrucao da linha */
					break;
					}
				}
			break;

		case MACRO:
			if ((memsimb = simbolo) == NULL)
				{
				erro (INSTRUCAO_INVALIDA);
				break;
				}
			define_macro (memsimb);
			break;

		case REPT:
			define_rept ();
			break;

		case SET:
			if ((memsimb = simbolo) == NULL)
				{
				erro (INSTRUCAO_INVALIDA);
				break;
				}
			if ((memsimb -> atrib & (set | definido)) == definido)
				{
				erro (DUPLA_DEFINICAO);
				break;
				}
			if (memsimb -> atrib & public)
				{
				erro (PUBLIC_E_SET);
				break;
				}
			if (!(aux = expressao ()))	/* trata expressao */
				if (pilha [0].atr & extrn)
					erro (EXPRESSAO_INVALIDA);
				else
					define_simbolo (memsimb, pilha [0].valor, pilha [0].atr | set);
			else
				erro (aux);
			break;

		default:
			trata_inst (atomo);			/* trata possivel instrucao da linha */
			break;
			}
		if ((atomo = analex ()) != EOL && atomo != EOA)
			erro (FIM_DE_LINHA_ESPERADO);
		else
			if (atomo == EOA)
				volta_atomo (atomo);
		}
	}

void define_simbolo (simb *s, int valor, char aloc)
	{
	s -> valor = valor;
	s -> atrib |= (aloc | definido | maclib);
	}

void trata_inst (t_atomo atomo)
	{
	int comp, aux;
	macro_desc *mac;

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
		break;

	case CSEG:
		acerta_pont_old ();
		alocatual = COD;
		acerta_pc ();
		break;

	case DSEG:
		acerta_pont_old ();
		alocatual = DAT;
		acerta_pc ();
		break;

	case MACLIB:
		if (maclib)
			erro (MACLIB_EM_ARQUIVO_INCLUIDO);
		else
			{
			if ((atomo = analex ()) == NOME || (atomo == STRING && string [valor - 1] == '\0'))
				trata_include (atomo == NOME ? strcat (strcpy (string, simbolo -> nome), ".LIB") : string);
			else
				{
				volta_atomo (atomo);
				erro (NOME_ESPERADO);
				}
			}
		break;

	case EXTRN:
		while (1)
			{
			if ((atomo = analex ()) == NOME)
				if ((simbolo -> atrib) & (definido | ABS | COD | DAT | public))
					erro_linha (DUPLA_DEFINICAO);
				else
					if (iec == (sizeof extrn_chain) / (sizeof (ext)))
						{
						erro (MUITOS_EXTERNOS_DEFINIDOS);
						break;
						}
					else
						{
						simbolo -> atrib |= (definido | extrn | maclib);
						simbolo -> valor = iec;
						extrn_chain [iec].a = ABS;
						extrn_chain [iec++].os = 0;
						}
			else
				{
				volta_atomo (atomo);
				erro (NOME_ESPERADO);
				break;
				}
			if ((atomo = analex ()) == EOL || atomo == EOA)
				{
				volta_atomo (atomo);
				break;
				}
			else
				if (atomo != VIRGULA)
					{
					erro (VIRGULA_ESPERADA);
					break;
					}
			}
		break;

	case PUBLIC:
		while (1)
			{
			if ((atomo = analex ()) == NOME)
				if (simbolo -> atrib & extrn)
					erro_linha (DUPLA_DEFINICAO);
				else
					if (simbolo -> atrib & set)
						erro_linha (PUBLIC_E_SET);
					else
						simbolo -> atrib |= public | maclib;
			else
				{
				volta_atomo (atomo);
				erro (NOME_ESPERADO);
				break;
				}
			if ((atomo = analex ()) == EOL || atomo == EOA)
				{
				volta_atomo (atomo);
				break;
				}
			else
				if (atomo != VIRGULA)
					{
					erro (VIRGULA_ESPERADA);
					break;
					}
			}
		break;

	case ORG:
		if (aux = expressao ())	/* trata expressao */
			erro (aux);
		else
			if (pilha [0].atr & ABS)
				pc = pilha [0].valor;
			else
				erro (EXPRESSAO_INVALIDA);
		break;

	case DB:
		comp = 0;
		do
			{
			if ((atomo = analex ()) == STRING && valor != 1)
				{
				comp += valor;
				atomo = analex ();
				}
			else
				{
				volta_atomo (atomo);
				if (aux = parse_expr ())
					{
					erro_linha (aux);
					while ((atomo = analex ()) != VIRGULA && atomo != EOA && atomo != EOL);
					}
				else
					atomo = analex ();
				comp++;
				}
			}
		while (atomo == VIRGULA);
		if (atomo == ERRO)
			erro (causa);
		else
			volta_atomo (atomo);

/*		aux = pmem + comp;
/*		if (aux < pmem)
/*			erro_fatal (PROGRAMA_MUITO_GRANDE);
/*		else
/*			pmem = aux;
*/
		pmem += comp;
		pc += comp;
		break;

	case DS:
		comp = 0;
		if (aux = expressao ())	/* trata expressao */
			erro (aux);
		else
			if (pilha [0].atr & ABS)
				comp = pilha [0].valor;
			else
				erro (EXPRESSAO_INVALIDA);
		pc += comp;
		break;

	case DW:
		comp = 0;
		do
			{
			if (aux = parse_expr ())
				{
				erro_linha (aux);
				while ((atomo = analex ()) != VIRGULA && atomo != EOA && atomo != EOL);
				}
			else
				atomo = analex ();
			comp += 2;
			}
		while (atomo == VIRGULA);
		if (atomo == ERRO)
			erro (causa);
		else
			volta_atomo (atomo);

/*		aux = pmem + comp;
/*		if (aux < pmem)
/*			erro_fatal (PROGRAMA_MUITO_GRANDE);
/*		else
/*			pmem = aux;
*/
		pmem += comp;
		pc += comp;
		break;

	case ENDM:
		if (ppmac)
			switch (exp_tipo_macro)
				{
			case 0:														/* macro normal */
				recupera_estado ();
				break;

			case 1:														/* irp */
				if (++m_loops >= (unsigned int)nparm)
					recupera_estado ();
				else
					{
					*nmacro = ppmac + '0' - 1;
					mac = busca_macro (nmacro);
					mac_char = mac -> pchar;
					mac_pp = mac -> ppar;
					mac_np = mac -> npar;
					mac_nl = 0;					/* nenhum local por enquanto */
					if_counter = 0;
					clocais = 0;
					monta = 1;
					}
				break;

			case 2:														/* rept */
				if (++m_loops >= n_rept)
					recupera_estado ();
				else
					{
					*nmacro = ppmac + '0' - 1;
					mac = busca_macro (nmacro);
					mac_char = mac -> pchar;
					mac_pp = mac -> ppar;
					mac_np = mac -> npar;
					mac_nl = 0;					/* nenhum local por enquanto */
					if_counter = 0;
					clocais = 0;
					monta = 1;
					}
				break;
				}
		else
			erro (INSTRUCAO_INVALIDA);
		break;

	case EXITM:
		if (ppmac)
			recupera_estado ();
		else
			erro (INSTRUCAO_INVALIDA);
		break;

	case LOCAL:
		if (ppmac)
			coloca_locais ();
		else
			erro (INSTRUCAO_INVALIDA);
		break;

	case NAME:
		if ((atomo = analex ()) == NOME)
			mod_name = simbolo;
		else
			{
			volta_atomo (atomo);
			erro (NOME_ESPERADO);
			}
		break;

	case NOME:
		if ((mac = busca_macro (simbolo -> nome)) -> atrib & definido)
			{
			salva_estado ();
			pega_parametros (mpar, &nparm);
			mac_char = mac -> pchar;
			mac_pp = mac -> ppar;
			mac_np = mac -> npar;
			mac_nl = 0;					/* nenhum local por enquanto */
			exp_tipo_macro = 0;
			volta_atomo (EOL);		/* para nao dar erro */
			break;
			}
		else
			{
			erro (INSTRUCAO_INVALIDA);
			return;
			}

	default:
		switch (atomo)
			{
		case ACI:
			bytmem (0xce);
			parse_1_byte ();
			comp = 2;
			break;

		case ADC:
			comp = monta1 (0x88);
			break;

		case ADD:
			comp = monta1 (0x80);
			break;

		case ADI:
			bytmem (0xc6);
			parse_1_byte ();
			comp = 2;
			break;

		case ANA:
			comp = monta1 (0xa0);
			break;

		case ANI:
			bytmem (0xe6);
			parse_1_byte ();
			comp = 2;
			break;

		case BIT:
			if (comp = expressao ())
				{
				erro (comp);
				comp = 2;
				}
			else
				if ((!(pilha [0].atr & ABS)) || (pilha [0].valor & ~0x07))
					erro (EXPRESSAO_INVALIDA);
				else
					if (espera_virgula ())
						comp = monta9 (0x40 | (pilha [0].valor << 3));
					else
						comp = 2;
			break;

		case BSET:
			if (comp = expressao ())
				{
				erro (comp);
				comp = 2;
				}
			else
				if ((!(pilha [0].atr & ABS)) || (pilha [0].valor & ~0x07))
					erro (EXPRESSAO_INVALIDA);
				else
					if (espera_virgula ())
						comp = monta9 (0xc0 | (pilha [0].valor << 3));
					else
						comp = 2;
			break;

		case CALL:
			bytmem (0xcd);
			parse_2_bytes ();
			comp = 3;
			break;

		case CC:
			bytmem (0xdc);
			parse_2_bytes ();
			comp = 3;
			break;

		case CM:
			bytmem (0xfc);
			parse_2_bytes ();
			comp = 3;
			break;

		case CMA:
			bytmem (0x2f);
			comp = 1;
			break;

		case CMC:
			bytmem (0x3f);
			comp = 1;
			break;

		case CMD:
			bytz80 (0xed, 0xa9);
			comp = 2;
			break;

		case CMDR:
			bytz80 (0xed, 0xb9);
			comp = 2;
			break;

		case CMI:
			bytz80 (0xed, 0xa1);
			comp = 2;
			break;

		case CMIR:
			bytz80 (0xed, 0xb1);
			comp = 2;
			break;

		case CMP:
			comp = monta1 (0xb8);
			break;

		case CNC:
			bytmem (0xd4);
			parse_2_bytes ();
			comp = 3;
			break;

		case CNZ:
			bytmem (0xc4);
			parse_2_bytes ();
			comp = 3;
			break;

		case CP:
			bytmem (0xf4);
			parse_2_bytes ();
			comp = 3;
			break;

		case CPE:
			bytmem (0xec);
			parse_2_bytes ();
			comp = 3;
			break;

		case CPI:
			bytmem (0xfe);
			parse_1_byte ();
			comp = 2;
			break;

		case CPO:
			bytmem (0xe4);
			parse_2_bytes ();
			comp = 3;
			break;

		case CZ:
			bytmem (0xcc);
			parse_2_bytes ();
			comp = 3;
			break;

		case DAA:
			bytmem (0x27);
			comp = 1;
			break;

		case DAD:
			monta2 (0x09);
			comp = 1;
			break;

		case DADC:
			monta7 (0x4a);
			comp = 2;
			break;

		case DADX:
		case DADY:
			monta8 (0x09, atomo == DADX ? IX : IY);
			comp = 2;
			break;

		case DCR:
			comp = monta3 (0x05);
			break;

		case DCX:
			if (!(comp = monta6 (0x0b)))
				comp = 1;
			break;

		case DI:
			bytmem (0xf3);
			comp = 1;
			break;

		case DJNZ:
			monta11 (0x10);
			comp = 2;
			break;

		case DSBB:
			monta7 (0x42);
			comp = 2;
			break;

		case EI:
			bytmem (0xfb);
			comp = 1;
			break;

		case HLT:
			bytmem (0x76);
			comp = 1;
			break;

		case IM:
			if (comp = expressao ())
				erro (comp);
			else
				if (!(pilha [0].atr & ABS) || pilha [0].valor > 2)
					erro (PARAMETRO_INVALIDO);
				else
					bytz80 (0xed, pilha [0].valor == 2 ? 0x5e : 0x46 | (pilha [0].valor << 4));
			comp = 2;
			break;

		case IN:
			bytmem (0xdb);
			parse_byte ();
			comp = 2;
			break;

		case IN0:
			if (monta10 (0x00))
				if (espera_virgula ())
					parse_byte ();
			comp = 3;
			break;

		case INC:
			monta10 (0x40);
			comp = 2;
			break;

		case IND:
			bytz80 (0xed, 0xaa);
			comp = 2;
			break;

		case INDR:
			bytz80 (0xed, 0xba);
			comp = 2;
			break;

		case INI:
			bytz80 (0xed, 0xa2);
			comp = 2;
			break;

		case INIR:
			bytz80 (0xed, 0xb2);
			comp = 2;
			break;

		case INR:
			comp = monta3 (0x04);
			break;

		case INX:
			if (!(comp = monta6 (0x03)))
				comp = 1;
			break;

		case JC:
			bytmem (0xda);
			parse_2_bytes ();
			comp = 3;
			break;

		case JCR:
			monta11 (0x38);
			comp = 2;
			break;

		case JM:
			bytmem (0xfa);
			parse_2_bytes ();
			comp = 3;
			break;

		case JMP:
			bytmem (0xc3);
			parse_2_bytes ();
			comp = 3;
			break;

		case JNC:
			bytmem (0xd2);
			parse_2_bytes ();
			comp = 3;
			break;

		case JNCR:
			monta11 (0x30);
			comp = 2;
			break;

		case JNZ:
			bytmem (0xc2);
			parse_2_bytes ();
			comp = 3;
			break;

		case JNZR:
			monta11 (0x20);
			comp = 2;
			break;

		case JP:
			bytmem (0xf2);
			parse_2_bytes ();
			comp = 3;
			break;

		case JPE:
			bytmem (0xea);
			parse_2_bytes ();
			comp = 3;
			break;

		case JPO:
			bytmem (0xe2);
			parse_2_bytes ();
			comp = 3;
			break;

		case JR:
			monta11 (0x18);
			comp = 2;
			break;

		case JZ:
			bytmem (0xca);
			parse_2_bytes ();
			comp = 3;
			break;

		case JZR:
			monta11 (0x28);
			comp = 2;
			break;

		case LBCD:
			bytz80 (0xed, 0x4b);
			parse_2_bytes ();
			comp = 4;
			break;

		case LDA:
			bytmem (0x3a);
			parse_2_bytes ();
			comp = 3;
			break;

		case LDAX:
			monta4 (0x0a);
			comp = 1;
			break;

		case LDD:
			bytz80 (0xed, 0xa8);
			comp = 2;
			break;

		case LDDR:
			bytz80 (0xed, 0xb8);
			comp = 2;
			break;

		case LDED:
			bytz80 (0xed, 0x5b);
			parse_2_bytes ();
			comp = 4;
			break;

		case LDI:
			bytz80 (0xed, 0xa0);
			comp = 2;
			break;

		case LDIR:
			bytz80 (0xed, 0xb0);
			comp = 2;
			break;

		case LHLD:
			bytmem (0x2a);
			parse_2_bytes ();
			comp = 3;
			break;

		case LIXD:
			bytz80 (0xdd, 0x2a);
			parse_2_bytes ();
			comp = 4;
			break;

		case LIYD:
			bytz80 (0xfd, 0x2a);
			parse_2_bytes ();
			comp = 4;
			break;

		case LSPD:
			bytz80 (0xed, 0x7b);
			parse_2_bytes ();
			comp = 4;
			break;

		case LXI:
			if (!(comp = monta6 (0x01)))
				comp = 3;
			else
				{
				comp += 2;
				if (espera_virgula ())
					parse_2_bytes ();
				}
			break;

		case MLT:
			monta7 (0x4c);
			comp = 2;
			break;

		case MOV:
			comp = monta12 ();
			break;

		case MVI:
			if (comp = monta3 (0x06))
				if (espera_virgula ())
					parse_1_byte ();
			comp++;
			break;

		case NEG:
			bytz80 (0xed, 0x44);
			comp = 2;
			break;

		case NOP:
			bytmem (0x00);
			comp = 1;
			break;

		case ORA:
			comp = monta1 (0xb0);
			break;

		case ORI:
			bytmem (0xf6);
			parse_1_byte ();
			comp = 2;
			break;

		case OTDM:
			bytz80 (0xed, 0x8b);
			comp = 2;
			break;

		case OTDMR:
			bytz80 (0xed, 0x9b);
			comp = 2;
			break;

		case OTDR:
			bytz80 (0xed, 0xbb);
			comp = 2;
			break;

		case OTIM:
			bytz80 (0xed, 0x83);
			comp = 2;
			break;

		case OTIMR:
			bytz80 (0xed, 0x93);
			comp = 2;
			break;

		case OTIR:
			bytz80 (0xed, 0xb3);
			comp = 2;
			break;

		case OUT:
			bytmem (0xd3);
			parse_byte ();
			comp = 2;
			break;

		case OUT0:
			if (parse_byte ())
				if (espera_virgula ())
					monta10 (0x01);
			comp = 3;
			break;

		case OUTC:
			monta10 (0x41);
			comp = 2;
			break;

		case OUTD:
			bytz80 (0xed, 0xab);
			comp = 2;
			break;

		case OUTI:
			bytz80 (0xed, 0xa3);
			comp = 2;
			break;

		case PCHL:
			bytmem (0xe9);
			comp = 1;
			break;

		case PCIX:
			bytz80 (0xdd, 0xe9);
			comp = 2;
			break;

		case PCIY:
			bytz80 (0xfd, 0xe9);
			comp = 2;
			break;

		case POP:
			if (!(comp = monta5 (0xc1)))
				comp = 1;
			break;

		case PUSH:
			if (!(comp = monta5 (0xc5)))
				comp = 1;
			break;

		case RAL:
			atomo = analex ();
			volta_atomo (atomo);
			if (atomo == EOL)
				{
				bytmem (0x17);
				comp = 1;
				}
			else
				comp = monta9 (0x10);
			break;

		case RAR:
			atomo = analex ();
			volta_atomo (atomo);
			if (atomo == EOL)
				{
				bytmem (0x1f);
				comp = 1;
				}
			else
				comp = monta9 (0x18);
			break;

		case RC:
			bytmem (0xd8);
			comp = 1;
			break;

		case RES:
			if (comp = expressao ())
				{
				erro (comp);
				comp = 2;
				}
			else
				if ((!(pilha [0].atr & ABS)) || (pilha [0].valor & ~0x07))
					erro (EXPRESSAO_INVALIDA);
				else
					if (espera_virgula ())
						comp = monta9 (0x80 | (pilha [0].valor << 3));
					else
						comp = 2;
			break;

		case RET:
			bytmem (0xc9);
			comp = 1;
			break;

		case RETI:
			bytz80 (0xed, 0x4d);
			comp = 2;
			break;

		case RETN:
			bytz80 (0xed, 0x45);
			comp = 2;
			break;

		case RIM:
			bytmem (0x20);
			comp = 1;
			break;

		case RLC:
			atomo = analex ();
			volta_atomo (atomo);
			if (atomo == EOL)
				{
				bytmem (0x07);
				comp = 1;
				}
			else
				comp = monta9 (0x0);
			break;

		case RLD:
			bytz80 (0xed, 0x6f);
			comp = 2;
			break;

		case RM:
			bytmem (0xf8);
			comp = 1;
			break;

		case RNC:
			bytmem (0xd0);
			comp = 1;
			break;

		case RNZ:
			bytmem (0xc0);
			comp = 1;
			break;

		case RP:
			bytmem (0xf0);
			comp = 1;
			break;

		case RPE:
			bytmem (0xe8);
			comp = 1;
			break;

		case RPO:
			bytmem (0xe0);
			comp = 1;
			break;

		case RRC:
			atomo = analex ();
			volta_atomo (atomo);
			if (atomo == EOL)
				{
				bytmem (0x0f);
				comp = 1;
				}
			else
				comp = monta9 (0x8);
			break;

		case RRD:
			bytz80 (0xed, 0x67);
			comp = 2;
			break;

		case RST:
			if (comp = expressao ())
				erro (comp);
			else
				if ((!(pilha [0].atr & ABS)) || (pilha [0].valor & ~0x07))
					erro (INSTRUCAO_INVALIDA);
				else
					bytmem (0xc7 | (pilha [0].valor << 3));
			comp = 1;
			break;

		case RZ:
			bytmem (0xc8);
			comp = 1;
			break;

		case SBB:
			comp = monta1 (0x98);
			break;

		case SBCD:
			bytz80 (0xed, 0x43);
			parse_2_bytes ();
			comp = 4;
			break;

		case SBI:
			bytmem (0xde);
			parse_1_byte ();
			comp = 2;
			break;

		case SDED:
			bytz80 (0xed, 0x53);
			parse_2_bytes ();
			comp = 4;
			break;

		case SHLD:
			bytmem (0x22);
			parse_2_bytes ();
			comp = 3;
			break;

		case SIM:
			bytmem (0x30);
			comp = 1;
			break;

		case SIXD:
			bytz80 (0xdd, 0x22);
			parse_2_bytes ();
			comp = 4;
			break;

		case SIYD:
			bytz80 (0xfd, 0x22);
			parse_2_bytes ();
			comp = 4;
			break;

		case SLA:
			comp = monta9 (0x20);
			break;

		case SLP:
			bytz80 (0xed, 0x76);
			comp = 2;
			break;

		case SPHL:
			bytmem (0xf9);
			comp = 1;
			break;

		case SPIX:
			bytz80 (0xdd, 0xf9);
			comp = 2;
			break;

		case SPIY:
			bytz80 (0xfd, 0xf9);
			comp = 2;
			break;

		case SRA:
			comp = monta9 (0x28);
			break;

		case SRL:
			comp = monta9 (0x38);
			break;

		case SSPD:
			bytz80 (0xed, 0x73);
			parse_2_bytes ();
			comp = 4;
			break;

		case STA:
			bytmem (0x32);
			parse_2_bytes ();
			comp = 3;
			break;

		case STAX:
			monta4 (0x02);
			comp = 1;
			break;

		case STC:
			bytmem (0x37);
			comp = 1;
			break;

		case SUB:
			comp = monta1 (0x90);
			break;

		case SUI:
			bytmem (0xd6);
			parse_1_byte ();
			comp = 2;
			break;

		case TST:
			if ((atomo = analex ()) >= B && atomo <= A)
				{
				bytz80 (0xed, 0x04 | (atomo << 3));
				comp = 2;
				}
			else
				{
				volta_atomo (atomo);
				bytz80 (0xed, 0x64);
				parse_1_byte ();
				comp = 3;
				}
			break;

		case TSTIO:
			bytz80 (0xed, 0x74);
			parse_1_byte ();
			comp = 3;
			break;

		case XCHG:
			bytmem (0xeb);
			comp = 1;
			break;

		case XCHX:
			bytmem (0xd9);
			comp = 1;
			break;

		case XPSW:
			bytmem (0x08);
			comp = 1;
			break;

		case XRA:
			comp = monta1 (0xa8);
			break;

		case XRI:
			bytmem (0xee);
			parse_1_byte ();
			comp = 2;
			break;

		case XTHL:
			bytmem (0xe3);
			comp = 1;
			break;

		default:
			erro (INSTRUCAO_INVALIDA);
			return;
			}

		pmem += comp;
		pc += comp;
		}
	}

int monta1 (int oc)
	{
	t_atomo atomo, atomo1;

	if ((atomo = analex ()) >= B && atomo <= A)
		{
		bytmem (oc | atomo);
		return 1;
		}

	if (atomo == IX || atomo == IY)
		{
		if ((atomo1 = analex ()) == MAIS || atomo1 == MENOS)
			{
			if (atomo1 == MENOS)
				volta_atomo (atomo1);
			parse_s_byte ();
			}
		else
			volta_atomo (atomo1);
		bytz80 (atomo == IX ? 0xdd : 0xfd, oc | M);
		return 3;
		}

	volta_atomo (atomo);
	erro (INSTRUCAO_INVALIDA);
	return 1;
	}

void monta2 (int oc)
	{
	t_atomo atomo;

	if (((atomo = analex ()) >= B && atomo <= H) && !(atomo & 1))
		bytmem (oc | (atomo << 3));
	else
		if (atomo == SP)
			bytmem (oc | 0x30);
		else
			{
			volta_atomo (atomo);
			erro (INSTRUCAO_INVALIDA);
			}
	}

int monta3 (int oc)
	{
	t_atomo atomo, atomo1;

	if ((atomo = analex ()) >= B && atomo <= A)
		{
		bytmem (oc | (atomo << 3));
		return 1;
		}
	else if (atomo == IX || atomo == IY)
		{
		if ((atomo1 = analex ()) == MAIS || atomo1 == MENOS)
			{
			if (atomo1 == MENOS)
				volta_atomo (atomo1);
			if (!parse_s_byte ())
				return 0;
			}
		else
			volta_atomo (atomo1);
		bytz80 (atomo == IX ? 0xdd : 0xfd, oc | M << 3);
		return 3;
		}
	else
		{
		volta_atomo (atomo);
		erro (INSTRUCAO_INVALIDA);
		return 0;
		}
	}

void monta4 (int oc)
	{
	t_atomo atomo;

	if ((atomo = analex ()) == B || atomo == D)
		bytmem (oc | (atomo == B ? 0 : 0x10));
	else
		{
		volta_atomo (atomo);
		erro (INSTRUCAO_INVALIDA);
		}
	}

int monta5 (int oc)
	{
	t_atomo atomo;

	if (((atomo = analex ()) >= B && atomo <= H) && !(atomo & 1) || atomo == PSW)
		{
		bytmem (oc | (atomo == PSW ? 0x30 : (atomo << 3)));
		return 1;
		}
	else if (atomo == IX || atomo == IY)
		{
		bytz80 (atomo == IX ? 0xdd : 0xfd, oc | (H << 3));
		return 2;
		}
	else
		{
		volta_atomo (atomo);
		erro (INSTRUCAO_INVALIDA);
		return 0;
		}
	}

int monta6 (int oc)
	{
	t_atomo atomo;
	int comp;

	comp = 1;
	if (((atomo = analex ()) >= B && atomo <= H) && !(atomo & 1))
		bytmem (oc | (atomo << 3));
	else if (atomo == SP)
		bytmem (oc | 0x30);
	else if (atomo == IX || atomo == IY)
		{
		bytz80 (atomo == IX ? 0xdd : 0xfd, oc | (H << 3));
		comp = 2;
		}
	else
		{
		volta_atomo (atomo);
		erro (INSTRUCAO_INVALIDA);
		comp = 0;
		}
	return comp;
	}

void monta7 (int oc)
	{
	t_atomo atomo;
	int codg;

	if (((atomo = analex ()) >= B && atomo <= H) && !(atomo & 1))
		codg = oc | (atomo << 3);
	else
		if (atomo == SP)
			codg = oc | 0x30;
		else
			{
			volta_atomo (atomo);
			erro (INSTRUCAO_INVALIDA);
			return;
			}
	bytz80 (0xed, codg);
	}

void monta8 (int oc, t_atomo at)
	{
	t_atomo atomo;
	int codg;

	switch (atomo = analex ())
		{
	case B:
	case D:
		codg = oc | (atomo << 3);
		break;

	case SP:
		codg = oc | 0x30;
		break;

	default:
		if (atomo == at)
			codg = oc | (H << 3);
		else
			{
			volta_atomo (atomo);
			erro (INSTRUCAO_INVALIDA);
			return;
			}
		break;
		}

	bytz80 (at == IX ? 0xdd : 0xfd, codg);
	}

int monta9 (int oc)
	{
	t_atomo atomo, atomo1;

	if (!(cpu & (z80 | z180)))
		return 1;
	if ((atomo = analex ()) >= B && atomo <= A)
		{
		bytz80 (0xcb, oc | atomo);
		return 2;
		}
	else if (atomo == IX || atomo == IY)
		{
		if ((atomo1 = analex ()) == MAIS || atomo1 == MENOS)
			{
			if (atomo1 == MENOS)
				volta_atomo (atomo1);
			if (!parse_s_byte ())
				return 4;
			}
		else
			volta_atomo (atomo1);
		bytz80 (atomo == IX ? 0xdd : 0xfd, 0xcb);
		return 4;
		}
	else
		{
		volta_atomo (atomo);
		erro (INSTRUCAO_INVALIDA);
		return 2;
		}
	}

int monta10 (int oc)
	{
	t_atomo atomo;

	if (((atomo = analex ()) >= B && atomo <= L) || atomo == A)
		{
		bytz80 (0xed, oc | (atomo << 3));
		return 1;
		}
	else
		{
		volta_atomo (atomo);
		erro (PARAMETRO_INVALIDO);
		return 0;
		}
	}

void monta11 (int oc)
	{
	int err_exp;

	bytmem (oc);
	if (err_exp = parse_expr ())
		{
		erro (err_exp);
		return;
		}

	pilha [0].valor -= pc + 2;
	if (pilha [0].atr & definido)
		if (!(pilha [0].atr & alocatual) || (pilha [0].valor & ~0x7f && (pilha [0].valor & ~0x7f) != ~0x7f))
			erro (PARAMETRO_INVALIDO);
	}

int monta12 (void)
	{
	t_atomo atomo, atomo1, sinal;

	switch (atomo = analex ())
		{
	case A:
	case B:
	case C:
	case D:
	case E:
	case H:
	case L:
	case M:
		if (!espera_virgula ())
			return 1;
		switch (atomo1 = analex ())
			{
		case A:
		case B:
		case C:
		case D:
		case E:
		case H:
		case L:
		case M:
			if (atomo != M || atomo1 != M)
				bytmem (0x40 | (atomo << 3) | atomo1);
			else
				erro (INSTRUCAO_INVALIDA);
			return 1;

		case IX:
		case IY:
			if (atomo == M)
				{
				erro (INSTRUCAO_INVALIDA);
				return 3;
				}
			if ((sinal = analex ()) == MAIS || sinal == MENOS)
				{
				if (sinal == MENOS)
					volta_atomo (sinal);
				if (!parse_s_byte ())
					return 3;
				}
			else
				volta_atomo (sinal);
		bytz80 (atomo1 == IX ? 0xdd : 0xfd, 0x40 | (atomo << 3) | M);
		return 3;

		case I:
		case R:
			if (atomo == A)
				bytz80 (0xed, atomo1 == I ? 0x57 : 0x5f);
			else
				erro (INSTRUCAO_INVALIDA);
			return 2;

		default:
			volta_atomo (atomo1);
			erro (INSTRUCAO_INVALIDA);
			return 1;
			}

	case IX:
	case IY:
		if ((sinal = analex ()) == MAIS || sinal == MENOS)
			{
			if (sinal == MENOS)
				volta_atomo (sinal);
			if (!parse_s_byte ())
				return 3;
			}
		else
			volta_atomo (sinal);
		if (!espera_virgula ())
			return 3;
		if ((atomo1 = analex ()) >= B && atomo1 <= L || atomo1 == A)
			bytz80 (atomo == IX ? 0xdd : 0xfd, 0x40 | (M << 3) | atomo1);
		else
			{
			volta_atomo (atomo1);
			erro (INSTRUCAO_INVALIDA);
			}
		return 3;

	case I:
	case R:
		if (espera_virgula ())
			if ((atomo1 = analex ()) == A)
				bytz80 (0xed, atomo == I ? 0x47 : 0x4f);
			else
				{
				volta_atomo (atomo1);
				erro (INSTRUCAO_INVALIDA);
				}
		return 2;

	default:
		volta_atomo (atomo);
		erro (INSTRUCAO_INVALIDA);
		return 1;
		}
	}

void inicializa1 (char *nome)
	{
	int i;

	mod_name = NULL;			/* modulo a priori tem o nome do .rel */
	arqrel = arq = arq_sym = 0;
	iec = erros = 0;
	alocatual = COD;
	pcabs = pccod = pcdata = pc = 0;
	linha = 1;
	if_counter = 0;
	monta = 1;
	maclib = 0;
	definindo_macro = 0;
	catual = 0;
	mchar = 0;
	nmcaloc = 0;
	expressao_em_parametro = 0;
	passo_1 = 1;
	ppmac = 0;
	local_atual = 1;
	naloc = 0;
	atomos_colocados = 0;
	resta_simb = 0;
	nset_simb = 0;
	despreza_linha = 0;

	limpa_ts ();				/* limpa tabela de simbolos */

	pmem  = 0;
	nmem_aloc = 0;

	for (i = 0; i < sizeof nome_arq; i++)
		{
		nome_arq [i] = nome [i];
		if (nome_arq [i] >= 'a' && nome_arq [i] <= 'z')
			nome_arq [i] += ('A' - 'a');
		}
	nome_arq [sizeof (nome_arq) - 1] = '\0';
	inic_lex (nome);			/* inicializa analisador lexico */
	}

void inicializa2 (char *nome)
	{
	erros = 0;
	alocatual = COD;
	pc = 0;
	linha = 1;
	if_counter = 0;
	monta = 1;
	maclib = 0;
	definindo_macro = 0;
	catual = 0;
	passo_1 = 0;
	ppmac = 0;

	pmem = 0;
	inic_lex (nome);
	}

void erro (int erronum)
	{
	erro_linha (erronum);
	fim_da_linha ();
	}

void erro_linha (int erronum)
	{
	char *c;

	erros++;
	mprintf ("%s(%u) %s\n", maclib ? mac_arq : nome_arq, linha, erro_tab [erronum - 1]);
	c = next_linha;
	if (passo_1 && ppmac)
		{
		while (*c != '\n' && *c != _EOF)
			mprintf ("%c",*(c++));
		mprintf ("%c",'\n');
		}
	}

int espera_virgula (void)
	{
	t_atomo atomo;

	if ((atomo = analex ()) == VIRGULA)
		return 1;
	else
		{
		volta_atomo (atomo);
		erro (VIRGULA_ESPERADA);
		return 0;
		}
	}

void fim_da_linha (void)
	{
	t_atomo atomo;

	despreza_linha = 1;		/* faz analex ir mais rapido */
	while ((atomo = analex ()) != EOL && atomo != EOA);
	despreza_linha = 0;
	volta_atomo (atomo);
	}

void acerta_pont_old (void)
	{
	switch (alocatual)
		{
	case ABS:
		pcabs = pc;
		break;

	case COD:
		pccod = pc;
		break;

	case DAT:
		pcdata = pc;
		break;
		}
	}

void acerta_pc (void)
	{
	switch (alocatual)
		{
	case ABS:
		pc = pcabs;
		break;

	case COD:
		pc = pccod;
		break;

	case DAT:
		pc = pcdata;
		break;
		}
	}

void erro_fatal (int erronum)
	{
	if (tratando_pch)
		mprintf ("%s %s\n", mac_arq, erro_tab [erronum - 1]);
	else
		mprintf ("%s(%u) %s\n", maclib ? mac_arq : nome_arq, linha, erro_tab [erronum - 1]);
	grave ();
	}

void grave (void)
	{
	mprintf ("ABORTING\n");
	desaloca ();
	longjmp (finaliza, 2);
	}

void termina (void)
	{
	desaloca ();				/* desaloca memoria */
	if (!erros)
		{
		mprintf ("Assembly finished successfully.\n");
		longjmp (finaliza, 1);
		}

	if (erros == 1)
		mprintf ("Assembly finished with 1 error.\n");
	else
		mprintf ("Assembly finished with %u errors.\n", erros);

	longjmp (finaliza, 2);
	}

void define_irp (void)
	{
	macro_desc *m;
	t_atomo atomo;

	if ((atomo = analex ()) == NOME)
		if ((atomo = analex ()) == VIRGULA)
			{
			*nmacro = ppmac + '0';
			m = busca_macro (nmacro);
			strcpy (m -> nome, nmacro);
			m -> atrib |= (ocupado | definido);
			m -> pchar = mchar;
			m -> npar = 1;				/* numero de parametros */
			m -> ppar = catual;
			poe_nome (simbolo -> nome);
			pega_parametros (par_irp, &np_irp);
			definindo_macro = 1;		/* indica definindo macro */
			tipo_macro = 1;
			}
		else
			{
			volta_atomo (atomo);
			erro (VIRGULA_ESPERADA);
			}
	else
		{
		volta_atomo (atomo);
		erro (NOME_ESPERADO);
		}
	}

void define_rept (void)
	{
	macro_desc *m;
	int aux;

	if (!(aux = expressao ()))
		{
		*nmacro = ppmac + '0';
		m = busca_macro (nmacro);
		strcpy (m -> nome, nmacro);
		m -> atrib |= (ocupado | definido);
		m -> pchar = mchar;
		m -> npar = 0;				/* numero de parametros */
		m -> ppar = catual;
		nn_rept = pilha [0].valor;
		definindo_macro = 1;		/* indica definindo macro */
		tipo_macro = 2;			/* indica tipo rept */
		}
	else
		erro (aux);
	}

void define_macro (simb *memsimb)
	{
	definindo_macro = 1;		/* indica definindo macro */
	tipo_macro = 0;			/* indica definindo macro normal */
	coloca_macro (memsimb, &pmacro);	/* coloca macro na lista de macros */
	coloca_simbolos (&(pmacro -> npar), &(pmacro -> ppar));
	}

void coloca_macro (simb *s, macro_desc **m)
	{
	*m = busca_macro (s -> nome);
	strcpy ((*m) -> nome, s -> nome);
	(*m) -> atrib &= ~_MACLIB;
	(*m) -> atrib |= ocupado | definido | maclib;
	(*m) -> pchar = mchar;
	(*m) -> npar = 0;		/* zera numero de parametros */
	}

macro_desc *busca_macro (char n[])
	{
	int i, ent;

	for (i = ent = 0; n [i] != '\0'; i++)
		ent = hash (ent, n [i]);
	ent = (ent & 0xff) << 2;

	for (i = 0; i < max_macro; i++)
		{
		if (mfila[ent].atrib & ocupado)
			{
			if (!strcmp (n, mfila[ent].nome))
				return &mfila[ent];
			}
		else
			return &mfila[ent];
		ent++;
		if (ent >= max_macro)
			ent = 0;
		}
	if (i >= max_macro)
		erro_fatal (NUMERO_DE_MACROS_EXCEDIDO);
	return NULL;			// Just to make the compiler happy.
	}

/*****************************************************************************
	guarda_car ():

	Guarda caracter em memoria de macro.

*****************************************************************************/

void guarda_car (char c)
	{
	static int char_colocados = 0;

	if (!(mchar++ & msk_rot_macro_char))
		{
		if (nmcaloc >= max_char_aloc)
			erro_fatal (MUITO_CODIGO_EM_MACROS);
		if ((tab_end_chars [nmcaloc] = (char *) malloc (msk_rot_macro_char + 1)) == NULL)
			erro_fatal (MEMORIA_INSUFICIENTE);
		char_colocados = 0;
		nmcaloc++;
		}

	*(tab_end_chars [nmcaloc - 1] + char_colocados++) = c;
	}

void coloca_simbolos (int *n, int *p)
	{
	t_atomo atomo;

	do								/* pega parametros */
		if ((atomo = analex ()) == NOME)
			{
			if (!((*n)++))
				*p = catual;
			poe_nome (simbolo -> nome);
			}
		else
			if (atomo == VIRGULA)
				erro (NOME_ESPERADO);
			else
				volta_atomo (atomo);
	while ((atomo = analex ()) == VIRGULA);
	volta_atomo (atomo);
	}

void coloca_locais (void)
	{
	t_atomo atomo;
	int chegou_nome;

	chegou_nome = 0;
	do								/* pega parametros */
		if ((atomo = analex ()) == NOME)
			{
			poe_local (simbolo -> nome);
			chegou_nome = 1;
			}
		else
			if (atomo == VIRGULA)
				erro (NOME_ESPERADO);
			else
				volta_atomo (atomo);
	while ((atomo = analex ()) == VIRGULA);
	volta_atomo (atomo);
	if (!chegou_nome)
		erro (NOME_ESPERADO);
	}

void poe_local (char *n)
	{
	while (clocais < max_clocais - 3 && *n != '\0')
		nomes_locais [clocais++] = *(n++);
	if (clocais >= max_clocais - 3)
		erro (MUITOS_LOCAIS);
	else
		{
		nomes_locais [clocais++] = '\0';
		nomes_locais [clocais++] = local_atual & 0xff;
		nomes_locais [clocais++] = (local_atual >> 8) & 0xff;
		mac_nl++;			/* incrementa numero de locais da macro atual */
		local_atual++;		/* incrementa numero de locais atual */
		}
	}

void poe_nome (char *n)
	{
	do
		if (catual >= max_macro_par)
			erro_fatal (MACROS_COM_MUITOS_PARS);
		else
			macro_par [catual++] = *n;
	while (*(n++) != '\0');
	}

/*****************************************************************************
	salva_estado ()

	Salva estado atual do assembler para expandir macro.

*****************************************************************************/

void salva_estado (void)
	{
	unsigned int i;

	if (ppmac > max_mac_call)
		erro_fatal (MUITAS_MACROS_SENDO_EXPANDIDAS);
	mac_pilha [ppmac].if_counter = if_counter;
	mac_pilha [ppmac].monta = monta;
	for (i = 0; i < if_size; i++)
		mac_pilha [ppmac].if_stack [i] = if_stack [i];
	mac_pilha [ppmac].clocais = clocais;
	for (i = 0; i < clocais; i++)
		mac_pilha [ppmac].nomes_locais [i] = nomes_locais [i];
	mac_pilha [ppmac].nparm = nparm;
	mac_pilha [ppmac].mac_np = mac_np;
	mac_pilha [ppmac].mac_pp = mac_pp;
	mac_pilha [ppmac].mac_nl = mac_nl;
	for (i = 0; i < max_car_par; i++)
		mac_pilha [ppmac].mpar [i] = mpar [i];
	mac_pilha [ppmac].m_loops = m_loops;
	mac_pilha [ppmac].n_rept = n_rept;
	mac_pilha [ppmac].exp_tipo_macro = exp_tipo_macro;
	mac_pilha [ppmac++].mac_char = mac_char;
	if_counter = 0;
	clocais = 0;
	monta = 1;
	}

/*****************************************************************************
	recupera_estado ()

	Recupera estado atual do assembler para expandir macro.

*****************************************************************************/

void recupera_estado (void)
	{
	unsigned int i;

	if_counter = mac_pilha [--ppmac].if_counter;
	monta = mac_pilha [ppmac].monta;
	for (i = 0; i < if_size; i++)
		if_stack [i] = mac_pilha [ppmac].if_stack [i];
	clocais = mac_pilha [ppmac].clocais;
	for (i = 0; i < clocais; i++)
		nomes_locais [i] = mac_pilha [ppmac].nomes_locais [i];
	nparm = mac_pilha [ppmac].nparm;
	mac_np = mac_pilha [ppmac].mac_np;
	mac_pp = mac_pilha [ppmac].mac_pp;
	mac_nl = mac_pilha [ppmac].mac_nl;
	for (i = 0; i < max_car_par; i++)
		mpar [i] = mac_pilha [ppmac].mpar [i];
	m_loops = mac_pilha [ppmac].m_loops;
	n_rept = mac_pilha [ppmac].n_rept;
	exp_tipo_macro = mac_pilha [ppmac].exp_tipo_macro;
	mac_char = mac_pilha [ppmac].mac_char;
	}

/*****************************************************************************
	pega_parametros ()

	Pega parametros da macro a ser expandida.

*****************************************************************************/

void pega_parametros (char *p, int *np)
	{
	converte = 0;
	get_par (p, np);
	converte = 1;
	}

/*****************************************************************************
	get_par ()

	Pega parametros da macro a ser expandida.

*****************************************************************************/

void get_par (char *p, int *np)
	{
	int escape, literal, valendo, i, posicao_final, rstring;
	char c, cstring;
	char *ns, numero [(sizeof (long)) * 8 + 1];
	int pos_numero, aux, salva;
	t_atomo atomo;

	literal = valendo = 0;
	*np = 0;						/* numero de parametros da macro */
	i = 0;						/* posicao atual no buffer de parametros */
	posicao_final = 0;		/* local onde deve ser colocado o '\0' */
	escape = 0;

	if (setjmp (jmp_erro))
		{
		erro (MUITOS_PARAMETROS);
		return;
		}

	while (1)
		switch (c = le_car ())
			{
		case ' ':
		case '\t':
			if (literal)
				escape = 0;
			if (valendo)
				coloca_car (p, c, &i, np);
			if (escape)
				{
				posicao_final = i;
				escape = 0;
				}
			break;

		case '%':
			if (valendo)
				{
				coloca_car (p, c, &i, np);
				escape = 0;
				valendo = 1;
				posicao_final = i;
				}
			else
				{
				salva = converte;
				converte = 1;
				expressao_em_parametro = 1;
				aux = expressao ();
				expressao_em_parametro = 0;
				converte = salva;
				if (aux)
					{
					erro (aux);
					return;
					}
				if (!(pilha [0].atr & ABS))
					{
					erro (EXPRESSAO_INVALIDA);
					return;
					}
				if ((atomo = analex ()) != VIRGULA && atomo != EOL)
					{
					erro (EXPRESSAO_INVALIDA);
					return;
					}
				devolve ();		/* devolve ',' ou '\n' */
				ns = ultoa (((unsigned long) pilha [0].valor) & ((1LL << (8 * (sizeof (int)))) - 1), numero, 10);
				pos_numero = 0;
				while (*ns != '\0')
					{
					if (pos_numero || *ns != '0')
						{
						pos_numero = 1;
						coloca_car (p, *ns, &i, np);
						}
					ns++;
					}
				if (!pos_numero)
					coloca_car (p, '0', &i, np);
				escape = 0;
				valendo = 1;
				posicao_final = i;
				}
			break;

		case '<':
			if (escape)
				{
				coloca_car (p, c, &i, np);
				posicao_final = i;
				escape = 0;
				}
			else
				if (literal++)
					coloca_car (p, c, &i, np);
				else
					valendo = 1;
			break;

		case '>':
			if (escape)
				{
				escape = 0;
				coloca_car (p, c, &i, np);
				}
			else
				if (literal)
					{
					if (--literal)
						coloca_car (p, c, &i, np);
					}
				else
					{
					valendo = 1;
					coloca_car (p, c, &i, np);
					}
			posicao_final = i;
			break;

		case '\'':
		case '\"':
			if (escape)
				{
				coloca_car (p, c, &i, np);
				escape = 0;
				}
			else
				{
				coloca_car (p, c, &i, np);
				cstring = c;
				valendo = rstring = 1;
				do
					{
					switch (c = le_car ())
						{
					case '\\':
						escape = !escape;
						break;

					case '\'':
					case '\"':
						if (!escape && c == cstring)
							rstring--;
						escape = 0;
						break;

					case '\n':
						devolve ();			/* devolve caracter para nao dar erro no fim da linha */
						coloca_car (p, '\0', &i, np);
						(*np)++;
						return;

					default:
						escape = 0;
						}
					coloca_car (p, c, &i, np);
					posicao_final = i;
					}
				while (rstring);
				posicao_final = i;
				}
			break;

		case '!':
			if (!(escape = !escape))
				{
				coloca_car (p, c, &i, np);
				posicao_final = i;
				}
			else
				valendo = 1;
			break;

		case ',':
			if (literal || escape)
				{
				escape = 0;
				coloca_car (p, c, &i, np);
				posicao_final = i;
				}
			else
				{
				i = posicao_final;
				coloca_car (p, '\0', &i, np);
				(*np)++;
				valendo = 0;
				}
			break;

		case ';':
			if (literal || escape)
				{
				escape = 0;
				coloca_car (p, c, &i, np);
				posicao_final = i;
				break;
				}

		case '\n':
			devolve ();			/* devolve caracter para nao dar erro no fim da linha */
			i = posicao_final;
			coloca_car (p, '\0', &i, np);
			if (valendo)
				(*np)++;
			return;

		default:
			coloca_car (p, c, &i, np);
			escape = 0;
			valendo = 1;
			posicao_final = i;
			}
	}

/*****************************************************************************
	coloca_car ()

	Coloca caracteres de parametros no buffer de parametros, dando erro se
estourou o buffer.

*****************************************************************************/

void coloca_car (char *p, char c, int *i, int *np)
	{
	if (*i >= max_car_par)
		{
		if (p [max_car_par - 1] != '\0')
			{
			p [max_car_par - 1] = '\0';
			(*np)++;
			}
		longjmp (jmp_erro, 1);
		}
	else
		p [(*i)++] = c;
	}

/*****************************************************************************
	parse_byte ():

	Faz parsing de expressao que deve ter apenas 1 byte.

*****************************************************************************/

int parse_byte (void)
	{
	int err_exp;

	if (err_exp = parse_expr ())
		{
		erro (err_exp);
		return 0;
		}
	else
		if (pilha [0].atr & definido)
			if (!((pilha [0].atr & ABS) && !(pilha [0].valor & ~0xff)))
				{
				erro (PARAMETRO_INVALIDO);
				return 0;
				}
	return 1;
	}

/*****************************************************************************
	parse_s_byte ():

	Faz parsing de expressao que deve ter apenas 1 byte com sinal.

*****************************************************************************/

int parse_s_byte (void)
	{
	int err_exp;

	if (err_exp = parse_expr ())
		{
		erro (err_exp);
		return 0;
		}

	if (pilha [0].atr & definido)
		if (!((pilha [0].atr & ABS) && (!(pilha [0].valor & ~0x7f) || ((pilha [0].valor & ~0x7f) == ~0x7f))))
			{
			erro (PARAMETRO_INVALIDO);
			return 0;
			}
	return 1;
	}

/*****************************************************************************
	parse_1_byte ():

	Faz parsing de expressao que deve ter apenas 1 byte.

*****************************************************************************/

void parse_1_byte (void)
	{
	int err_exp;

	if (err_exp = parse_expr ())
		erro (err_exp);
	else
		if (pilha [0].atr & definido)
			if (!((pilha [0].atr & ABS) && (!(pilha [0].valor & ~0xff) || ((pilha [0].valor & ~0x7f) == ~0x7f))))
				erro (PARAMETRO_INVALIDO);
	}

/*****************************************************************************
	parse_2_bytes ():

	Faz parsing de expressao.

*****************************************************************************/

void parse_2_bytes (void)
	{
	int err_exp;

	if (err_exp = parse_expr ())
		erro (err_exp);
	}

/*****************************************************************************
	bytz80 ():

	Coloca 2 bytes na memoria.

*****************************************************************************/

void bytz80 (unsigned char byte1, unsigned char byte2)
	{
	coloca_byte (byte1, pmem);
	coloca_byte (byte2, pmem + 1);
	}

/*****************************************************************************
	coloca_byte ():

	Coloca byte na memoria.

*****************************************************************************/

void coloca_byte (unsigned char byte, unsigned int posicao)
	{
	while (nmem_aloc < ((posicao >> bit_rot_mem) & ((1 << (16 - bit_rot_mem))) - 1) + 1)
		{
		if (nmem_aloc >= max_mem_aloc)
			erro_fatal (PROGRAMA_MUITO_GRANDE);
		if ((tab_mem_aloc [nmem_aloc] = (unsigned char *) malloc (masc_mem + 1)) == NULL)
			erro_fatal (MEMORIA_INSUFICIENTE);
		nmem_aloc++;
		}

	*(tab_mem_aloc [nmem_aloc - 1] + (posicao & masc_mem)) = byte;
	}

/*****************************************************************************
	void trata_include ():

	Dado nome do arquivo a ser incluido, acerta variaveis de estado, inicializa
 analisador lexico e abre arquivo correto.

*****************************************************************************/

void trata_include (char *inc_file)
	{
	char *caux;				/* auxiliar */
	char str_aux [sizeof string];

	if (usa_pch)
		{
		caux = poe_ext (strcpy (str_aux, inc_file), "PCH");	/* forca extensao PCH */
		if (inc_path == NULL || (caux = arq_include (inc_path, caux)) == NULL || strlen (caux) >= sizeof mac_arq)
			strcpy (mac_arq, caux);	/* usa nome passado se nao existe arquivo ou nome muito grande */
		else
			strcpy (mac_arq, caux);		/* usa novo nome se arquivo existe e nome nao e' muito grande */
		if (trata_pch (strupr (mac_arq)))
			return;							/* retorna caso tratou arquivo */
		}

	maclib = _MACLIB;		/* indica que esta' tratando arquivo incluido */
	salva_lex ();			/* salva estado do analisador lexico */
	if_maclib = if_counter;		/* salva estado do contador de condicionais */
	linha_atual = linha;		/* salva linha atual */

	if (inc_path == NULL || (caux = arq_include (inc_path, inc_file)) == NULL || strlen (caux) >= sizeof mac_arq)
		strcpy (mac_arq, inc_file);	/* usa nome passado se nao existe arquivo ou nome muito grande */
	else
		strcpy (mac_arq, caux);		/* usa novo nome se arquivo existe e nome nao e' muito grande */

	inic_lex (strupr (mac_arq));		/* inicia analisador lexico */
	linha = 1;					/* indica na primeira linha */
	volta_atomo (EOL);		/* para nao dar erro de instrucao invalida */
	}

/*****************************************************************************
	int trata_pch ():

	Dado arquivo de cabecalho pre compilado, ajusta tabela de simbolos e de
macros.

*****************************************************************************/

int trata_pch (char *pch)
	{
	macro_desc *m;
	unsigned char byte;
	char nome [comp_max + 1];
	int i, j;

	if (!inicia_pch (pch))
		return 0;						/* caso nao conseguiu abrir arquivo */

	/* pega simbolos */
	while ((byte = le_byte_pch ()) != '\0')
		{
		i = 0;
		do
			nome [i++] = byte;
		while ((byte = le_byte_pch ()) != '\0' && i < sizeof nome);
		if (i >= sizeof nome)
			erro_fatal (ARQUIVO_PCH_INCONSISTENTE);
		nome [i] = '\0';
		ver_nome (nome, i);			/* procura o simbolo na tabela de simbolos */
		if (simbolo -> atrib & definido)			/* caso simbolo ja' definido */
			{
			erros++;
			mprintf ("%s %s: %s\n", mac_arq, nome, erro_tab [DUPLA_DEFINICAO - 1]);
			le_byte_pch ();							/* sincroniza arquivo */
			le_word_pch ();							/* sincroniza arquivo */
			}
		else												/* caso simbolo ainda nao definido */
			{
			simbolo -> atrib |= le_byte_pch () | _MACLIB;	/* pega atributo */
			simbolo -> valor = le_word_pch ();					/* pega valor */
			}
		}

	/* pega macros */
	while ((byte = le_byte_pch ()) != '\0')
		{
		i = 0;
		do
			nome [i++] = byte;
		while ((byte = le_byte_pch ()) != '\0' && i < sizeof nome);
		if (i >= sizeof nome)
			erro_fatal (ARQUIVO_PCH_INCONSISTENTE);
		nome [i] = '\0';
		m = busca_macro (nome);
		strcpy (m -> nome, nome);
		m -> atrib = le_byte_pch () | _MACLIB;
		m -> pchar = mchar;
		m -> npar = le_word_pch ();
		/* pega parametros da macro */
		for (j = 0; j < m -> npar; j++)
			{											/* pega parametros */
			for (i = 0; ((byte = le_byte_pch ()) != '\0') && i < sizeof nome; i++)
				nome [i] = byte;
			if (i >= sizeof nome)
				erro_fatal (ARQUIVO_PCH_INCONSISTENTE);
			nome [i] = '\0';
			if (!j)
				m -> ppar = catual;		/* indica primeiro parametro */
			poe_nome (nome);				/* coloca parametro */
			}
		/* pega texto da macro */
		while ((byte = le_byte_pch ()) != '\0')
			guarda_car (byte);
		}
	close (arq_pch);		/* fecha arquivo */
	tratando_pch = 0;		/* indica que nao esta' mais tratando pch */
	return 1;				/* indica que tratou arquivo */
	}

/*****************************************************************************
	int inicia_pch ():

	Faz inicializacoes de arquivo pch para poder ler.

*****************************************************************************/

int inicia_pch (char *nome)
	{
	if ((arq_pch = open (nome, O_BINARY | O_RDONLY)) == -1)
		return 0;						/* caso nao existe arquivo */
	tratando_pch = 1;			/* indica tratando pch (para efeitos de erro) */
	num_pch = 0;
	return 1;
	}

/*****************************************************************************
	unsigned char le_byte_pch ()

	Faz leitura de um byte do arquivo de entrada.

*****************************************************************************/

unsigned char le_byte_pch (void)
	{
	if (!num_pch)
		{
		num_pch = read (arq_pch, buffer_pch, sizeof buffer_pch);
		le_pch = 0;
		if (!num_pch)
			erro_fatal (ARQUIVO_PCH_INCONSISTENTE);
		}
	num_pch--;
	return buffer_pch [le_pch++];
	}

/*****************************************************************************
	unsigned int le_word_pch ()

	Faz leitura de uma word do arquivo de entrada.

*****************************************************************************/

unsigned int le_word_pch (void)
	{
	unsigned char byte;

	byte = le_byte_pch ();
	return (le_byte_pch () << 8) + byte;
	}

/*****************************************************************************
	char *arq_include ():

	Dados paths (separados por ';') e nome de arquivo, volta com ponteiro
 para o nome do primeiro arquivo encontrado. Caso nao encontre nenhum arquivo
 volta com NULL.

*****************************************************************************/

char *arq_include (char *path_list, char *arq)
	{
	static char arq_aux [128];
	int i = 0;
	int j;

	if (exist (arq))
		return arq;

	while (path_list [i] != '\0')
		{
		j = 0;
		while (path_list [i] != ';' && path_list [i] != '\0')
			arq_aux [j++] = path_list [i++];
		if (path_list [i] == ';')
			i++;						/* aponta para proximo path, se nao encontrar arquivo */
		if (j)						/* deixa apenas um '\' separando path e arquivo */
			if (arq [0] != '\\')
				{
				if (arq_aux [j - 1] != '\\')
					arq_aux [j++] = '\\';
				}
			else
				{
				if (arq_aux [j - 1] == '\\')
					j--;
				}
		if (j + strlen (arq) + 1 <= sizeof arq_aux)
			{
			strcpy (arq_aux + j, arq);
			if (exist (arq_aux))
				return arq_aux;
			}
		}

	return NULL;			/* nao existe arquivo */
	}

/*****************************************************************************
	int exist ():

	Dado nome do arquivo, volta com 0 caso nao exista.

*****************************************************************************/

int exist (char *arq)
	{
//nfgf	int aux;
//nfgf	if ((aux = _chmod (arq, 0)) == -1)
//nfgf		return 0;
//nfgf	return aux & (FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC) ? 0 : 1;
	struct stat   buffer;
	return stat(arq, &buffer);
	}

/*****************************************************************************
	void constroi_pch ()

	Monta arquivo de pre compiled header de saida a partir das tabelas de
simbolos e de macros.

*****************************************************************************/

void constroi_pch (void)
	{
	int sn;
	simb *s;

	inicia_saida ();			/* faz inicializacoes devidas */

	if (nset_simb)				/* so' faz alguma coisa se existe simbolo */
		for (sn = 0; sn < ((nset_simb - 1) << nrot_aloc) + (nsimb_aloc - resta_simb); sn++)
			if (((s = aloc_simb [(sn >> nrot_aloc) & c_mask_aloc] + (sn & mask_aloc)) -> atrib & (definido | _MACLIB)) == definido)
				simbolo_pch (s);
	w_byte (0);					/* indica final da tabela de simbolos */

	for (sn = 0; sn < max_macro; sn++)
		if ((mfila[sn].atrib & (ocupado | definido | _MACLIB)) == (ocupado | definido))
				macro_pch (&mfila [sn]);
	w_byte (0);					/* indica final da tabela de macros */

	if (csaida)
		escreve (arqrel, saida, csaida);	/* para compatibilidade com linc80 */
	close (arqrel);
	arqrel = 0;
	}

/*****************************************************************************
	void simbolo_pch ()

	Dado ponteiro de simbolo, manda informacoes para arquivo de saida.

*****************************************************************************/

void simbolo_pch (simb *s)
	{
	int i;

	for (i = 0; s -> nome [i] != '\0'; i++)
		w_byte (s -> nome [i]);
	w_byte ('\0');
	w_byte (s -> atrib);
	w_word (s -> valor);
	}

#define char_macro(i)	*(tab_end_chars \
								[(i >> n_rot_macro_char) & cmsk_rot_macro_char] \
								+ (i & msk_rot_macro_char))

/*****************************************************************************
	void macro_pch ()

	Dado ponteiro de macro, manda informacoes para arquivo de saida.

*****************************************************************************/

void macro_pch (macro_desc *m)
	{
	int i;
	unsigned int j;

	for (i = 0; m -> nome [i] != '\0'; i++)
		w_byte (m -> nome [i]);
	w_byte ('\0');						/* manda nome da macro */
	w_byte (m -> atrib);				/* manda atributo */
	w_word (m -> npar);				/* manda numero de parametros da macro */
	for (i = 0, j = m -> ppar; i < m -> npar; i++)
		{											/* manda parametros da macro */
		while (macro_par [j] != '\0')
			w_byte (macro_par [j++]);
		j++;
		w_byte ('\0');
		}
	for (i = m -> pchar; char_macro (i) != '\0'; i++)	/* manda texto da macro */
		w_byte (char_macro (i));
	w_byte ('\0');
	}

/*****************************************************************************
	void w_byte ()

	Manda byte para arquivo de saida.

*****************************************************************************/

void w_byte (unsigned char byte)
	{
	saida [csaida] = byte;
	if (++csaida >= sizeof saida)
		{
		escreve (arqrel, saida, sizeof saida);
		csaida = 0;
		}
	}

/*****************************************************************************
	void w_word ()

	Manda word para arquivo de saida.

*****************************************************************************/

void w_word (unsigned int word)
	{
	w_byte ((unsigned char) word);
	w_byte ((unsigned char) (word >> 8));
	}

