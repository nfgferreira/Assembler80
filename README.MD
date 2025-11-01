# What is this

This is a assembler/linker/librarian package that is able to build and maintain 8080, 8085, Z80 and Z180 assembly code.

# The story behind it

By the mid-80s I was working as a developer to a small Brazilian company whose
products were based on the 8085 processor. Our development system was basically
a PC (very old ones -- the first was based on the 8088) to which we plugged a
board called BabyBlue, in contrast to the Big Blue (IBM). The BabyBlue had a Z80
processor and was able to run CP/M, which was something like an 8-bit version of
MS-DOS. The software we developed controlled a very demanding real time system,
so everything was written in assembly language. We used to assemble the software using a
Digital Research package that had an assembler (RMAC), a linker (LINK-80), and a
librarian (LIB-80). This package was very popular at the time.

By around 1989 or 1990 we already used 80386-based PCs, which were way faster
and more powerful than the first PCs we had few years prior. But that did not
improve the assembly time much, since it was executed by the old BabyBlues. On the
contrary, as the software grew in size, the assembly time took longer and longer
and frustration mounted. I do not remember the exact time it took to assembly
everything, but it should be something between 20 and 30 minutes or perhaps even more.

By around this time I went back to college in order to study towards my master's
degree. One of the courses was about compilers and I enjoyed the subject very
much. In order to consolidate my knowledge and at the same time to improve the
assembly performance, I decided to use my spare time to write a set of tools that could replace the
one that ran on the BabyBlue.

I began with the assembler. It needed to create files that had the same format
as the ones created by RMAC, since my ideal was to keep using LINK-80 (I
did not want to replace all the executables at once). I eventually ended up having success with the assembler and implemented
the linker. I do not remember whether we used the librarian or not by that time.
Perhaps I implemented one because that was necessary for the work, perhaps I
implemented one just because I already had implemented the assembler and the
linker and I wanted to have the whole package. In the end, I got the three of
them, which I called A80, L80 and LB80.

# Compatibility

A80 supports the same language as RMAC, with the following exceptions:

- As for the assembler directives, the following ones are supported: ORG, END,
EQU, SET, IF, ELSE, ENDIF, DB, DS, DW, ASEG, CSEG, DSEG, PUBLIC, EXTRN, NAME. The PAGE and TITLE directives are not supported, the reason being that A80
does not generate any listing. The COMMON directive is also not supported.

- As for the macro facilities, the command IRPC is not supported. This shouldn't
be a big problem because it can be replaced by IRP.

- As for the supported opcodes, A80 supports the instructions sets of 8080,
8085, Z80 and Z180. The meumonics undestood by the program are the ones as
defined by Intel, not Zilog. The instructions that are implemented by Z80 and
Z180 but not by 8080/8085 are implemented using the following mneumonics:


| A80 code                                      |	Z80/Z180(*) code                                |
|-----------------------------------------------|---------------------------------------------------|
| BIT b,r                                       | 	BIT b,r
| BIT b,M                                       |	BIT b,(HL)
| BSET b,r                                      |	SET b,r
| BSET b,M                                      |	SET b,(HL)
| CMD                                           |	CPD
| CMDR                                          |	CPDR
| CMI                                           |	CPI
| CMIR                                          |	CPIR
| CNV nnnn                                      |	CALL PO,nnnn (**)
| CV nnnn                                       |	CALL PE,nnnn (**)
| DADC [B \| D \| H \| SP]                      |	ADC HL,[BC \| DE \| HL \| SP]
| DADX [B \| D \| IX \| SP]                     |	ADD IX,[BC \| DE \| IX \| SP]
| DADY [B \| D \| IY \| SP]                     |	ADD IY,[BC \| DE \| IY \| SP]
| DJNZ o                                        |	DJNZ o
| DSBB [B \| D \| H \| SP]                      |	SBC HL, [BC \| DE \| HL \| SP]
| IM [0 \| 1 \| 2]                              |	IM [0 \| 1 \| 2]
| INC [A \| B \| C \| D \| E \| H \| L]         |	IN [A \| B \| C \| D \| E \| H \| L],C
| IND                                           |	IND
| INDR                                          |	INDR
| INI                                           |   INI
| INIR                                          |	INIR
| JCR nnnn                                      |	JR C,o
| JNCR nnnn                                     |	JR NC,o
| JNV nnnn                                      |	JP PO,nnnn (**)
| JNZR nnnn                                     |	JR NZ,o
| JR nnnn                                       |	JR o
| JV nnnn                                       |	JP PE,nnnn (**)
| JZR nnnn                                      |	JR Z,o
| LBCD                                          |	LD BC,(nnnn)
| LDD                                           |	LDD
| LDDR                                          |	LDDR
| LDED                                          |	LD DE,(nnnn)
| LDI                                           |	LDI
| LDIR                                          |	LDIR
| LIXD                                          |	LD IX,(nnnn)
| LIYD                                          |	LD IY,(nnnn)
| LSPD                                          |	LD SP,(nnnn)
| MLT [B \| D \| H \| SP]                       |	MLT [BC \| DE \| HL \| SP] (*)
| NEG                                           |	NEG
| OTDM                                          |	OTDM (*)
| OTDMR                                         |	OTDMR (*)
| OTDR                                          |	OTDR
| OTIM                                          |	OTIM (*)
| OTIMR                                         |	OTIMR (*)
| OTIR                                          |	OTIR
| OUT0 m,[A \| B \| C \| D \| E \| H \| L]      |	OUT0 (m),[A \| B \| C \| D \| E \| H \| L] (*)
| OUTC [A \| B \| C \| D \| E \| H \| L]        |	OUT (C),[A \| B \| C \| D \| E \| H \| L]
| OUTD                                          |	OUTD
| OUTI                                          |	OUTI
| PCIX                                          |	JP (IX)
| PCIY                                          |	JP (IY)
| RES b, [A \| B \| C \| D \| E \| H \| L \| M] |	RES b,[A \| B \| C \| D \| E \| H \| L \| (HL)]
| RES b,[IX | IY]+o                             |	RES b,([IX | IY ]+o)
| RETI                                          |	RETI
| RETN                                          |	RETN
| RLD                                           |	RLD
| RNV                                           |	RET PO (**)
| RRD                                           |	RRD
| RV                                            |	RET PE (**)
| SBCD nnnn                                     |	LD (nnnn),BC
| SDED nnnn                                     |	LD (nnnn),DE
| SIXD nnnn                                     |	LD (nnnn),IX
| SIYD nnnn                                     |	LD (nnnn),IY
| SLA [A \| B \| C \| D \| E \| H \| L \| M]    |	SLA [ r | (HL) ]
| SLA [ IX \| IY ] + o                          |	SLA ([ IX \| IY] + o)
| SLP                                           |	SLP (*)
| SPIX                                          |	LD SP,IX
| SPIY                                          |	LD SP,IY
| SRA [A \| B \| C \| D \| E \| H \| L \| M]    |	SRA [ r \| (HL) ]
| SRA [ IX \| IY ] + o                          |	SRA ([ IX \| IY] + o)
| SRL [A \| B \| C \| D \| E \| H \| L \| M]    |	SRL [ r \| (HL) ]
| SRL [ IX \| IY ] + o                          |	SRL ([ IX \| IY] + o)
| SSPD                                          |	LD (nnnn),SP
| TST [A \| B \| C \| D \| E \| H \| L]         |	TST [A \| B \| C \| D \| E \| H \| L] (*)
| TST M                                         |	TST (HL) (*)
| TST nn                                        |	TST nn (*)
| TSTIO nn                                      |	TSTIO nn (*)
| XCHX                                          |	EXX
| XPSW                                          |	EX AF,AF’


(*) Z180 exclusive instruction. All the others are Z80's too.
(**) CPO, CPE, JPO, JPE, RPO, RPE can be used instead of those.

# Building

Clone this repository and move to the directory of the program you want to build (A80, LB80 or L80).

Then execute the commands bellow.

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

If you want to debug the software, call cmake with the option -DCMAKE_BUILD_TYPE=Debug instead.

Tests
=====

A small amount of tests is executed by running a Python script. The easiest way to run them
is going to directory test and executing the bash script test.sh.

LB80 is not tested. The Z80/Z180 mneumonics are also not tested.
