#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lb80.h"

/* variaveis globais inicializadas */
int falta = 0;
int voltou = 0;
int resta_simb = 0;
int nset_simb = 0;
int ignora_simbolo = 0;

char *_argv [256];
char buf_prj [2048 + 1];

void cdecl main (int argc, char *argv [])
	{
	int arq, tam_arq, _argc, i, i_aux, tem_parametro;
	char *mensagem = "LB80: Gerenciador de bibliotecas - Versao 2.00\n";

	while (*mensagem != '\0')		/* imprime mensagem */
		putc (*(mensagem++), stderr);

	if (argc == 2 && *argv [1] == '@')
		{
		if ((arq = open (argv [1] + 1, O_TEXT | O_RDONLY)) == -1)
			{
			mprintf ("Failed to open file %s\n", argv [1] + 1);
			exit (1);
			}
		tam_arq = read (arq, buf_prj, (sizeof buf_prj) - 1);
		if (read (arq, buf_prj, sizeof buf_prj))
			{
			close (arq);
			mprintf ("The file is too large: %s\n", argv [1] + 1);
			exit (1);
			}
		close (arq);
		_argv [0] = argv [0];
		_argc = 1;
		for (i = tem_parametro = 0; i < tam_arq; i++)
			if (buf_prj [i] == ' ' || buf_prj [i] == '\t' || buf_prj [i] == '\n')
				{
				if (tem_parametro)
					{
					_argv [_argc++] = buf_prj + i_aux;
					buf_prj [i] = '\0';
					tem_parametro = 0;
					if (_argc >= (sizeof (_argv)) / (sizeof (char *)))
						{
						mprintf ("Too many parameters in %s\n", argv [1] + 1);
						exit (1);
						}
					}
				}
			else
				if (!tem_parametro)
					{
					i_aux = i;
					tem_parametro = 1;
					}
		if (tem_parametro)
			{
			_argv [_argc++] = buf_prj + i_aux;
			buf_prj [i] = '\0';
			if (_argc >= (sizeof (_argv)) / (sizeof (char *)))
				{
				mprintf ("Too many parameters in %s\n", argv [1] + 1);
				exit (1);
				}
			}
		faz_lib (_argc, _argv);
		}
	else
		faz_lib (argc, argv);
	}

/*****************************************************************************
	faz_lib ()

	Monta biblioteca baseada nos parametros passados.

*****************************************************************************/

void faz_lib (int argc, char *argv [])
	{
	limpa_ts ();						/* limpa tabela de simbolos */

	if (argc == 1)
		{
		explica ();
		termina (0);
		}

	if (argc == 2)						/* caso informacoes sobre biblioteca */
		{
		informa (argv [1]);
		termina (0);
		}

	edita (argc - 1, argv + 1);	/* edita arquivo */

	termina (0);
	}

/*****************************************************************************
	edita ()

	Edita arquivo de biblioteca.

*****************************************************************************/

void edita (int npar, char *par [])
	{
	int ncom;		/* numero de comandos */
	char **com;		/* ponteiro para primeiro comando */
	char *temp = "lbXXXXXX";
	int arqout;		/* arquivo de saida */
	int arqn, arqo;	/* indica se tem ou nao arquivos velho e novo */
	int i;

	if (*par [0] == '+')
		{
		ncom = npar;
		com = par;
		arqo = 0;
		}
	else
		{
		ncom = npar - 1;
		com = par + 1;
		arqo = 1;
		}

	if (*par [npar - 1] != '+' && *par [npar - 1] != '-' && *par [npar - 1] != '*')
		{
		ncom--;
		arqn = 1;
		}
	else
		{
		arqn = 0;
		if (!arqo)				/* erro de chamada: nao ha arquivo novo nem velho */
			{
			mprintf ("Output file is missing.\n");
			termina (1);
			}
		}

	if ((arqout = open (temp = mktemp (temp), O_TRUNC | O_CREAT | O_BINARY | O_WRONLY, S_IWRITE)) == -1)
		{
		mprintf ("Failed to open temporary file.\n");
		termina (1);
		}

	inicia_saida0 ();
	if (arqo)
		tira_subst (par [0], arqout, com, ncom);
	inclui (arqout, com, ncom);
	fecha_saida0 (arqout);
	copy (nomeok (arqn ? par [npar - 1] : par [0], "lib"), temp);	/* 1.02 */
	unlink (temp);
	for (i = 0; i < ncom; i++, com++)
		if (*com != NULL)
			mprintf ("WARNING: command %s was ignored.\n", *com);
	}

/*****************************************************************************
	tira_subst ()

	Faz retiradas e substituicoes de modulos do arquivo original.

*****************************************************************************/

void tira_subst (char *arq, int arqout, char **com, int ncom)
	{
	int arqrel;
	int atomo;
	char *file;

	if (!inicia (nomeok (arq, "lib")))
		termina (1);
	while ((atomo = analex ()) == PROGRAM_NAME)
		{
		volta (atomo);
		switch (busca_t_s (simbolo -> nome, com, ncom))
			{
		case 0:										/* excluir modulo */
			ignora_simbolo = 1;
			parse_mod (-1, -1);
			break;

		case 1:										/* substituir modulo */
			salva_analex ();
			if (!inicia (nomeok (simbolo -> nome, "rel")))
				termina (1);
			parse_mod (arqout, -1);
			close (lib_file);
			recupera_analex ();
			ignora_simbolo = 1;
			parse_mod (-1, -1);
			break;

		case 2:										/* copiar modulo em .rel */
			if ((arqrel = open (file = nomeok (simbolo -> nome, "rel"), O_TRUNC | O_CREAT | O_BINARY | O_WRONLY, S_IWRITE)) == -1)
				{
				mprintf ("Failed to open file %s\n", str_maiuscula (file));
				termina (1);
				}
			inicia_saida1 ();
			parse_mod (arqout, arqrel);
			fecha_saida1 (arqrel);
			break;

		case 3:										/* excluir modulo e copiar em .rel */
			if ((arqrel = open (file = nomeok (simbolo -> nome, "rel"), O_TRUNC | O_CREAT | O_BINARY | O_WRONLY, S_IWRITE)) == -1)
				{
				mprintf ("Failed to open file %s\n", str_maiuscula (file));
				termina (1);
				}
			inicia_saida1 ();
			ignora_simbolo = 1;
			parse_mod (-1, arqrel);
			fecha_saida1 (arqrel);
			break;

		default:										/* nao ha operacao */
			parse_mod (arqout, -1);
			}
		}
	if (atomo != END_FILE)
		{
		erro ();
		termina (1);
		}
	}

/*****************************************************************************
	inclui ()

	Faz insercoes de modulos no arquivo de saida.

*****************************************************************************/

void inclui (int arq, char **com, int ncom)
	{
	char *arqrel;
	int atomo;

	while ((arqrel = busca_i (com, ncom)) != NULL)
		{
		if (!inicia (nomeok (arqrel, "rel")))
			termina (1);
		while (1)
			{
			parse_mod (arq, -1);
			if ((atomo = analex ()) == END_FILE)
				break;
			else
				volta (atomo);
			}
		close (lib_file);
		}
	}

/*****************************************************************************
	busca_i ()

	Busca insercoes para serem feitas.

*****************************************************************************/

char *busca_i (char **com, int ncom)
	{
	int i;
	char *aux;

	for (i = 0; i < ncom; i++, com++)
		if (*com != NULL && **com == '+')
			{
			aux = *com + 1;
			*com = NULL;
			return aux;
			}
	return NULL;
	}

/*****************************************************************************
	busca_t_s ()

	Busca delecoes ou substituicoes na linha de comando.

*****************************************************************************/

int busca_t_s (char *nome, char **com, int ncom)
	{
	int i;

	for (i = 0; i < ncom; i++, com++)
		if (*com != NULL)
			if (**com == '-')
				{
				if (*(*com + 1) == '+')
					{
					if (!stricmp (*com + 2, nome))
						{
						*com = NULL;
						return 1;
						}
					}
				else if (*(*com + 1) == '*')
					{
					if (!stricmp (*com + 2, nome))
						{
						*com = NULL;
						return 3;
						}
					}
				else
					if (!stricmp (*com + 1, nome))
						{
						*com = NULL;
						return 0;
						}
				}
			else
				if (**com == '*' && !stricmp (*com + 1, nome))
					{
					*com = NULL;
					return 2;
					}
	return -1;
	}

/*****************************************************************************
	parse_mod ()

	Faz parsing de modulo, escrevendo saida em arquivos.

*****************************************************************************/

void parse_mod (int arq1, int arq2)
	{
	nome_mod (arq1, arq2);
	entry_symbols (arq1, arq2);
	data_size (arq1, arq2);
	program_size (arq1, arq2);
	programa (arq1, arq2);
	define_entry_point (arq1, arq2);
	chain_external (arq1, arq2);
	end_module (arq1, arq2);
	ignora_simbolo = 0;
	}

/*****************************************************************************
	explica ()

	Imprime mensagem com basico sobre o comando.

*****************************************************************************/

void explica (void)
	{
	mprintf ("\nCall instructions:\n");
	mprintf ("lb80 [old file] [options] [new file]\n");
	mprintf (" Opcoes : +arq    add file to library\n");
	mprintf ("          -mod    remove file from library\n");
	mprintf ("          -+mod   replace module by file mod.rel\n");
	mprintf ("          *mod    copy the module contents into file mod.rel\n");
	mprintf ("          -*mod   remove module and copy it into mod.rel\n");
	}

/*****************************************************************************
	informa ()

	Imprime informacoes sobre biblioteca na tela.

*****************************************************************************/

void informa (char *lib)
	{
	int atomo;

	if (!inicia (nomeok (lib, "lib")))
		termina (1);
	while (1)
		{
		parse_mod (-1, -1);
		if ((atomo = analex ()) == END_FILE)
			{
			close (lib_file);
			manda_inf ();
			return;
			}
		else
			volta (atomo);
		}
	}

/*****************************************************************************
	nome_mod ()

	Pega nome do modulo.

*****************************************************************************/

void nome_mod (int arq1, int arq2)
	{
	if (analex () == PROGRAM_NAME)
		{
		if (mod_cont >= max_mod)
			{
			mprintf ("FATAL ERROR: the maximum numer of modules was exceeded.\n");
			termina (1);
			}
		if (!ignora_simbolo)
			{
			if (simbolo -> atrib & MODULO)
				mprintf ("WARNING: module %s was redefined.\n", simbolo -> nome);
			simbolo -> atrib |= MODULO;	/* indica se tratar de um modulo */
			strcpy (modulo [mod_cont++].nome , simbolo -> nome);
			}
		manda_nome (simbolo -> nome, arq1, 0);
		manda_nome (simbolo -> nome, arq2, 1);
		}
	else
		{
		erro ();
		termina (1);
		}
	}

/*****************************************************************************
	entry_symbols ()

	Pega simbolos publicos

*****************************************************************************/

void entry_symbols (int arq1, int arq2)
	{
	int atomo;

	while ((atomo = analex ()) == ENTRY_SYMBOL)
		{
		if (simbolo -> atrib & PUBLICO)
			{
			if (!ignora_simbolo)
				mprintf ("WARNING: symbol %s defined in modules %s and %s.\n", simbolo -> nome, modulo [mod_cont - 1].nome,
				 modulo [simbolo -> modulo].nome);
			}
		else
			simbolo -> modulo = mod_cont - 1;
		simbolo -> atrib |= PUBLICO;
		manda_entry_symbol (simbolo -> nome, arq1, 0);
		manda_entry_symbol (simbolo -> nome, arq2, 1);
		}
	volta (atomo);
	}

/*****************************************************************************
	data_size ()

	Espera chegada de tamanho de area de dados.

*****************************************************************************/

void data_size (int arq1, int arq2)
	{
	if (analex () != DEFINE_DATA_SIZE || rel != 'A')
		{
		erro ();
		termina (1);
		}
	modulo [mod_cont - 1].dsize = valor;
	manda_data_size (valor, arq1, 0);
	manda_data_size (valor, arq2, 1);
	}

/*****************************************************************************
	program_size ()

	Espera chegada de tamanho de area de programa.

*****************************************************************************/

void program_size (int arq1, int arq2)
	{
	if (analex () != DEFINE_PROGRAM_SIZE || rel != 'P')
		{
		erro ();
		termina (1);
		}
	modulo [mod_cont - 1].csize = valor;
	manda_program_size (valor, arq1, 0);
	manda_program_size (valor, arq2, 1);
	}

/*****************************************************************************
	programa ()

	Fica em loop esperando final de programa, que e dado pela chegada de um
DEFINE_ENTRY_POINT.	

*****************************************************************************/

void programa (int arq1, int arq2)
	{
	int atomo;

	while (1)
		switch (atomo = analex ())
			{
		case CHAIN_EXTERNAL:
		case DEFINE_ENTRY_POINT:
		case END_FILE:
		case END_MODULE:
			volta (atomo);
			return;

		case DEFINE_DATA_SIZE:
		case DEFINE_PROGRAM_SIZE:
		case ENTRY_SYMBOL:
		case ERRO:
		case PROGRAM_NAME:
			erro ();
			termina (1);
			break;

		default:
			manda_programa (atomo, arq1, 0);
			manda_programa (atomo, arq2, 1);
			}
	}

/*****************************************************************************
	define_entry_point ()

	Pega os DEFINE_ENTRY_POINTS.

*****************************************************************************/

void define_entry_point (int arq1, int arq2)
	{
	int atomo;

	while ((atomo = analex ()) == DEFINE_ENTRY_POINT)
		{
		if (!ignora_simbolo)
			if (!(simbolo -> atrib & PUBLICO))
				mprintf ("WARNING: public symbol inconsistency detected in module %s.\n", modulo [mod_cont - 1].nome);
		manda_entry_point (simbolo -> nome, arq1, 0);
		manda_entry_point (simbolo -> nome, arq2, 1);
		}
	volta (atomo);
	}

/*****************************************************************************
	chain_external ()

	Pega os CHAIN_EXTERNAL.

*****************************************************************************/

void chain_external (int arq1, int arq2)
	{
	int atomo;

	while ((atomo = analex ()) == CHAIN_EXTERNAL)
		{
		manda_chain_external (simbolo -> nome, arq1, 0);
		manda_chain_external (simbolo -> nome, arq2, 1);
		}
	volta (atomo);
	}

/*****************************************************************************
	end_module ()

	Espera chegada de END_MODULE.

*****************************************************************************/

void end_module (int arq1, int arq2)
	{
	if (analex () != END_MODULE || rel != 'A' || valor != 0)
		{
		erro ();
		termina (1);
		}
	manda_end_module (arq1, 0);
	manda_end_module (arq2, 1);
	}

/*****************************************************************************
	inicia ()

	Abre arquivo e inicia variaveis de leitura e analisador lexico.

*****************************************************************************/

int inicia (char *file)
	{
	if ((lib_file = open (file, O_BINARY | O_RDONLY)) == -1)
		{
		mprintf ("Failed to open file %s\n", str_maiuscula (file));
		return 0;
		}
	voltou = 0;
	falta = 0;
	return 1;
	}

/*****************************************************************************
	nomeok ()

	Coloca extensao apropriada no nome passado e volta com novo nome.

*****************************************************************************/

char *nomeok (char *n, char *ext)
	{
	int i, j;
	static char nomeout [128];

	if ((i = (int)strlen (n)) >= sizeof nomeout)
		return NULL;
	strcpy (nomeout, n);
	for (j = i; n [j] != ':' && n [j] != '\\' && n [j] != '.' && j; j--);
	if (strlen (ext) + i + 2 > sizeof nomeout)
		return NULL;
	if (n [j] != '.')
		{
		nomeout [i++] = '.';
		while (*ext != '\0')
			nomeout [i++] = *(ext++);
		}
	nomeout [i] = '\0';
	return nomeout;
	}

/*****************************************************************************
	volta ()

	Devolve atomo.

*****************************************************************************/

void volta (int atomo)
	{
	devolvido = atomo;
	voltou = 1;
	}

/*****************************************************************************
	salva_analex ()

	Salva estado do analisador lexico.

*****************************************************************************/

void salva_analex (void)
	{
	int i;

	svoltou = voltou;
	sdevolvido = devolvido;
	sfalta = falta;
	smask = mask;
	sleit = leit;
	slib_file = lib_file;
	for (i =0; i < sizeof bl; i++)
		sbl [i] = bl [i];
	}

/*****************************************************************************
	recupera_analex ()

	Recupera estado do analisador lexico.

*****************************************************************************/

void recupera_analex (void)
	{
	int i;

	voltou = svoltou;
	devolvido = sdevolvido;
	falta = sfalta;
	mask = smask;
	leit = sleit;
	lib_file = slib_file;
	for (i =0; i < sizeof bl; i++)
		bl [i] = sbl [i];
	}

/*****************************************************************************
	analex ()

	Decodifica arquivo de entrada.

*****************************************************************************/

int analex (void)
	{
	if (voltou)
		{
		voltou = 0;
		return devolvido;
		}

	if (!pega_bit ())
		{
		byte = pega_8bits ();
		return BYTE;
		}

	switch (pega_2bits ())
		{
	case 0:
		switch (pega_4bits ())
			{
		case 0:
			pega_nome ();
			return ENTRY_SYMBOL;

		case 1:
		case 3:
		case 4:
		case 5:
		case 8:
		case 12:
			return ERRO;

		case 2:
			pega_nome ();
			return PROGRAM_NAME;

		case 6:
			pega_numero ();
			pega_nome ();
			return CHAIN_EXTERNAL;

		case 7:
			pega_numero ();
			pega_nome ();
			return DEFINE_ENTRY_POINT;

		case 9:
			pega_numero ();
			return EXTERNAL_PLUS_OFFSET;

		case 10:
			pega_numero ();
			return DEFINE_DATA_SIZE;

		case 11:
			pega_numero ();
			return SET_LOCATION_COUNTER;

		case 13:
			pega_numero ();
			return DEFINE_PROGRAM_SIZE;

		case 14:
			pega_numero ();
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
		pega_valor ();
		return PROGRAM_RELATIVE;

	case 2:
		pega_valor ();
		return DATA_RELATIVE;

	case 3:
		return ERRO;
		}
	return ERRO;			// Just to make the compiler happy.
	}

/*****************************************************************************
	pega_nome ()

	Devolve nome lido do arquivo de entrada.

*****************************************************************************/

void pega_nome (void)
	{
	int i, j;
	char atm_name [comp_max + 1];

	for (j = 0, i = pega_3bits (); i; i--, j++)
		atm_name [j] = (char) pega_8bits ();
	atm_name [j] = '\0';
	ver_nome (atm_name, j);		/* coloca simbolo na tabela de simbolos */
	}

/*****************************************************************************
	ver_nome ():

	Procura ou coloca nome na tabela de simbolos.

*****************************************************************************/

void ver_nome (char *s, int comp)
	{
	int i, ent;
	simb *smb;

	for (ent = i = 0; i < comp; i++)
		ent = hash (ent, s[i]);

	if (inic_simbolo [ent &= (inic_simb_size - 1)] != NULL)
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
			simbolo = smb -> next_simbolo = get_simb ();
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
			erro_fatal ("TABELA DE SIMBOLOS CHEIA");
		if ((aloc_simb [nset_simb] = (simb *) malloc ((sizeof (simb)) * nsimb_aloc)) == NULL)
			erro_fatal ("MEMORIA INSUFICIENTE");
		resta_simb = nsimb_aloc;
		nset_simb++;
		}
	smb = aloc_simb [nset_simb - 1] + nsimb_aloc - resta_simb--;
	smb -> atrib = 0;
	smb -> next_simbolo = NULL;
	return smb;
	}

/*****************************************************************************
	pega_numero ()

	Devolve valor do numero de 16 bits, indicando a que modulo e' relativo.

*****************************************************************************/

void pega_numero (void)
	{
	switch (pega_2bits ())
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
		erro ();
		termina (1);
		}

	pega_valor ();
	}

/*****************************************************************************
	pega_valor ()

	Devolve valor do numero de 16 bits.

*****************************************************************************/

void pega_valor (void)
	{
	int i;

	i = pega_8bits ();
	valor = i + 0x100 * pega_8bits ();
	}

/*****************************************************************************
	pega_8bits ()

	Devolve valor dos proximos 8 bits do arquivo de entrada.

*****************************************************************************/

unsigned int pega_8bits (void)
	{
	int i;

	i = 16 * pega_4bits ();
	return i + pega_4bits ();
	}

/*****************************************************************************
	pega_4bits ()

	Devolve valor dos proximos 4 bits do arquivo de entrada.

*****************************************************************************/

int pega_4bits (void)
	{
	int i;

	i = 4 * pega_2bits ();
	return i + pega_2bits ();
	}

/*****************************************************************************
	pega_3bits ()

	Devolve valor dos proximos 3 bits do arquivo de entrada.

*****************************************************************************/

int pega_3bits (void)
	{
	int i;

	i = 2 * pega_2bits ();
	return i + pega_bit ();
	}

/*****************************************************************************
	pega_2bits ()

	Devolve valor dos proximos 2 bits do arquivo de entrada.

*****************************************************************************/

int pega_2bits (void)
	{
	int i;

	i = 2 * pega_bit ();
	return i + pega_bit ();
	}

/*****************************************************************************
	pega_bit ()

	Le bit do arquivo de entrada.

*****************************************************************************/

int pega_bit (void)
	{
	int retorno;

	if (falta == 0)
		{
		falta = read (lib_file, bl, sizeof bl);
		mask = 0x80;
		leit = 0;
		if (!falta)
			{
			erro ();
			termina (1);
			}
		}
	retorno = bl [leit] & mask;
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
	erro_fatal ()

	Informa mensagem de erro fatal.

*****************************************************************************/

void erro_fatal (char *s)
	{
	mprintf ("FATAL ERROR: %s\n", s);
	termina (1);
	}

/*****************************************************************************
	erro ()

	Informa mensagem de erro.

*****************************************************************************/

void erro (void)
	{
	mprintf ("Invalid file.\n");
	}

/*****************************************************************************
	str_maiuscula ()

	Converte string para maiusculo.

*****************************************************************************/

char *str_maiuscula (char *s)
	{
	char *p;

	for (p = s; *p != '\0'; p++)
		*p = toupper (*p);
	return s;
	}

/*****************************************************************************
	termina ()

	Finaliza execucao.

*****************************************************************************/

void termina (int cod)
	{
	int i;

	for (i = 0; i < nset_simb; i++)	/* desaloca memoria da tabela de simbolos */
		free ((void *) aloc_simb [i]);
	exit (cod);
	}

/*****************************************************************************
	manda_inf ()

	Escreve informacoes sobre arquivos.

*****************************************************************************/

void manda_inf (void)
	{
	int par = 0;
	int brancos;
	int i, j;
	simb *s;
	int sn;

	if (nset_simb)					/* so faz alguma coisa se existe simbolo */
		{
		ordena_tab_sym ();			/* ordena tabela de simbolos */
		for (sn = 0; sn < ((nset_simb - 1) << nrot_aloc) + (nsimb_aloc - resta_simb); sn++)
			if ((s = aloc_simb [(sn >> nrot_aloc) & c_mask_aloc] + (sn & mask_aloc)) -> atrib & PUBLICO)
				imprime_simbolo (s, par++);
		if (par & 1)
			putchar ('\n');
		for (i = 0; i < mod_cont; i++)
			{
			putchar ('\n');
			putchar ('\n');
			j = 18 - mprintf ("%s", modulo [i].nome);
			while (j--)
				putchar (' ');
			mprintf ("Code: %xH        Data: %xH", modulo [i].csize, modulo [i].dsize);
			for (brancos = 2, par = 0, sn = 0; sn < ((nset_simb - 1) << nrot_aloc) + (nsimb_aloc - resta_simb); sn++)
				if ((s = aloc_simb [(sn >> nrot_aloc) & c_mask_aloc] + (sn & mask_aloc)) -> atrib & PUBLICO && s -> modulo == i)
					{
					if (!(par & 0x3))
						putchar ('\n');
					while (brancos--)
						putchar (' ');
					brancos = 18 - mprintf ("%s", s -> nome);
					if (!(++par & 0x3))
						brancos = 2;
					}
			}
		}
	}

/*****************************************************************************
	imprime_simbolo ()

	Imprime simbolo.

*****************************************************************************/

void imprime_simbolo (simb *s, int par)
	{
	char buf [25];
	int nnome, i;

	nnome = (int)strlen (s -> nome);
	for (i = 0; i < nnome; i++)
		buf [i] = s -> nome [i];
	while (i < sizeof buf)
		buf [i++] = '.';
	strcpy (buf + (sizeof buf) - 1 - strlen (modulo [s -> modulo].nome), modulo [s -> modulo].nome);
	if (par & 1)
		mprintf ("            ");
	mprintf ("%s", buf);
	if (par & 1)
		putchar ('\n');
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
	unsigned int tempmodulo;
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
					tempmodulo = s1 -> modulo;
					tempatrib = s1 -> atrib;

					strcpy (s1 -> nome, s2 -> nome);
					s1 -> modulo = s2 -> modulo;
					s1 -> atrib = s2 -> atrib;

					strcpy (s2 -> nome, tempnome);
					s2 -> modulo = tempmodulo;
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
	mprintf ()

	Emula printf, so que muito menor.

*****************************************************************************/

int cdecl mprintf (char *s, ...)
	{
	int escape, pos_numero;
	char *aux, numero [(sizeof (long)) * 8 + 1];
	va_list arg;
	int nimp = 0;

	va_start (arg, s);		/* inicializa ponteiro p/ argumentos */
	for (escape = 0; *s != '\0'; s++)
		{
		switch (*s)
			{
		case '\\':
			if (!(escape = !escape))
				putchar (*s);
			nimp++;
			break;

		case '%':
			if (escape)
				{
				putchar (*s);
				escape = 0;
				nimp++;
				}
			else
				switch (*(++s))
					{
				case 'u':
				case 'i':
					aux = ultoa (((unsigned long) va_arg (arg, int)) & ((1LL << (8 * (sizeof (int)))) - 1), numero, 10);
					pos_numero = 0;
					while (*aux != '\0')
						{
						if (pos_numero || *aux != '0')
							{
							pos_numero = 1;
							putchar (*aux);
							nimp++;
							}
						aux++;
						}
					if (!pos_numero)
						{
						putchar ('0');
						nimp++;
						}
					break;

				case 'x':
					aux = ultoa (((unsigned long) va_arg (arg, int)) & ((1LL << (8 * (sizeof (int)))) - 1), numero, 16);
					pos_numero = 4 - (int)strlen (aux);
					while (pos_numero--)
						{
						putchar ('0');
						nimp++;
						}
					while (*aux != '\0')
						{
						putchar (*(aux++));
						nimp++;
						}
					break;

				case 'c':
					putchar (va_arg (arg, int));
					nimp++;
					break;

				case '\0':
				case '%':
					putchar ('%');
					nimp++;
					break;

				case 's':
					aux = va_arg (arg, char *);
					while (*aux != '\0')
						{
						putchar (*(aux++));
						nimp++;
						}
					break;

				default:
					putchar ('%');
					putchar (*s);
					nimp += 2;
					}
			break;

		default:
			if (!escape)
				{
				putchar (*s);
				nimp++;
				}
			else
				{
				escape = 0;
				nimp++;
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
	return nimp;
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

