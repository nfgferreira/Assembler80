#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <string.h>
#include "lb80.h"

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

char cpbuf [4096];		/* buffer usado para copiar uma rotina na outra */

/*****************************************************************************
	manda_nome ()

	Manda nome de modulo para arquivo de saida.

*****************************************************************************/

void manda_nome (char *nome, int arq, int saida)
	{
	manda_n (nome, program_name, arq, saida);
	}

/*****************************************************************************
	manda_entry_symbol ()

	Manda nome de entry symbol (simbolo publico) para arquivo de saida.

*****************************************************************************/

void manda_entry_symbol (char *nome, int arq, int saida)
	{
	manda_n (nome, entry_symbol, arq, saida);
	}

/*****************************************************************************
	manda_data_size ()

	Manda comprimento de area de dados.

*****************************************************************************/

void manda_data_size (unsigned int valor, int arq, int saida)
	{
	manda_at_numero (define_data_size, absolute, valor, arq, saida);
	}

/*****************************************************************************
	manda_program_size ()

	Manda comprimento de area de dados.

*****************************************************************************/

void manda_program_size (unsigned int valor, int arq, int saida)
	{
	manda_at_numero (define_program_size, program_relative, valor, arq, saida);
	}

/*****************************************************************************
	manda_programa ()

	Manda atomos de programas, quais sejam:
		BYTE
		EXTERNAL_PLUS_OFFSET
		SET_LOCATION_COUNTER
		PROGRAM_RELATIVE
		DATA_RELATIVE

*****************************************************************************/

void manda_programa (int atomo, int arq, int saida)
	{
	int aloc;

	switch (atomo)
		{
	case BYTE:
		manda (0, 1, arq, saida);
		manda (byte, 8, arq, saida);
		break;

	case EXTERNAL_PLUS_OFFSET:
	case SET_LOCATION_COUNTER:
		switch (rel)
			{
		case 'A':
			aloc = 0;
			break;

		case 'P':
			aloc = 1;
			break;

		case 'D':
			aloc = 2;
			break;
			}
		manda (1, 1, arq, saida);
		manda (special_link, 2, arq, saida);
		manda (atomo == EXTERNAL_PLUS_OFFSET ? external_plus_offset : set_location_counter, 4, arq, saida);
		manda (aloc, 2, arq, saida);
		manda (valor, 8, arq, saida);
		manda (valor >> 8, 8, arq, saida);
		break;

	case PROGRAM_RELATIVE:
	case DATA_RELATIVE:
		manda (1, 1, arq, saida);
		manda (atomo == PROGRAM_RELATIVE ? 1 : 2, 2, arq, saida);
		manda (valor, 8, arq, saida);
		manda (valor >> 8, 8, arq, saida);
		break;
		}
	}

/*****************************************************************************
	manda_chain_external ()

	Manda codigos de chain external.

*****************************************************************************/

void manda_chain_external (char *nome, int arq, int saida)
	{
	manda_n_n (nome, chain_external, arq, saida);
	}

/*****************************************************************************
	manda_entry_point ()

	Manda codigos de entry point.

*****************************************************************************/

void manda_entry_point (char *nome, int arq, int saida)
	{
	manda_n_n (nome, define_entry_point, arq, saida);
	}

/*****************************************************************************
	manda_end_module ()

	Manda codigo de fim de modulo.

*****************************************************************************/

void manda_end_module (int arq, int saida)
	{
	int *ms;

	manda_at_numero (end_module, absolute, 0, arq, saida);
	ms = (int *) (saida ? &masksaida1 : &masksaida0);
	if (arq != -1)
		while (*ms != 0x80)
			manda_bit (0, arq, saida);
	}

/*****************************************************************************
	manda ()

	Manda numero de bits especificados para a saida.

*****************************************************************************/

void manda (int num, int n, int arq, int saida)
	{
	unsigned int mask;

	mask = 1 << (n - 1);
	while (mask)
		{
		manda_bit (num & mask, arq, saida);
		mask >>= 1;
		}
	}

/*****************************************************************************
	manda_bit ()

	Manda um bit para a saida especificada.

*****************************************************************************/

void manda_bit (int bit, int arq, int t_saida)
	{
	char *saida;
	unsigned int *masksaida;
	int *csaida;

	if (arq == -1)
		return;

	saida = t_saida ? saida1 : saida0;
	masksaida = t_saida ? &masksaida1 : &masksaida0;
	csaida = t_saida ? &csaida1 : &csaida0;

	if (bit)
		saida [*csaida] |= *masksaida;
	else
		saida [*csaida] &= ~*masksaida;

	if (*masksaida == 1)
		{
		*masksaida = 0x80;
		if (++*csaida >= sizeof saida)
			{
			escreve (arq, saida, sizeof saida);
			*csaida = 0;
			}
		}
	else
		*masksaida >>= 1;
	}

/*****************************************************************************
	escreve ()

	Rotina semelhante ao write do C, so que confere se nao houve erro.

*****************************************************************************/

void escreve (int handle, char *buffer, unsigned int count)
	{
	if (write (handle, buffer, count) != count)
		erro_fatal ("disco cheio");
	}

/*****************************************************************************
	inicia_saida0 ()

	Inicia variaveis relativas a saida 0.

*****************************************************************************/

void inicia_saida0 (void)
	{
	int i;

	csaida0 = 0;
	masksaida0 = 0x80;
	}

/*****************************************************************************
	inicia_saida1 ()

	Inicia variaveis relativas a saida 1.

*****************************************************************************/

void inicia_saida1 (void)
	{
	int i;

	csaida1 = 0;
	masksaida1 = 0x80;
	}

/*****************************************************************************
	fecha_saida0 ()

	Fecha a saida 0, mandando bytes que faltam

*****************************************************************************/

void fecha_saida0 (int arq)
	{
	manda (0x9e, 8, arq, 0);
	if (csaida0)
		escreve (arq, saida0, csaida0);
	close (arq);
	}

/*****************************************************************************
	fecha_saida1 ()

	Fecha a saida 1, mandando bytes que faltam

*****************************************************************************/

void fecha_saida1 (int arq)
	{
	manda (0x9e, 8, arq, 1);
	if (csaida1)
		escreve (arq, saida1, csaida1);
	close (arq);
	}

/*****************************************************************************
	copy ()

	Copia um arquivo em outro.

*****************************************************************************/

void copy (char *dst, char *org)
	{
	int d, o, lidos;

	if ((o = open (org, O_BINARY | O_RDONLY)) == -1)
		return;

	if ((d = open (dst, O_TRUNC | O_CREAT | O_BINARY | O_WRONLY, S_IWRITE)) == -1)
		{
		close (o);
		mprintf ("FATAL ERROR: failed to open file %s\n", str_maiuscula (dst));
		return;
		}

	while (lidos = read (o, cpbuf, sizeof cpbuf))
		escreve (d, cpbuf, lidos);
	close (d);
	close (o);
	}

/*****************************************************************************
	manda_n_n ()

	Manda numero e nome.

*****************************************************************************/

void manda_n_n (char *nome, int cod, int arq, int saida)
	{
	int aloc, i;

	switch (rel)
		{
	case 'A':
		aloc = 0;
		break;

	case 'P':
		aloc = 1;
		break;

	case 'D':
		aloc = 2;
		break;
		}
	manda (1, 1, arq, saida);
	manda (special_link, 2, arq, saida);
	manda (cod, 4, arq, saida);
	manda (aloc, 2, arq, saida);
	manda (valor, 8, arq, saida);
	manda (valor >> 8, 8, arq, saida);
	manda (strlen (nome), 3, arq, saida);
	for (i = 0; nome [i] != '\0'; i++)
		manda (nome [i], 8, arq, saida);
	}

/*****************************************************************************
	manda_n ()

	Manda nome.

*****************************************************************************/

void manda_n (char *nome, int cod, int arq, int saida)
	{
	int i;

	manda_bit (1, arq, saida);
	manda (special_link, 2, arq, saida);
	manda (cod, 4, arq, saida);
	manda (strlen (nome), 3, arq, saida);
	for (i = 0; nome [i] != '\0'; i++)
		manda (nome [i], 8, arq, saida);
	}

/*****************************************************************************
	manda_at_numero ()

	Manda atomo com numero apenas.

*****************************************************************************/

void manda_at_numero (int cod, int aloc, unsigned int valor, int arq, int saida)
	{
	manda_bit (1, arq, saida);
	manda (special_link, 2, arq, saida);
	manda (cod, 4, arq, saida);
	manda (aloc, 2, arq, saida);
	manda (valor, 8, arq, saida);
	manda (valor >> 8, 8, arq, saida);
	}

