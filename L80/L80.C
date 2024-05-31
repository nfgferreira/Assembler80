#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <dos.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include "l80.h"

extern jmp_buf erro_tratamento;						/* ponteiro para erro de tratamento durante analise sintatica/semantica */
extern jmp_buf erro_tratamento2;						/* ponteiro para erro de tratamento apos analise sintatica/semantica */

//nfgf char *__argv [256];
char buf_prj [2048 + 1];
char nome_arq_rel [128];						/* nome do arquivo .rel de saida */
char nome_arq_sym [128];						/* nome do arquivo .sym de saida */

void cdecl main (int argc, char *argv [])
	{
	int arq, tam_arq, _argc, i, i_aux, tem_parametro;
	char *mensagem = "L80: Linker - Versao 2.00\n";

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
			mprintf ("The file is too big: %s\n", argv [1] + 1);
			exit (1);
			}
		close (arq);
		__argv [0] = argv [0];
		_argc = 1;
		for (i = tem_parametro = 0; i < tam_arq; i++)
			if (buf_prj [i] == ' ' || buf_prj [i] == '\t' || buf_prj [i] == '\n')
				{
				if (tem_parametro)
					{
					__argv [_argc++] = buf_prj + i_aux;
					buf_prj [i] = '\0';
					tem_parametro = 0;
					if (_argc >= (sizeof (__argv)) / (sizeof (char *)))
						{
						mprintf ("Too many paramters in %s\n", argv [1] + 1);
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
			__argv [_argc++] = buf_prj + i_aux;
			buf_prj [i] = '\0';
			if (_argc >= (sizeof (__argv)) / (sizeof (char *)))
				{
				mprintf ("Too many paramters in %s\n", argv [1] + 1);
				exit (1);
				}
			}
		faz_link (_argc, __argv);
		}
	else
		faz_link (argc, argv);
	}

/*****************************************************************************
	faz_link ()

	Faz link baseado nos parametros passados.

*****************************************************************************/

void faz_link (int argc, char *argv [])
	{
	int i, erro;
	char *rel = NULL,
		  *sym = NULL;				/* nomes de arquivos .rel e .sym */
	int destino;					/* arquivo destino da lincagem */
	int resultado = 0;
	int tem_nao_definido = 0;
	int tem_c = 0,
		 tem_d = 0;
	char *n_dest;					/* nome do arquivo destino */

	if (argc == 1)
		{
		explica ();
		exit (0);
		}

	for (i = 1; i < argc && (*argv [i] == '-' || *argv [i] == '/'); i++)
		switch (*(argv [i] + 1))
			{
		case 'c':
		case 'C':
			if (tem_c)
				{
				mprintf ("Invalid parameter: %s\n", argv [i]);
				exit (1);
				}
			if (pega_hex (argv[i] + 2, (int *) &areac))
				{
				mprintf ("Invalid parameter: %s\n", argv [i]);
				exit (1);
				}
			tem_c = 1;
			break;

		case 'd':
		case 'D':
			if (tem_d)
				{
				mprintf ("Invalid parameter: %s\n", argv [i]);
				exit (1);
				}
			if (pega_hex (argv[i] + 2, (int *) &aread))
				{
				mprintf ("Invalid parameter: %s\n", argv [i]);
				exit (1);
				}
			tem_d = 1;
			break;

		case 's':
		case 'S':
			if (sym != NULL)
				{
				mprintf ("Invalid parameter: %s\n", argv [i]);
				exit (1);
				}
			sym = argv [i] + 2;
			break;

		case 'r':
		case 'R':
			if (rel != NULL)
				{
				mprintf ("Invalid parameter: %s\n", argv [i]);
				exit (1);
				}
			rel = argv [i] + 2;
			break;

		default:
			mprintf ("Invalid parameter: %s\n", argv [i]);
			exit (1);
			}

	if (argc - i < 2)
		{
		mprintf ("Not enough parameters.\n");
		exit (1);
		}

	if ((*argv [i + 1] == '-' || *argv [i + 1] == '/') && (*(argv [i + 1] + 1) == 'l' || *(argv [i + 1] + 1) == 'L'))
		{
		mprintf ("First file cannot be a library.\n");
		exit (1);
		}

	if ((destino = open (n_dest = argv [i++], O_TRUNC | O_CREAT | O_BINARY | O_WRONLY, S_IWRITE)) == -1)
		{
		mprintf ("Could not open destination file: %s\n", argv [i - 1]);
		exit (1);
		}

	inicia_linc ();				/* faz inicializacoes devidas */

	switch (monta_destinos (n_dest, sym, rel))
		{
	case 0:
		break;						/* caso ok: compila */

	case 1:
		mprintf ("The symbol file name is too long. The program was not linked.\n");
		exit (1);

	case 2:
		mprintf ("The relocatable file name is too long. The program was not linked\n");
		exit (1);
		}

	do
		resultado = linca (argv [i++]);
	while (i < argc && !resultado);

	if (erro = setjmp (erro_tratamento2))
		{							/* ponto para retornar caso erro grave ainda nao detectado */
		switch (erro)
			{
		case 1:
			mprintf ("THERE IS AN INCONSISTENCY IN THE FILE");
			break;

		case 2:
			mprintf ("INTERNAL ERROR");
			break;

		case 3:
			mprintf ("CODE ADDRESS BEYOND 64K");
			break;

		case 4:
			mprintf ("DATA ADDRESS BEYOND 64K");
			break;

		case 5:
			mprintf ("CODE AND DATA ADDRESS CONFLICT");
			break;

		case 6:
			mprintf ("DISK WRITE ERROR");
			break;

		case 7:
			mprintf ("COULD NOT OPEN SYMBOL FILE");
			break;

		case 8:
			mprintf ("COULD NOT OPEN RELOCATABLE OUTPUT FILE");
			break;

		default:
			mprintf ("INTERNAL ERROR");
			}
		mprintf (".\nABORTING.\n");
		close (destino);
		apaga_arqs (n_dest, sym, rel);		/* destroi arquivos destinos */
		termina (1);
		}

	if (!resultado && !simbolo_redefinido)
		if (tem_nao_definido = varre_t_sym ())		/* trata chain_external e verifica se ha' simbolos nao definidos */
			trata_nao_definidos ();
		else
			{
			if (!tem_c)
				areac = 0;
			if (!tem_d)
				aread = areac + catual;
			if (areac + catual < areac)
				longjmp (erro_tratamento2, 3);	/* endereco de codigo ultrapassou 64K */
			if (aread + datual < aread)
				longjmp (erro_tratamento2, 4);	/* endereco de data ultrapassou 64K */
			if (aread >= areac)
				{
				if (areac + catual > aread)
					longjmp (erro_tratamento2, 5);	/* conflito de enderecos */
				}
			else
				{
				if (aread + datual > areac)
					longjmp (erro_tratamento2, 5);	/* conflito de enderecos */
				}
			acerta_enderecos (areac, aread);		/* coloca enderecos corretos */
			trata_offsets ();				/* coloca offsets */
			monta_arquivo (destino, areac, aread);
			if (rel != NULL)				/* tem que montar arquivo .rel */
				cria_rel (nome_arq_rel);
			if (sym != NULL)				/* tem que montar arquivo com simbolos */
				cria_sym (nome_arq_sym);

			if (catual)
				mprintf ("CODE: [%X-%X] (%X)\n", areac, areac + catual - 1, catual);
			if (datual)
				mprintf ("DATA:  [%X-%X] (%X)\n", aread, aread + datual - 1, datual);
			mprintf ("\nLINK FINISHED SUCCESSFULLY.\n");
			}
	close (destino);
	if (resultado || simbolo_redefinido || tem_nao_definido)
		{
		apaga_arqs (n_dest, sym, rel);		/* destroi arquivos destinos */
		termina (1);
		}
	else
		termina (0);
	}

/*****************************************************************************
	explica ()

	Imprime mensagem com basico sobre o comando.

*****************************************************************************/

void explica (void)
	{
	mprintf ("\nCall instructions:\n");
	mprintf ("l80 [options] dest_file source_file {[-L]source_file}\n\n");
	mprintf (" Options:   -Chhhh  define code area\n");
	mprintf ("            -Dhhhh  define data area\n");
	mprintf ("            -S[arq] create symbol table file\n");
	mprintf ("            -R[arq] create relocatable file of defined symbols\n\n");
	mprintf ("            -L      the file is a library\n\n");
	mprintf (" Defaults:  -C:     0000\n");
	mprintf ("            -D:     end of code area\n\n");
	mprintf (" Default extensions:   source_file:        .rel if it is not a library\n");
	mprintf ("                                           .lib if it is\n");
	mprintf ("                       symbol file:        .sym\n");
	mprintf ("                       relocatable file:   .rel\n");
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
					aux = ultoa (((unsigned long) va_arg (arg, int)) & ((1L << (8 * (sizeof (int)))) - 1), numero, 10);
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
					aux = ultoa (((unsigned long) va_arg (arg, int)) & ((1L << (8 * (sizeof (int)))) - 1), numero, 16);
					pos_numero = 4 - strlen (aux);
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

				case 'X':
					aux = ultoa (((unsigned long) va_arg (arg, int)) & ((1L << (8 * (sizeof (int)))) - 1), numero, 16);
					pos_numero = 4 - strlen (aux);
					while (pos_numero--)
						{
						putchar ('0');
						nimp++;
						}
					while (*aux != '\0')
						{
						putchar (*aux >= 'a' && *aux <= 'f' ? *aux - 'a' + 'A' : *aux);
						aux++;
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
	pega_hex ()

	Converte uma string de caracteres hexa-decimais para inteiro.

*****************************************************************************/

int pega_hex (char *p, int *n)
	{
	long num = 0L;

	do
		{
		num <<= 4;
		if (num >= 0x10000)
			return 2;						/* numero muito grande */
		if (*p >= '0' && *p <= '9')
			num += *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			num += *p - 'a' + 10;
		else if (*p >= 'A' && *p <= 'F')
			num += *p - 'A' + 10;
		else return 1;						/* caracter invalido */
		p++;
		}
	while (*p != '\0');

	*n = num;
	return 0;
	}

/*****************************************************************************
	inicia_linc ()

	Faz inicializacoes de variaveis globais.

*****************************************************************************/

void inicia_linc (void)
	{
	int i;

	for (i = 0; i < sizeof modo_end_c; i++)
		modo_end_c [i] = 0;
	for (i = 0; i < sizeof modo_end_d; i++)
		modo_end_d [i] = 0;	/* indica absoluto em todos os bytes */

	catual = datual = i_cseg = i_dseg = 0;		/* indica nenhum codigo guardado */
	i_os = 0;					/* indica que nao ha' off-set */
	resta_simb = 0;			/* indica que nao ha' espaco alocado para simbolos */
	nset_simb = 0;
	simbolo_redefinido = 0;	/* indica que ainda nao existe simbolo redefinido */
	limpa_ts ();				/* limpa tabela de simbolos */
	}

/*****************************************************************************
	linca ()

	Linca arquivo dado com estrutura.

*****************************************************************************/

int linca (char *arq)
	{
	int erro, atomo;
	int lib = 0;

	if ((*arq == '-' || *arq == '/') && (*(arq + 1) == 'l' || *(arq + 1) == 'L'))
		if (*(arq + 2) == '\0')
			{
			mprintf ("Invalid parameter: %s\nABORTING.\n", arq);
			return 1;
			}
		else
			{
			arq += 2;
			lib = 1;
			}

	if ((l_file = open (nome = nomeok (arq, lib ? "lib" : "rel"), O_BINARY | O_RDONLY)) == -1)
		{
		mprintf ("FAILE TO OPEN FILE %s\nABORTING.\n", str_maiuscula (nome));
		return 1;
		}

	inicia_analex ();			/* inicia variaveis de analisador lexico */

	if (erro = setjmp (erro_tratamento))
		{							/* ponto para retornar de erro grave de arquivo */
		switch (erro)
			{
		case 1:
			mprintf ("%s: INVALID FILE", str_maiuscula (nome));
			break;

		case 2:
			mprintf ("%s(%s): INVALID MODULE", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 3:
			mprintf ("%s(%s): SYMBOL TABLE OVERFLOW", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 4:
			mprintf ("%s(%s): INSUFFICIENT MEMORY", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 5:
			mprintf ("%s(%s): CODE IN ABSOLUTE ADDRESS", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 6:
			mprintf ("%s(%s): MORE THAN 64K OF CODE", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 7:
			mprintf ("%s(%s): MORE THAN 64K OF DATA", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 8:
			mprintf ("%s(%s): NUMBER OF EXTERNAL SYMBOL OFFSETS EXCEEDED", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 9:
			mprintf ("%s(%s): INTERNAL ERROR", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		case 10:
			mprintf ("%s(%s): CODE WENT BACK", str_maiuscula (nome), str_maiuscula (nome_modulo));
			break;

		default:
			mprintf ("%s(%s): INTERNAL ERROR", str_maiuscula (nome), str_maiuscula (nome_modulo));
			}

		mprintf (".\nABORTING.\n");
		close (l_file);
		return 1;
		}

	while (1)
		{
		trata_arq (lib);
		if ((atomo = analex ()) != END_FILE)
			volta (atomo);
		else
			{
			close (l_file);
			return 0;			/* arquivo linkado com sucesso */
			}
		}
	}

/*****************************************************************************
	nomeok ()

	Coloca extensao apropriada no nome passado e volta com novo nome.

*****************************************************************************/

char *nomeok (char *n, char *ext)
	{
	int i, j;
	static char nomeout [128];

	if ((i = strlen (n)) >= sizeof nomeout)
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
	poe_ext ()

	Coloca extensao apropriada no nome passado e volta com novo nome. A di-
ferenca para a rotina nomeok e' que a extensao e' forcada, i.e.: se o arquivo
tem alguma extensao, ele e' substituida.

*****************************************************************************/

char *poe_ext (char *n, char *ext)
	{
	int i, j;
	static char nomeout [128];

	if ((i = strlen (n)) >= sizeof nomeout)
		return NULL;
	strcpy (nomeout, n);
	for (j = i; n [j] != ':' && n [j] != '\\' && n [j] != '.' && j; j--);
	if (n [j] == '.')
		i = j;
	if (strlen (ext) + i + 2 > sizeof nomeout)
		return NULL;
	nomeout [i++] = '.';
	while (*ext != '\0')
		nomeout [i++] = *(ext++);
	nomeout [i] = '\0';
	return nomeout;
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
	int i, j;

	for (i = 0; i < nset_simb; i++)	/* desaloca memoria da tabela de simbolos */
		free ((void *) aloc_simb [i]);

	if (i_cseg)
		{
		j = (i_cseg >> part_bits);
		if (!(i_cseg & mask_part))
			j--;
		for (i = 0; i <= j; i++)
			free ((void *) cseg [i]);
		}

	if (i_dseg)
		{
		j = (i_dseg >> part_bits);
		if (!(i_dseg & mask_part))
			j--;
		for (i = 0; i <= j; i++)
			free ((void *) dseg [i]);
		}

	if (i_os)
		{
		j = (i_os >> os_bits);
		if (!(i_os & mask_prt_os))
			j--;
		for (i = 0; i <= j; i++)
			free ((void *) end_os [i]);
		}

	exit (cod);
	}

/*****************************************************************************
	trata_arq ()

	Faz tratamento de arquivo.

*****************************************************************************/

void trata_arq (int lib)
	{
	base_cseg = catual;
	base_dseg = datual;		/* acerta base de enderecos */
	rel_atual = 'P';			/* coloca codigo no endereco default: 0x0000 em programa (CSEG) */

	coloca_simbolo = 0;			/* manda nao colocar simbolos na tabela de simbolos */
	trata_program_name ();
	if (trata_entry_symbol (lib))
		{								/* linka o arquivo */
		coloca_simbolo = 1;			/* coloca simbolos na tabela */
		trata_define_data_size ();
		trata_define_program_size ();
		trata_programa ();			/* faz tratamento de programa ate' chegar um atomo nao reconhecivel */
		trata_define_entry_point ();
		trata_chain_external ();
		coloca_simbolo = 0;			/* manda nao colocar simbolos na tabela de simbolos */
		trata_end_module ();
		}
	else
		while (analex () != END_MODULE);	/* pula arquivo */
	}

/*****************************************************************************
	void trata_program_name ()

	Espera chegada de PROGRAM_NAME.

*****************************************************************************/

void trata_program_name (void)
	{
	if (analex () != PROGRAM_NAME)
		longjmp (erro_tratamento, 1);
	strcpy (nome_modulo, simbolo_analex);		/* guarda nome do modulo */
	}

/*****************************************************************************
	int trata_entry_symbol ()

	Pula ENTRY_SYMBOL's.

*****************************************************************************/

int trata_entry_symbol (int lib)
	{
	int atomo;
	int i, ent;
	simb *smb;

	while ((atomo = analex ()) == ENTRY_SYMBOL)
		if (lib)
			{			/* arquivo de biblioteca: veja se simbolo existe e nao definido */
			for (ent = i = 0; simbolo_analex [i] != '\0'; i++)
				ent = hash (ent, simbolo_analex [i]);

			if ((smb = inic_simbolo [ent &= (inic_simb_size - 1)]) != NULL)
				do
					if (!strcmp (simbolo_analex, smb -> nome))
						lib = smb -> definido;
				while (lib && ((smb = smb -> next_simbolo) != NULL));
			}

	volta (atomo);
	return !lib;
	}

/*****************************************************************************
	void trata_define_data_size ()

	Espera chegada de DEFINE_DATA_SIZE.

*****************************************************************************/

void trata_define_data_size (void)
	{
	if (analex () != DEFINE_DATA_SIZE || rel != 'A')
		longjmp (erro_tratamento, 2);
	}

/*****************************************************************************
	void trata_define_program_size ()

	Espera chegada de DEFINE_PROGRAM_SIZE.

*****************************************************************************/

void trata_define_program_size (void)
	{
	if (analex () != DEFINE_PROGRAM_SIZE || rel != 'P')
		longjmp (erro_tratamento, 2);
	}

/*****************************************************************************
	void trata_end_module ()

	Espera chegada de END_MODULE.

*****************************************************************************/

void trata_end_module (void)
	{
	if (analex () != END_MODULE)
		longjmp (erro_tratamento, 2);
	}

/*****************************************************************************
	monta_destinos ()

	Constroi nomes de arquivos destinos de simbolos e .rel

*****************************************************************************/

int monta_destinos (char *nome, char *sym, char *rel)
	{
	char *caux;

	if (sym != NULL)
		{
		caux = combina (*sym != '\0' ? sym : NULL, nome, "SYM");
		if (caux == NULL || strlen (caux) >= sizeof nome_arq_sym)
			return 1;
		strcpy (nome_arq_sym, caux);
		}

	if (rel != NULL)
		{
		caux = combina (*rel != '\0' ? rel : NULL, nome, "REL");
		if (caux == NULL || strlen (caux) >= sizeof nome_arq_rel)
			return 2;
		strcpy (nome_arq_rel, caux);
		}

	return 0;
	}

/*****************************************************************************
	combina ()

	Dadas strings com nome do arquivo original, do destino e da extensao,
volta com path destino.

*****************************************************************************/

char *combina (char *dest, char *org, char *ext)
	{
	static char nomeout [128];
	int aux;
	char *caux;

	org = tira_path (org);
	if (dest == NULL)
		return poe_ext (org, ext);

	aux =  strlen (dest) - 1;
	if (dest [aux] == ':' || dest [aux] == '\\')
		{								/*  caso acessando diretorio */
		if ((caux = poe_ext (org, ext)) == NULL)
			return NULL;
		if (aux + 1 + strlen (caux) + 1 > sizeof nomeout)
			return NULL;
		strcpy (nomeout, dest);
		return strcat (nomeout, caux);
		}

//nfgf	if ((aux = _chmod (dest, 0)) != -1 && aux & FA_DIREC)
	struct stat   buffer;
	if ((stat(dest, &buffer) == 0) && ((buffer.st_mode & S_IFMT) == S_IFDIR))
		{								/* caso acessando diretorio */
		if ((caux = poe_ext (org, ext)) == NULL)
			return NULL;
		if (strlen (dest) + strlen (caux) + 2 > sizeof nomeout)
			return NULL;
		strcpy (nomeout, dest);
		return strcat (strcat (nomeout, "\\"), caux);
		}

	return (nomeok (dest, ext));
	}

/*****************************************************************************
	tira_path ()

	Dado nome de arquivo, volta com nome do arquivo sem o path.

*****************************************************************************/

char *tira_path (char *org)
	{
	int i;

	i = strlen (org);
	while (i && org [i - 1] != '\\' & org [i - 1] != ':')
		i--;
	return org + i;
	}

/*****************************************************************************
	apaga_arqs ()

	Destroi arquivos montados, se e' que foram construidos.

*****************************************************************************/

void apaga_arqs (char *nome, char *sym, char *rel)
	{
	remove (nome);
	if (sym != NULL)
		remove (nome_arq_sym);
	if (rel != NULL)
		remove (nome_arq_rel);
	}

