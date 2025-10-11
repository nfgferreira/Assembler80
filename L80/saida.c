#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
//#include <sys\types.h>
#include <sys/stat.h>
//#include <io.h>
#include <setjmp.h>
#include <string.h>
#include "l80.h"

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
#define snome(i) ((aloc_simb [(i >> nrot_aloc) & c_mask_aloc] + (i & mask_aloc)) -> nome)

extern jmp_buf erro_tratamento;						/* ponteiro para erro de tratamento durante analise sintatica/semantica */
extern jmp_buf erro_tratamento2;						/* ponteiro para erro de tratamento apos analise sintatica/semantica */

char buf_sym [2048];			/* buffer para arquivo de simbolos */
int simb_linha, simb_col, sym_saida;		/* variaveis p/ construcao de arquivo de simbolos */
int arq_sym;					/* arquivo de simbolos */
int arq_rel;					/* arquivo relocavel */
char saida [1024 * 4];		/* buffer de saida */
int csaida;						/* ponteiro do buffer */
unsigned int masksaida;		/* mascara do bit atual de saida */
char nome_rel [comp_max + 1];			/* nome do arquivo de saida */

/*****************************************************************************
	cria_sym ():

	Cria arquivo com listagem de simbolos.

*****************************************************************************/

void cria_sym (char *n)
	{
	int sn;
	simb *s;

	simb_linha = simb_col = sym_saida = 0;

	if ((arq_sym = open (n, O_TRUNC | O_CREAT | O_TEXT | O_WRONLY, S_IWRITE)) == -1)
		longjmp (erro_tratamento2, 7);

	if (nset_simb)					/* so faz alguma coisa se existe simbolo */
		{
		ordena_tab_sym ();			/* ordena tabela de simbolos */
		for (sn = 0; sn < ((nset_simb - 1) << nrot_aloc) + (nsimb_aloc - resta_simb); sn++)
			if ((s = aloc_simb [(sn >> nrot_aloc) & c_mask_aloc] + (sn & mask_aloc)) -> definido)
				imprime_simbolo (s);
		if (write (arq_sym, buf_sym, sym_saida) != sym_saida)
			longjmp (erro_tratamento2, 6);
		}

	close (arq_sym);
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
	char tempdefinido, tempatrib;
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
					tempdefinido = s1 -> definido;
					tempatrib = s1 -> atrib;

					strcpy (s1 -> nome, s2 -> nome);
					s1 -> valor = s2 -> valor;
					s1 -> definido = s2 -> definido;
					s1 -> atrib = s2 -> atrib;

					strcpy (s2 -> nome, tempnome);
					s2 -> valor = tempvalor;
					s2 -> definido = tempdefinido;
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
	unsigned int v;

	while (simb_col & 0xf)
		manda_car_sym (' ');

	v = s -> valor;
	if (s -> atrib == 'P')
		v += areac;
	else if (s -> atrib == 'D')
		v += aread;

	num0 = num = ultoa ((unsigned long) v, n, 16);
	while (*(++num0) != '\0');
	for (i = 4 - (int)(num0 - num); i; i--)
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
		if (write (arq_sym, buf_sym, sizeof (buf_sym)) != sizeof (buf_sym))
			longjmp (erro_tratamento2, 6);
		sym_saida = 0;
		}
	simb_col++;
	}

/*****************************************************************************
	cria_rel ():

	Cria arquivo de saida para ser linkado com outros bancos.

*****************************************************************************/

void cria_rel (char *n)
	{
	if ((arq_rel = open (n, O_TRUNC | O_CREAT | O_BINARY | O_WRONLY, S_IWRITE)) == -1)
		longjmp (erro_tratamento2, 8);

	csaida = 0;
	masksaida = 0x80;		/* inicia variaveis de escrita em disco */

	define_nome (n);		/* coloca nome em nome_rel */
	manda_nome ();
	manda_entry_symbols ();
	manda_comprimentos ();
	manda_publics ();
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
		if (write (arq_rel, saida, csaida) != csaida)
			longjmp (erro_tratamento2, 6);
	close (arq_rel);
	}

/*****************************************************************************
	define_nome ()

	Volta com nome a ser escrito no arquivo rel de saida em nome_rel

*****************************************************************************/

void define_nome (char n[])
	{
	int i, j;

	for (i = 0; n [i] != '\0'; i++);
	while (1)
		{
		for (j = i - 1; j && n [j] != '.' && n [j] != '\\'; j--);
		if (!j || n [j] == '\\')
			{
			if (n [j] == '\\')
				j++;
			if (i >= (sizeof nome_rel) + j)
				{
				for (i = 0; i < (sizeof nome_rel) - 1; i++)
					{
					nome_rel [i] = n [j++];
					if (nome_rel [i] >= 'a' && nome_rel [i] <= 'z')
						nome_rel [i] += 'A' - 'a';
					}
				nome_rel [(sizeof nome_rel) - 1] = '\0';
				}
			else
				{
				for (i = 0; n [j] != '\0' && n [j] != '.'; i++, j++)
					{
					nome_rel [i] = n [j];
					if (nome_rel [i] >= 'a' && nome_rel [i] <= 'z')
						nome_rel [i] += 'A' - 'a';
					}
				nome_rel [i] = '\0';
				}
			return;
			}
		i = j;
		}
	}

/*****************************************************************************
	manda_nome ()

	Manda o nome do modulo para o arquivo rel de saida.

*****************************************************************************/

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

/*****************************************************************************
	manda_entry_symbols ()

	Manda nomes dos simbolos declarados no modulo.

*****************************************************************************/

void manda_entry_symbols (void)
	{
	int i, j;
	simb *smb;

	for (i = 0; i < inic_simb_size; i++)
		for (smb = inic_simbolo [i]; smb != NULL; smb = smb -> next_simbolo)
			if (smb -> definido)			/* garantia */
				{
				manda_bit (1);
				manda (special_link, 2);
				manda (entry_symbol, 4);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++);
				manda (j, 3);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++)
					manda ((smb -> nome) [j], 8);
				}
	}

/*****************************************************************************
	manda_comprimentos ()

	Manda comprimentos de dados e codigo para arquivo rel de saida.

*****************************************************************************/

void manda_comprimentos (void)
	{
	manda_bit (1);
	manda (special_link, 2);
	manda (define_data_size, 4);
	manda (absolute, 2);
	manda (0, 8);
	manda (0, 8);
	manda_bit (1);
	manda (special_link, 2);
	manda (define_program_size, 4);
	manda (program_relative, 2);
	manda (0, 8);
	manda (0, 8);
	}

/*****************************************************************************
	manda_publics (void)

	Manda definicoes de simbolos para arquivo rel de saida.

*****************************************************************************/

void manda_publics (void)
	{
	int i, j;
	simb *smb;
	unsigned int v;

	for (i = 0; i < inic_simb_size; i++)
		for (smb = inic_simbolo [i]; smb != NULL; smb = smb -> next_simbolo)
			if (smb -> definido)				/* garantia */
				{
				manda_bit (1);
				manda (special_link, 2);
				manda (define_entry_point, 4);
				manda (absolute, 2);

				v = smb -> valor;
				if (smb -> atrib == 'P')
					v += areac;
				else if (smb -> atrib == 'D')
					v += aread;

				manda (v, 8);
				manda (v >> 8, 8);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++);
				manda (j, 3);
				for (j = 0; j < 7 && (smb -> nome) [j] != '\0'; j++)
					manda ((smb -> nome) [j], 8);
				}
	}

/*****************************************************************************
	manda_bit ()

	Manda um bit para o arquivo rel de saida.

*****************************************************************************/

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
			if (write (arq_rel, saida, sizeof saida) != sizeof saida)
				longjmp (erro_tratamento2, 6);
			csaida = 0;
			}
		}
	else
		masksaida >>= 1;
	}

/*****************************************************************************
	manda ()

	Manda n bits para o arquivo rel de saida.

*****************************************************************************/

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

char *ultoa(unsigned long value, char *buffer, int radix) {
    // Handle invalid radix
    if (radix < 2 || radix > 36) {
        // You might choose to return NULL or handle this error differently
        return NULL; 
    }

    char *ptr = buffer;
    char *low = buffer;

    // Handle the case of value being 0
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return buffer;
    }

    // Convert digits in reverse order
    while (value > 0) {
        int digit = value % radix;
        *ptr++ = (digit > 9) ? (digit - 10 + 'a') : (digit + '0');
        value /= radix;
    }

    *ptr = '\0'; // Null-terminate the string

    // Reverse the string
    // This is a common way to reverse a string in-place
    char *high = ptr - 1;
    while (low < high) {
        char temp = *low;
        *low = *high;
        *high = temp;
        low++;
        high--;
    }

    return buffer;
}
