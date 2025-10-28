        CSEG

TRUE    EQU      1        ; Anything other than 0 is true
FALSE   EQU      0

IF TRUE
        DB       0,1,2,3,4
ELSE
        DB       4,3,2,1,0
ENDIF

TEMP    SET      100B
IF TEMP EQ 4
        DB       5,6,7,8,9
ELSE
        DB       9,8,7,6,5
ENDIF

TEMP    SET      10Q
IF TEMP EQ 8
        DB       10,11,12,13
ELSE
        DB       13,12,11,10
ENDIF
        
TEMP    SET      5D
IF TEMP EQ 5
        DB       14,15,16,17
ELSE
        DB       17,16,15,14
ENDIF

TEMP    SET      0A10H
IF TEMP EQ 2576
        DB      18,19
ELSE
        DB      19,18
ENDIF

A0      EQU     TEMP + 10        ;2586
A1      EQU     TEMP - 20        ;2556
A2      EQU     TEMP * 2         ;5152
A3      EQU     TEMP / 10        ;257
A4      EQU     TEMP MOD 10      ;6
A5      EQU     A4 SHL 2         ;24
A6      EQU     A0 SHR A4        ;40
A7      EQU     NOT A0           ;0xf5e5
A8      EQU     A0 EQ 2586       ;0xffff
A9      EQU     A0 EQ A1         ;0
A10     EQU     A1 LT A0         ;0xffff
A11     EQU     A3 LE 257        ;0xffff
A12     EQU     A3 GT 100H       ;0xffff
A13     EQU     A3 GE 101H       ;0xffff
A14     EQU     A7 AND 0FFFH     ;0x5e5
A15     EQU     A3 OR A4         ;0x107
A16     EQU     A7 XOR A14       ;0xf000
A17     EQU     HIGH A7          ;0xf5
A18     EQU     LOW A7           ;0xe5

IF A0 EQ 2586
        DB      20
 IF A1 EQ 2556
        DB      21
  IF A2 EQ 5152
        DB      22
   IF A3 EQ 257
        DB      23
    IF A4 EQ 6
        DB      24
     IF A5 EQ 24
        DB      25
      IF A6 EQ 40
        DB      26
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
IF A7 EQ 0F5E5H
        DB      27
 IF A8 EQ 0FFFFH
        DB      28
  IF A9 EQ 0
        DB      29
   IF A10 EQ 0FFFFH
        DB      30
    IF A11 EQ 0FFFFH
        DB      31
     IF A12 EQ 0FFfFH
        DB      32
      IF A13 EQ 0FFFFH
        DB      33
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF

IF A14 EQ 5E5H
        DB      34
 IF A15 EQ 107H
        DB      35
  IF A16 EQ 0F000H
        DB      36
   IF A17 EQ 0F5H
        DB      37
    IF A18 EQ 0E5H
        DB      38
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF

ONE    EQU      1
TWO    EQU      2
THREE  EQU      3
FOUR   EQU      4
FIVE   EQU      5
SIX    EQU      6
SEVEN  EQU      7
EIGHT  EQU      8
NINE   EQU      9
TEN    EQU      10

; Test precedences
; * / MOD SHL SHR
IF (SIX * SEVEN MOD NINE SHL 4 SHR 2) EQ 24
        DB      39
 IF (SIX * (SEVEN MOD FOUR) SHL (4 SHR 2)) EQ 36
        DB      40
ENDIF
ENDIF

; + -
IF FIVE * SIX + SEVEN * EIGHT EQ 86
        DB      41
 IF FIVE * SIX - SEVEN * EIGHT EQ -26
        DB      42
ENDIF
ENDIF

; EQ LT LE GT GE NE
; NOT
; AND
; XOR OR
IF THREE + TWO EQ SIX - ONE
        DB      43
 IF EIGHT - TWO LT FIVE + TWO
        DB      44
  IF FOUR + THREE LE EIGHT - ONE
        DB      45
   IF NINE - TWO GT FOUR + ONE
        DB      46
    IF TWO * TWO GE FIVE - ONE
        DB      47
     IF FIVE + TWO NE TWO * THREE 
        DB      48
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
    
IF NOT THREE + TWO NE SIX - ONE
        DB      49
 IF NOT EIGHT - TWO GE FIVE + TWO
        DB      50
  IF NOT FOUR + THREE GT EIGHT - ONE
        DB      51
   IF NOT NINE - TWO LE FOUR + ONE
        DB      52
    IF NOT TWO * TWO LT FIVE - ONE
        DB      53
     IF NOT FIVE + TWO EQ TWO * THREE 
        DB      54
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
        
IF TWO + THREE EQ FIVE OR FOUR * TWO EQ SEVEN
        DB      55
 IF TWO + SIX EQ FIVE OR FOUR * TWO EQ EIGHT
        DB      56
  IF TWO + SIX EQ EIGHT OR THREE * TWO EQ SIX
        DB      57
   IF NOT (TWO + THREE EQ FIVE AND FOUR * TWO EQ SEVEN)
        DB      58
    IF NOT (TWO + SIX EQ FIVE AND FOUR * TWO EQ EIGHT)
        DB      59
     IF TWO + SIX EQ EIGHT AND THREE * TWO EQ SIX
        DB      60
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF

IF THREE OR FOUR EQ SEVEN
        DB      61
 IF SEVEN AND THREE EQ THREE
        DB      62
  IF SEVEN XOR THREE EQ FOUR
        DB      63
ENDIF
ENDIF
ENDIF

; HIGH LOW
IF (HIGH (TWO * FOUR + THREE) SHL (THREE + NINE)) EQ 0B0H
        DB      64
 IF (LOW TEN * TEN * TEN) EQ 0E8H
        DB      65
ENDIF
ENDIF

        END
        
