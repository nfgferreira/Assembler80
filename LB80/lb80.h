/******************************* DEFINES ************************************/

#define hash(r, c) ((~(r) >> 1) ^ (((~(r) >> 4) & 0xf) + ((~(r) << 4) & 0xf0)) + ((c) << 2))
#define snome(i) ((aloc_simb[(i >> nrot_aloc) & c_mask_aloc] + (i & mask_aloc))->nome)
#define comp_max 8
#define inic_simb_size 256
#define max_mod 256
#define PUBLICO 1
#define MODULO 2
#define max_simb_aloc 32 /* numero maximo de unidades de tabela de simbolos alocaveis */
#define nrot_aloc 10
#define c_mask_aloc ((1 << ((sizeof(int)) * 8 - nrot_aloc)) - 1)
#define nsimb_aloc (1 << nrot_aloc) /* numero de simbolos alocados por vez */
#define mask_aloc (nsimb_aloc - 1)
#define O_BINARY 0
#define O_TEXT 0

typedef struct simbl
{
    char nome[comp_max + 1];
    unsigned int modulo;
    char atrib; /* atributos: PUBLIC, MODULO */
    struct simbl *next_simbolo;
} simb;

typedef enum
{
    BYTE,
    CHAIN_EXTERNAL,
    DATA_RELATIVE,
    DEFINE_DATA_SIZE,
    DEFINE_ENTRY_POINT,
    DEFINE_PROGRAM_SIZE,
    END_FILE,
    END_MODULE,
    ENTRY_SYMBOL,
    ERRO,
    EXTERNAL_PLUS_OFFSET,
    PROGRAM_NAME,
    PROGRAM_RELATIVE,
    SET_LOCATION_COUNTER
} atomos;

/***************************** PROTOTYPES ***********************************/

/* l80.c */
void faz_lib(int argc, char *argv[]);
void edita(int npar, char *par[]);
void tira_subst(char *arq, int arqout, char **com, int ncom);
void inclui(int arq, char **com, int ncom);
char *busca_i(char **com, int ncom);
int busca_t_s(char *nome, char **com, int ncom);
void parse_mod(int arq1, int arq2);
void explica(void);
void informa(char *lib);
void nome_mod(int arq1, int arq2);
void entry_symbols(int arq1, int arq2);
void data_size(int arq1, int arq2);
void program_size(int arq1, int arq2);
void programa(int arq1, int arq2);
void define_entry_point(int arq1, int arq2);
void chain_external(int arq1, int arq2);
void end_module(int arq1, int arq2);
int inicia(char *file);
char *nomeok(char *n, char *ext);
void volta(int atomo);
void salva_analex(void);
void recupera_analex(void);
int analex(void);
void pega_nome(void);
void ver_nome(char *s, int comp);
simb *get_simb(void);
void pega_numero(void);
void pega_valor(void);
unsigned int pega_8bits(void);
int pega_4bits(void);
int pega_3bits(void);
int pega_2bits(void);
int pega_bit(void);
void erro_fatal(char *s);
void erro(void);
char *str_maiuscula(char *s);
void termina(int cod);
void manda_inf(void);
void imprime_simbolo(simb *s, int par);
void ordena_tab_sym(void);
void sort(long int n);
int mprintf(char *s, ...);
void limpa_ts(void);

/* manda.c */
void manda_nome(char *nome, int arq, int saida);
void manda_entry_symbol(char *nome, int arq, int saida);
void manda_data_size(unsigned int valor, int arq, int saida);
void manda_program_size(unsigned int valor, int arq, int saida);
void manda_programa(int atomo, int arq, int saida);
void manda_chain_external(char *nome, int arq, int saida);
void manda_entry_point(char *nome, int arq, int saida);
void manda_end_module(int arq, int saida);
void manda(int num, int n, int arq, int saida);
void manda_bit(int bit, int arq, int t_saida);
void escreve(int handle, char *buffer, unsigned int count);
void inicia_saida0(void);
void inicia_saida1(void);
void fecha_saida0(int arq);
void fecha_saida1(int arq);
void copy(char *dst, char *org);
void manda_n_n(char *nome, int cod, int arq, int saida);
void manda_n(char *nome, int cod, int arq, int saida);
void manda_at_numero(int cod, int aloc, unsigned int valor, int arq, int saida);
