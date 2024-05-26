#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <io.h>
#include <dos.h>
#include <setjmp.h>
#include <string.h>
#include "variaveis.h"
#include "protos.h"
#include <sys\stat.h>

void cdecl main (int argc, char *argv[])
	{
	int i, npar;
	char *env, caux;
	char *mensagem = "A80: Montador 8080/8085/Z80/Z180 - Versao 2.01\n";

	while (*mensagem != '\0')		/* imprime mensagem */
		putc (*(mensagem++), stderr);

	if (argc == 1)
		{
		explica ();
		exit (0);
		}

	if ((env = getenv ("A80FLAGS")) == NULL)		/* nao tem environment */
		a80 (argc, argv);
	else
		{
		if (strlen (env) >= sizeof argv_aux)
			{
			mprintf ("Variavel de ambiente muito comprida:\n%s\n", env);
			exit (1);
			}
		strcpy (argv_aux, env);
		i = 0;				/* indice para argv_aux */
		npar = 1;			/* contador de parametros */
		__argv [0] = argv [0];
		while (argv_aux [i] != '\0')
			{
			while (argv_aux [i] == ' ' || argv_aux [i] == '\t')
				i++;
			if (argv_aux [i] != '\0')
				{
				if (npar >= sizeof __argv / sizeof (char *))
					{
					mprintf ("Ultrapassado o limite de parametros na chamada.\n");
					exit (1);
					}
				__argv [npar++] = argv_aux + i;	/* aponta para parametro */
				while (argv_aux [i] != ' ' && argv_aux [i] != '\t' && argv_aux [i] != '\0')
					i++;
				caux = argv_aux [i];
				argv_aux [i] = '\0';
				if (caux != '\0')
					i++;
				}
			}
		for (i = 1; i < argc; i++)
			if (npar >= sizeof __argv / sizeof (char *))
				{
				mprintf ("Ultrapassado o limite de parametros na chamada.\n");
				exit (1);
				}
			else
				__argv [npar++] = argv [i];
		a80 (npar, __argv);
		}
	}

void a80 (int argc, char *argv[])
	{
	int i, retorno;

/* acerta defaults */
	retorno = usa_pch = monta_pch = faz_simbolo = 0;
	cpu = i8080;
	definiu_sym = definiu_saida = 0;
	inc_path = NULL;

	for (i = 1; i < argc; i++)
		if (*argv [i] == '-' || *argv [i] == '/')

			if ((*(argv [i] + 1) == 'R' || *(argv [i] + 1) == 'r'))
				if (*(argv [i] + 2) != '\0')
					{
					if (strlen (argv [i] + 2) >= sizeof (nome_saida))
						{
						mprintf ("Nome muito comprido: %s\n", argv [i] + 2);
						exit (1);
						}
					strcpy (nome_saida, argv [i] + 2);
					definiu_saida = 1;
					}
				else
					definiu_saida = 0;
			else if ((*(argv [i] + 1) == 'I' || *(argv [i] + 1) == 'i'))
				inc_path = argv [i] + 2;
			else if ((*(argv [i] + 1) == 'H' || *(argv [i] + 1) == 'h'))
				usa_pch = 0;
			else if ((*(argv [i] + 1) == 'P' || *(argv [i] + 1) == 'p'))
				monta_pch = 0;
			else if ((*(argv [i] + 1) == 'S' || *(argv [i] + 1) == 's') && *(argv [i] + 2) == '\0')
				faz_simbolo = 0;
			else if (!strcmp (argv [i] + 1, "8080"))
				cpu = i8080;
			else if (!strcmp (argv [i] + 1, "8085"))
				cpu = i8085;
			else if (!stricmp (argv [i] + 1, "Z80"))
				cpu = z80;
			else if (!stricmp (argv [i] + 1, "Z180"))
				cpu = z180;
			else
				{
				mprintf ("Parametro invalido: %s\n", argv [i]);
				exit (1);
				}
		else if (*argv [i] == '+')
			if (*(argv [i] + 1) == 's' || *(argv [i] + 1) == 'S')
				{
				faz_simbolo = 1;
				if (*(argv [i] + 2) != '\0')
					{
					if (strlen (argv [i] + 2) >= sizeof (nome_sym))
						{
						mprintf ("Nome muito comprido: %s\n", argv [i] + 2);
						exit (1);
						}
					strcpy (nome_sym, argv [i] + 2);
					definiu_sym = 1;
					}
				else
					definiu_sym = 0;
				}
			else if ((*(argv [i] + 1) == 'H' || *(argv [i] + 1) == 'h'))
				usa_pch = 1;
			else if ((*(argv [i] + 1) == 'P' || *(argv [i] + 1) == 'p'))
				monta_pch = 1;
			else
				{
				mprintf ("Parametro invalido: %s\n", argv [i]);
				exit (1);
				}
		else
		{
			struct _finddata_t c_file;
   			intptr_t hFile;

			if ((hFile = _findfirst (argv [i], &c_file)) == -1)
				{
				mprintf ("Nao foi encontrado arquivo %s\n", argv [i]);
				retorno = 1;
				}
			else
			{
				do
					{
					if (monta_nome (argv [i], c_file.name))
						{
						mprintf ("%s\n", c_file.name);
						switch (monta_destinos (nmarq))
							{
						case 0:									/* ok */
							if (compila (nmarq))				/* erro de compilacao */
								{
								remove (nome_arq_rel);		/* apaga .rel */
								if (faz_simbolo)
									remove (nome_arq_sym);	/* apaga .sym, se existe */
								retorno = 1;
								}
							break;

						case 1:									/* pau no arquivo de simbolos */
							mprintf ("Nome de arquivo de simbolos muito comprido. Programa nao foi montado.\n");
							retorno = 1;
							break;

						case 2:
							mprintf ("Nome de arquivo destino muito comprido. Programa nao foi montado.\n");
							retorno = 1;
							break;
							}
						}
					else
						{
						mprintf ("Nome muito comprido: %s\n", argv [i]);
						exit (1);
						}
					}
				while (_findnext (hFile, &c_file) == 0);
				_findclose( hFile );
			}
		}
	exit (retorno);
	}

int monta_nome (char *s1, char *s2)
	{
	int i;
	char *aux;

	aux = s1;
	while (*aux != '\0')
		aux++;
	while (aux != s1 && *aux != '\\' && *aux != ':')
		aux--;
	if (*aux == '\\' || *aux == ':')
		aux++;
	i = 0;
	while (s1 != aux)
		if (i >= sizeof (nmarq))
			return 0;
		else
			nmarq [i++] = *(s1++);
	do
		if (i >= sizeof (nmarq))
			return 0;
		else
			nmarq [i++] = *(s2++);
	while (*s2 != '\0');

	if (i >= sizeof (nmarq))
		return 0;
	nmarq [i] = '\0';

	return 1;
	}

/*****************************************************************************
	explica ()

	Imprime mensagem com basico sobre o comando.

*****************************************************************************/

void explica (void)
	{
	mprintf ("\nForma de chamada:\n");
	mprintf ("a80 {[opcoes] arquivo_origem}\n\n");
	mprintf (" Opcoes:    -I[path{;path}] determina opcoes para busca de arquivos\n");
	mprintf ("            -R[path]        define arquivo destino\n");
	mprintf ("            -S              nao cria arquivo com tabela de simbolos\n");
	mprintf ("            +S[path]        cria arquivo com tabela de simbolos\n");
	mprintf ("            -H              nao usa cabecalho pre compilado\n");
	mprintf ("            +H              usa cabecalho pre compilado\n");
	mprintf ("            -P              nao monta cabecalho pre compilado\n");
	mprintf ("            +P              monta cabecalho pre compilado\n");
	mprintf ("            -8080\n");
	mprintf ("            -8085\n");
	mprintf ("            -Z80\n");
	mprintf ("            -Z180           seleciona micro processador\n\n");
	mprintf (" Defaults:  -8080 -H -P -S\n");
	}

/*****************************************************************************
	monta_destinos ()

	Constroi nomes de arquivos destinos de simbolos e .rel

*****************************************************************************/

int monta_destinos (char *nome)
	{
	char *caux;

	if (faz_simbolo)
		{
		caux = combina (definiu_sym ? nome_sym : NULL, nome, "SYM");
		if (caux == NULL || strlen (caux) >= sizeof nome_arq_sym)
			return 1;
		strcpy (nome_arq_sym, caux);
		}

	caux = combina (definiu_saida ? nome_saida : NULL, nome, monta_pch ? "PCH" : "REL");
	if (caux == NULL || strlen (caux) >= sizeof nome_arq_rel)
		return 2;
	strcpy (nome_arq_rel, caux);

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

	struct stat   buffer;
	int isFile = stat(dest, &buffer) == 0;

	if (!isFile)
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

