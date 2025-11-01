        MACLIB      macrosrc                ; Filename will be converted to MACROSRC.LIB
                CSEG
        ORG         0

COUNT   SET         0                        ; 0x01 - 0x64
        REPT        10
        REPT        10
COUNT   SET         COUNT+1
        DB          COUNT
        ENDM
        ENDM

; The following code produces the following binary:
; Address = 0x64
; 0x00
; 0xc3 0x64 0x00
; 0x00
; 0xc3 0x68 0x00
; 0x00
; 0xc3 0x6c 0x00
; 0x00
; 0xc3 0x70 0x00
        
        REPT       2
        LOCAL      L1,L2
L1:     NOP
        JMP        L1
L2:     NOP
        JMP        L2
        ENDM

CONST1  EQU        100
CONST2  EQU        200
CONST3  EQU        300
CONST4  EQU        400

; The following code produces the following binary:
; Address = 0x74
; 0x64 0x00 0xc8 0x00 0xc3 0x7b 0x00
; 0x2c 0x01 0x90 0x01 0xc3 0x82 0x00
;  
        MY_MACRO   CONST1, CONST2
        MY_MACRO   CONST3, CONST4
