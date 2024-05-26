/******************************* DEFINES ************************************/

#define hash(r,c) ((~(r) >> 1) ^ (((~(r) >> 4) & 0xf) + ((~(r) << 4) & 0xf0)) + ((c) << 2))
#define part_bits 12								/* numero de bits para tamanho de particao de data e code */
#define comp_part (0x0001 << part_bits)	/* comprimento de particao */
#define mask_part (comp_part - 1)			/* mascara para particao */
#define num_part (0x10000 / comp_part)		/* numero maximo de particoes */
#define os_bits 9									/* numero de bits para uma particao da tabela de off-sets */
#define comp_prt_os (0x0001 << os_bits)
#define mask_prt_os (comp_prt_os - 1)
#define num_prt_os (0x8000 / comp_prt_os)
#define comp_max 8								/* maximo numero de caracteres que pode ter um nome */
#define inic_simb_size 256						/* comprimento da tabela de ponteiros para tabela de simbolos */
#define max_simb_aloc 32						/* numero maximo de unidades de tabela de simbolos alocaveis */
#define nrot_aloc 10
#define nsimb_aloc (1 << nrot_aloc)			/* numero de simbolos alocados por vez */
#define c_mask_aloc ((1 << ((sizeof (int)) - nrot_aloc)) - 1)
#define mask_aloc (nsimb_aloc - 1)

typedef struct simbl
	{
	char nome[comp_max + 1];
	char atrib;								/* atributos: C, D, A */
	unsigned int valor;
	char atrib_p;							/* atributo do endereco do simbolo anterior na lista ligada */
	unsigned int prev;					/* endereco do simbolo anterior na lista ligada */
	char definido;							/* indica que simbolo ja' foi definido */
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
void faz_link (int argc, char *argv []);
void explica (void);
int cdecl mprintf (char *s, ...);
int pega_hex (char *p, int *n);
void inicia_linc (void);
int linca (char *arq);
char *nomeok (char *n, char *ext);
char *poe_ext (char *n, char *ext);
char *str_maiuscula (char *s);
void termina (int cod);
void trata_arq (int lib);
void trata_program_name (void);
int trata_entry_symbol (int lib);
void trata_define_data_size (void);
void trata_define_program_size (void);
void trata_end_module (void);
int monta_destinos (char *nome, char *sym, char *rel);
char *combina (char *dest, char *org, char *ext);
char *tira_path (char *org);
void apaga_arqs (char *nome, char *sym, char *rel);

/* analex.c */
void inicia_analex (void);
int analex (void);
void pega_nome (void);
void ver_nome (char *s, int comp);
unsigned int pega_8bits (void);
int pega_4bits (void);
int pega_3bits (void);
int pega_2bits (void);
int pega_bit (void);
simb *get_simb (void);
void pega_numero (void);
void pega_valor (void);
void volta (int atomo);
void limpa_ts (void);

/* anass.c */
void trata_programa (void);
void coloca_byte (int byte);
void coloca_byte_cseg (int byte);
void coloca_byte_dseg (int byte);
void set_location_counter (unsigned int valor, char rel);
void coloca_external_plus_offset (unsigned int valor, char rel);
void coloca_program_relative (unsigned int valor);
void coloca_data_relative (unsigned int valor);
void get_bits (unsigned char *v, unsigned int end, unsigned char *m, unsigned char **b);
void trata_define_entry_point (void);
void trata_chain_external (void);
void pega_prev (char rel_o, unsigned int end_o, char *rel_d, unsigned int *end_d, void (*erro) (void));
int varre_t_sym (void);
void err_trt1 (void);
void err_trt2 (void);
void trata_nao_definidos (void);
void acerta_enderecos (unsigned int areac, unsigned int aread);
void trata_offsets (void);
void monta_arquivo (int arq, unsigned int code, unsigned int data);

/* saida.c */
void cria_sym (char *n);
void ordena_tab_sym (void);
void sort (long int n);
void imprime_simbolo (simb *s);
void manda_car_sym (char c);
void cria_rel (char *n);
void cria_rel (char *n);
void define_nome (char n[]);
void manda_nome (void);
void manda_entry_symbols (void);
void manda_comprimentos (void);
void manda_publics (void);
void manda_bit (int bit);
void manda (int num, int n);

/************************** VARIAVEIS GLOBAIS *******************************/

unsigned int i_cseg;								/* indice de codigo para code */
unsigned int i_dseg;								/* idem para data */
unsigned char *cseg [num_part];				/* particao code */
unsigned char *dseg [num_part];				/* particao data */

/* nos dois vetores abaixo, cada endereco gasta 2 bits, com as seguintes
 combinacoes:
	00 -> absoluto
	01 -> cseg
	10 -> dseg
	11 -> nao utilizado
*/
unsigned char modo_end_c [0x10000 / 4];	/* modo de enderecamento de code */
unsigned char modo_end_d [0x10000 / 4];	/* idem data */

unsigned int i_os;								/* indice de particoes de off-set */
typedef struct {
	char r_endereco;								/* alocacao do endereco onde somar o off-set */
	unsigned int endereco;						/* endereco onde somar o off-set */
	unsigned int os;								/* off-set */
	} off_set;
off_set *end_os [num_prt_os];
int l_file;											/* arquivo sendo lincado */
int voltou;											/* indica atomo voltado para analisador lexico */
int falta;											/* numero de bytes que falta ler do buffer de leitura de arquivo */
int devolvido;										/* atomo devolvido para analex */
int byte;											/* valor retornado por analex no caso de BYTE */
int mask;											/* mascara utilizada por analex para ler bits */
int leit;											/* indica numero de bytes do buffer lidos por analex */
simb *inic_simbolo [inic_simb_size];		/* ponteiros para tabela de simbolo */
simb *simbolo;										/* ponteiro para simbolo retornado por analex */
char bl [4096];									/* buffer de leitura de analex */
int resta_simb;									/* numero de simbolos ainda possiveis de serem usados na tabela */
int nset_simb;										/* numero de particoes de simbolo utilizadas */
simb *aloc_simb [max_simb_aloc];				/* ponteiro para arrays alocadas para simbolos */
int rel;												/* tipo de alocacao que e' o numero ('A', 'C' ou 'D') */
unsigned int valor;								/* valor retornado por analex */
int coloca_simbolo;								/* indica para analex que simbolo deve ser colocado se nao procurado */
char rel_atual;									/* particao (P ou D) atual de codigo */
unsigned int base_cseg;							/* endereco base do arquivo atual para cseg */
unsigned int base_dseg;							/* endereco base do arquivo atual para dseg */
char simbolo_analex [comp_max + 1];			/* nome do simbolo lido quando nao e' colocado na tabela de simbolos */
char nome_modulo [comp_max + 1];				/* nome do modulo sendo linkado */
int simbolo_redefinido;							/* indica simbolo definido mais do que 1 vez */
unsigned int catual, datual;					/* enderecos atuais dos segmentos */
char *nome;											/* nome do arquivo sendo linkado */
unsigned int areac, aread;						/* endereco da area de codigo e area de dados */

