#include <stdint.h>

#define LINUX
#define bottom_up /* expressao bottom-up ou top-down */
#define FAST_READ /* arquivos de leitura tratados como binario, para tornar leitura + rapida */
                  /* assume-se que os finais de linha dos arquivos sempre sao 0xd 0xd */
#ifdef FAST_READ
#define _EOF 0x1a
#else
#define _EOF EOF /* poderia ser qualquer coisa. EOF so' para ficar igual `as versoes anteriores */
#endif

#define O_BINARY 0
#define O_TEXT 0

#define hash(r, c) ((~(r) >> 1) ^ (((~(r) >> 4) & 0xf) + ((~(r) << 4) & 0xf0)) + ((c) << 2))
#define n_rot_aloc_ats 12 /* numero de bits para determinar mascara de alocacao de atomos */
#define max_aloc 160	  /* numero maximo de unidades alocaveis */
#define nrot_aloc 10
#define nsimb_aloc (1 << nrot_aloc) /* numero de simbolos alocados por vez */
#define c_mask_aloc ((1 << ((sizeof(int)) * 8 - nrot_aloc)) - 1)
#define mask_aloc (nsimb_aloc - 1)
#define max_simb_aloc 32 /* numero maximo de unidades de tabela de simbolos alocaveis */

/* 1.30 */
/*#define inic_simb_size 256
 */
#define inic_simb_size 991

#define max_ats
#define if_size 8

#ifdef bottom_up
#define comp_pilha 40
#else
#define comp_pilha 8
#endif

#define max_car_par 80
#define max_mac_call 8
#define max_macro 1024
#define max_macro_par 0x2000
#define max_clocais 1024 /* numero maximo de caracteres para labels locais */
#define comp_max 8
#define _MACLIB 1
#define definido 2
#define ocupado 4
#define ABS 4
#define COD 8
#define DAT 0x10
#define extrn 0x20
#define public 0x40
#define set 0x80
#define bit_rot_mem 12
#define masc_mem ((0x1 << bit_rot_mem) - 1)
#define max_mem_aloc (0x10000L / (masc_mem + 1))
#define n_rot_macro_char 11 /* numero de bits para determinar mascara de alocacao de caracteres de macros */
#define msk_rot_macro_char ((1 << n_rot_macro_char) - 1)
#define cmsk_rot_macro_char ((1 << (sizeof(int) * 8 - n_rot_macro_char)) - 1)
#define max_char_aloc (cmsk_rot_macro_char + 1)

/***************************** tipos de cpu *********************************/

#define i8080 0x01
#define i8085 0x02
#define z80 0x04
#define z180 0x08

typedef unsigned char t_atomo; /* tipo para atomo */

typedef struct simbl
{
    struct simbl *next_simbolo;
    unsigned int valor;
    char nome[comp_max + 1];
    char atrib; /* atributos: _MACLIB, definido, ABS, COD, DAT, extrn, public, set */
} simb;

typedef struct
{
    simb *s;
    uint16_t valor;
    char atr; /* atributo */
} number;

typedef struct
{
    char *pr;
    t_atomo i;
    unsigned char permissao;
} preservada;

typedef struct
{
    unsigned int os;
    char a;
} ext;

typedef struct
{
    unsigned int pchar;		 /* primeiro caracter */
    int ppar;				 /* primeiro parametro */
    int npar;				 /* numero de parametros */
    char nome[comp_max + 1]; /* nome da macro */
    char atrib;				 /* atributo: _MACLIB, ocupado, definido */
} macro_desc;

typedef struct
{
    int if_counter;
    int monta;
    int if_stack[if_size];
    unsigned int clocais;
    int nparm;
    int mac_np;
    int mac_pp;
    int mac_nl;
    unsigned int m_loops;
    unsigned int n_rept;
    int exp_tipo_macro;
    unsigned int mac_char;
    char nomes_locais[max_clocais];
    char mpar[max_car_par];
} mac_save;

typedef enum
{
    DUPLA_DEFINICAO = 1,
    NUMERO_MUITO_GRANDE,
    NUMERO_INVALIDO,
    VIRGULA_ESPERADA,
    INSTRUCAO_INVALIDA,
    PARAMETRO_INVALIDO,
    EXPRESSAO_INVALIDA,
    PROGRAMA_MUITO_GRANDE,
    FECHA_PARENTESES_ESPERADO,
    NUMERO_ESPERADO,
    NOT_INVALIDO,
    UNARIO_MENOS_INVALIDO,
    HIGH_COM_NUMERO_RELOCAVEL,
    LOW_COM_NUMERO_RELOCAVEL,
    AND_INVALIDO,
    SOMA_INVALIDA,
    SUBTRACAO_INVALIDA,
    COMPARACAO_INVALIDA,
    ERRO_INTERNO,
    MUITOS_OPERANDOS_PENDENTES,
    IDENTIFICADOR_INDEFINIDO,
    TABELA_DE_SIMBOLOS_CHEIA,
    OR_INVALIDO,
    XOR_INVALIDO,
    NOME_ESPERADO,
    MUITOS_EXTERNOS_DEFINIDOS,
    PASSO_2_DIFERENTE_DO_1,
    STRING_MUITO_GRANDE,
    STRING_NAO_FOI_FECHADA,
    MUITOS_IFS_ANINHADOS,
    ELSE_SEM_IF_CORRESPONDENTE,
    ENDIF_SEM_IF_CORRESPONDENTE,
    IFS_DESANINHADOS,
    MACLIB_EM_ARQUIVO_INCLUIDO,
    MUITOS_NIVEIS_DE_PARENTESES,
    DIVISAO_POR_ZERO,
    ENDM_ESPERADO,
    NUMERO_DE_MACROS_EXCEDIDO,
    MUITO_CODIGO_EM_MACROS,
    MACROS_COM_MUITOS_PARS,
    MUITAS_MACROS_SENDO_EXPANDIDAS,
    MUITOS_PARAMETROS,
    MEMORIA_INSUFICIENTE,
    MUITOS_LOCAIS,
    MULTIPLICACAO_INVALIDA,
    ERRO_DE_FASE,
    FIM_DE_LINHA_ESPERADO,
    PUBLIC_E_SET,
    DIVISAO_INVALIDA,
    DESLOCAMENTO_INVALIDO,
    CARACTER_INESPERADO,
    ARQUIVO_PCH_INCONSISTENTE
} tipo_erros;

typedef enum
{
    B,
    C,
    D,
    E,
    H,
    L,
    M,
    A
} registradores;

typedef enum
{
    EQ = A + 1,
    NE,
    LE,
    GE,
    LT,
    GT
} comparadores;

typedef enum
{
    ACI = GT + 1,
    ADC,
    ADD,
    ADI,
    ANA,
    AND,
    ANI,
    ASEG,
    BIT,
    BSET,
    CALL,
    CC,
    CM,
    CMA,
    CMC,
    CMD,
    CMDR,
    CMI,
    CMIR,
    CMP,
    CNC,
    CNZ,
    CP,
    CPE,
    CPI,
    CPO,
    CSEG,
    CZ,
    DAA,
    DAD,
    DADC,
    DADX,
    DADY,
    DB,
    DCR,
    DCX,
    DI,
    DJNZ,
    DS,
    DSBB,
    DSEG,
    DW,
    EI,
    ELSE,
    END,
    ENDIF,
    ENDM,
    EQU,
    EXITM,
    EXTRN,
    HIGH,
    HLT,
    I,
    IF,
    IFDEF,
    IFNDEF,
    IM,
    IN,
    IN0,
    INC,
    IND,
    INDR,
    INI,
    INIR,
    INR,
    INX,
    IRP,
    IX,
    IY,
    JC,
    JCR,
    JM,
    JMP,
    JNC,
    JNCR,
    JNZ,
    JNZR,
    JP,
    JPE,
    JPO,
    JR,
    JZ,
    JZR,
    LBCD,
    LDA,
    LDAX,
    LDD,
    LDDR,
    LDED,
    LDI,
    LDIR,
    LHLD,
    LIXD,
    LIYD,
    LOCAL,
    LOW,
    LSPD,
    LXI,
    MACLIB,
    MACRO,
    MLT,
    MOV,
    MVI,
    NAME,
    NEG,
    NOP,
    NOT,
    NUL,
    OR,
    ORA,
    ORG,
    ORI,
    OTDM,
    OTDMR,
    OTDR,
    OTIM,
    OTIMR,
    OTIR,
    OUT,
    OUT0,
    OUTC,
    OUTD,
    OUTI,
    PCHL,
    PCIX,
    PCIY,
    POP,
    PSW,
    PUBLIC,
    PUSH,
    R,
    RAL,
    RAR,
    RC,
    REPT,
    RES,
    RET,
    RETI,
    RETN,
    RIM,
    RLC,
    RLD,
    RM,
    RNC,
    RNZ,
    RP,
    RPE,
    RPO,
    RRC,
    RRD,
    RST,
    RZ,
    SBB,
    SBCD,
    SBI,
    SDED,
    SET,
    SHLD,
    SIM,
    SIXD,
    SIYD,
    SLA,
    SLP,
    SP,
    SPHL,
    SPIX,
    SPIY,
    SRA,
    SRL,
    SSPD,
    STA,
    STAX,
    STC,
    SUB,
    SUI,
    TST,
    TSTIO,
    XCHG,
    XCHX,
    XPSW,
    XOR,
    XRA,
    XRI,
    XTHL
} palavras_reservadas;

typedef enum
{
    AP = XTHL + 1,
    FP,
    LABEL,
    MAIS,
    MENOS,
    VIRGULA,
    EOL,
    EOLM, /* fim de linha em macro */
    EOA,
    NUMERO,
    NOME,
    STRING,
    PC,
    ERRO
} atomo;

typedef enum
{
    VEZES = ERRO + 1,
    DIVIDE,
    MOD,
    SHL,
    SHR
} opmult;

#if (SHR >= 256)
ATENCAO::NUMERO DE ATOMOS MUITO GRANDE !!!!!!!!
#endif
