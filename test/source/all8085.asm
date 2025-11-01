; Based on the nice table in https://pastraiser.com/cpu/i8085/i8085_opcodes.html

CTE     EQU        34

        cseg
        org 0
        JMP        code                                                                        ;0x0000

        org 44
        jmp        rst55                                                                       ;0x002c

; Let us try to go the numerical order as much as possible here
var0:   dw       1                                                                             ;0x002f 
var1:   db       2
code:   NOP               ;0x00                                                                ;0x0032
        LXI      B,1234   ;0x01 0xd2 0x04
rst55:  STAX     B        ;0x02
        INX      B        ;0x03
        INR      B        ;0x04
        DCR      B        ;0x05
        MVI      B,0FEh   ;0x06 0xfe
        RLC               ;0x07
;       DSUB              ;0x08 - not documented and not implemented by A80
        DAD      B        ;0x09
        LDAX     B        ;0x0a
        DCX      B        ;0x0b
        INR      C        ;0x0c                                                                ;0x0040
        DCR      C        ;0x0d
        MVI      C,254    ;0x0e 0xfe
        RRC               ;0x0f
        
;       ARHL              ;0x10 - not documented and not implemented by A80       
        LXI      D,1234H  ;0x11 0x34 0x12
        STAX     D        ;0x12
        INX      D        ;0x13
        INR      D        ;0x14
        DCR      D        ;0x15
        MVI      D,12H    ;0x16 0x12
        RAL               ;0x17
;       RDEL              ;0x18 - not documented and not implemented by a80
        DAD      D        ;0x19
        LDAX     D        ;0x1a                                                                ;0x0050
        DCX      D        ;0x1b
        INR      E        ;0x1c
        DCR      E        ;0x1d
        MVI      E,034H   ;0x1e 0x34
        RAR               ;0x1f
        
LBL2:   RIM               ;0x20                                                                ;0x0057
        LXI      H,VAR0   ;0x21 <address>
        SHLD     VAR1     ;0x22 <address>
        INX      H        ;0x23
        INR      H        ;0x24
	DCR      H        ;0x25                                                                ;0x0060
	MVI      H,CTE    ;0x26 <byte>
        DAA               ;0x27
;       LDHI d8           ;0x28 - not documented and not implemented by a80
        DAD      H        ;0x29
        LHLD     LBL2     ;0x2a addr
        DCX      H        ;0x2b
        INR      L        ;0x2c
	DCR      L        ;0x2d
	MVI      L,LOW(CTE)        ;0x2e byte
        CMA               ;0x2f

        SIM               ;0x30
        LXI      SP,STACK ;0x31 <address>                                                      ;0x006f
        STA      VAR1     ;0x32 <address>
        INX      SP       ;0x33
        INR      M        ;0x34
	DCR      M        ;0x35
        MVI      M,0d8H   ;0x36 0xd8
        STC               ;0x37
;	LDSI     d8       ;0x38 - not implemented
        DAD      SP       ;0x39
        LDA      VAR1     ;0x3a
        DCX      SP       ;0x3b 
        INR      A        ;0x3c                                                                ;0x0080
	DCR      A        ;0x3d
	MVI      A,0d9h   ;0x3e
        CMC               ;0x3f

        MOV      B,B      ;0x40
        MOV      B,C      ;0x41
        MOV      B,D      ;0x42
        MOV      B,E      ;0x43
        MOV      B,H      ;0x44
        MOV      B,L      ;0x45
        MOV      B,M      ;0x46
        MOV      B,A      ;0x47
        MOV      C,B      ;0x48
        MOV      C,C      ;0x49
        MOV      C,D      ;0x4a
        MOV      C,E      ;0x4b                                                                ;0x0090
        MOV      C,H      ;0x4c
        MOV      C,L      ;0x4d
        MOV      C,M      ;0x4e
        MOV      C,A      ;0x4f

        MOV      D,B      ;0x50
        MOV      D,C      ;0x51
        MOV      D,D      ;0x52
        MOV      D,E      ;0x53
        MOV      D,H      ;0x54
        MOV      D,L      ;0x55
        MOV      D,M      ;0x56
        MOV      D,A      ;0x57
        MOV      E,B      ;0x58
        MOV      E,C      ;0x59
        MOV      E,D      ;0x5a
        MOV      E,E      ;0x5b                                                                ;0x00a0
        MOV      E,H      ;0x5c
        MOV      E,L      ;0x5d
        MOV      E,M      ;0x5e
        MOV      E,A      ;0x5f

        MOV      H,B      ;0x60
        MOV      H,C      ;0x61
        MOV      H,D      ;0x62
        MOV      H,E      ;0x63
        MOV      H,H      ;0x64
        MOV      H,L      ;0x65
        MOV      H,M      ;0x66
        MOV      H,A      ;0x67
        MOV      L,B      ;0x68
        MOV      L,C      ;0x69
        MOV      L,D      ;0x6a
        MOV      L,E      ;0x6b                                                                ;0x00b0
        MOV      L,H      ;0x6c
        MOV      L,L      ;0x6d
        MOV      L,M      ;0x6e
        MOV      L,A      ;0x6f

        MOV      M,B      ;0x70
        MOV      M,C      ;0x71
        MOV      M,D      ;0x72
        MOV      M,E      ;0x73
        MOV      M,H      ;0x74
        MOV      M,L      ;0x75
        HLT               ;0x76
        MOV      M,A      ;0x77
        MOV      A,B      ;0x78
        MOV      A,C      ;0x79
        MOV      A,D      ;0x7a
        MOV      A,E      ;0x7b                                                                ;0x00c0
        MOV      A,H      ;0x7c
        MOV      A,L      ;0x7d
        MOV      A,M      ;0x7e
        MOV      A,A      ;0x7f

        ADD      B        ;0x80
        ADD      C        ;0x81
        ADD      D        ;0x82
        ADD      E        ;0x83
        ADD      H        ;0x84
        ADD      L        ;0x85
        ADD      M        ;0x86
        ADD      A        ;0x87
        ADC      B        ;0x88
        ADC      C        ;0x89
        ADC      D        ;0x8a
        ADC      E        ;0x8b                                                                ;0x00d0
        ADC      H        ;0x8c
        ADC      L        ;0x8d
        ADC      M        ;0x8e
        ADC      A        ;0x8f

        SUB      B        ;0x90
        SUB      C        ;0x91
        SUB      D        ;0x92
        SUB      E        ;0x93
        SUB      H        ;0x94
        SUB      L        ;0x95
        SUB      M        ;0x96
        SUB      A        ;0x97
        SBB      B        ;0x98
        SBB      C        ;0x99
        SBB      D        ;0x9a
        SBB      E        ;0x9b                                                                ;0x00e0
        SBB      H        ;0x9c
        SBB      L        ;0x9d
        SBB      M        ;0x9e
        SBB      A        ;0x9f

        ANA      B        ;0xa0
        ANA      C        ;0xa1
        ANA      D        ;0xa2
        ANA      E        ;0xa3
        ANA      H        ;0xa4
        ANA      L        ;0xa5
        ANA      M        ;0xa6
        ANA      A        ;0xa7
        XRA      B        ;0xa8
        XRA      C        ;0xa9
        XRA      D        ;0xaa
        XRA      E        ;0xab                                                                ;0x00f0
        XRA      H        ;0xac
        XRA      L        ;0xad
        XRA      M        ;0xae
        XRA      A        ;0xaf


FUN1:
FUN2:   ORA      B        ;0xb0                                                                ;0x00f5
        ORA      C        ;0xb1
        ORA      D        ;0xb2
        ORA      E        ;0xb3
FUN3:   ORA      H        ;0xb4
        ORA      L        ;0xb5
        ORA      M        ;0xb6
        ORA      A        ;0xb7
        CMP      B        ;0xb8
        CMP      C        ;0xb9
        CMP      D        ;0xba
        CMP      E        ;0xbb                                                                ;0x0100
        CMP      H        ;0xbc
        CMP      L        ;0xbd
        CMP      M        ;0xbe
        CMP      A        ;0xbf

LBL5:
LBL6:   RNZ               ;0xc0                                                                ;0x0105
        POP      B        ;0xc1
        JNZ      LBL5     ;0xc2 <address>
        JMP      LBL6     ;0xc3 <address>
        CNZ      FUN1     ;0xc4 <address>
        PUSH     B        ;0xc5                                                                ;0x0110
        ADI      78       ;0xc6 <byte>
LBL7:   RST      0        ;0xc7
        RZ                ;0xc8
        RET               ;0xc9
        JZ       LBL7     ;0xca <address>
;       *RSTV             ;0xcb - not implemented
        CZ       FUN2     ;0xcc <address>
        CALL     FUN3     ;0xcd <address>
        ACI      89       ;0xce <byte>                                                        ;0x011f
        RST      1        ;0xcf

        RNC               ;0xd0
        POP      D        ;0xd1
        JNC      LBL5     ;0xd2 <address>
        OUT      34h      ;0xd3 <byte>
        CNC      FUN3     ;0xd4 <address>
        PUSH     D        ;0xd5
        SUI      56h      ;0xd6 <byte>
        RST      2        ;0xd7
        RC                ;0xd8                                                                ;0x0130
;       *SHLX             ;0xd9 - not implemented
        JC       LBL7     ;0xda <address
        IN       90       ;0xdb <byte>
        CC       FUN2     ;0xdc <address>
;       *JNK     LBL5     ;0xdd <address> - not implemented
        SBI      09ah     ;0xde <byte>
        RST      3        ;0xdf

        RPO               ;0xe0
        POP      H        ;0xe1
        JPO      6789h    ;0xe2 <address>
        XTHL              ;0xe3                                                                ;0x0141
        CPO      9abch    ;0xe4 <address>
        PUSH     H        ;0xe5
        ANI      90       ;0xe6 <byte>
        RST      4        ;0xe7
        RPE               ;0xe8
        PCHL              ;0xe9
        JPE      0abcdh   ;0xea
        XCHG              ;0xeb
        CPE      1234     ;0xec 0xd2 0x04                                                       ;0x014f
;       *LHLX             ;0xed - not implemented
        XRI      34       ;0xee <byte>
        RST      5        ;0xef

        RP                ;0xf0
        POP      PSW      ;0xf1
        JP       4321     ;0xf2 <address>
        DI                ;0xf3
        CP       5679     ;0xf4 <address>
        PUSH     PSW      ;0xf5
        ORI      10       ;0xf6 <byte>
        RST      6        ;0xf7                                                                ;0x0161
        RM                ;0xf8
        SPHL              ;0xf9
        JM       9089     ;0xfa <address>
        EI                ;0xfb
        CM       8085     ;0xfc <address>
;       *JK a16           ;0xfd <address> - not implemented
        CPI      90       ;0xfe <byte>
        RST      7        ;0xff                                                                ;0x016d

        DSEG
        ORG    2000H
VAR9:   DS     2
        DS     1024
STACK   EQU    $
        
