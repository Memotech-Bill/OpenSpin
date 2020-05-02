Introduction
============

This is a fork of the Parallax OpenSpin compiler for the Propeller chip. The original is at:
https://github.com/parallaxinc/OpenSpin

This version adds one extra option "-l" to produce an "Annotated Listing" of the generated Propeller binary.

An abreviated example of the resulting listing is shown below.

Status
======

*   The only changes to the existing code are:
    +   Adding processing of the "-l" option.
    +   Adding the "Annotate.h" header as required.
    +   Including calls to routines AL_* (...) all of which have input only parameters.
    So I am certain that the generated code will be identical to the original source.

*   By comparison with the binary data I am reasonably confident that the PUB, PRI and DAT
    blocks are correctly placed.

*   The translation of the Spin ByteCode bytes to mnemonics is based upon the information in:
    +   https://github.com/rosco-pc/propeller-wiki/wiki/Spin-Byte-Code
    +   http://forums.parallax.com/discussion/111684/spin-bytecode
    The result looks plausible, but there are some ambiguities so this may not always be correct.

*   The placement of variables looks plausible, but there is less information to cross reference.
    Object arrays and nested sub-objects have not been tested.

*   The Visual Studio project files have not been updated.

Example
=======

	Propeller Spin/PASM Compiler 'OpenSpin' (c)2012-2018 Parallax Inc. DBA Parallax Semiconductor.
	Fork by Memotech Bill to add "Annotated Listing" of generated binary.
	https://github.com/Memotech-Bill/OpenSpin
	Version 1.01.00 Compiled on May  2 2020 16:11:32
	Compiling...
	MTX_Monitor_11.spin
	|-MTX_Video_8.spin
	|-MTX_Input_2.spin
	|-MTX_ReadBack_4.spin
	|-MTX_VDP_5.spin
	Done.
	Program size is 28060 bytes
	0000     03D09000      Frequency 64000000
	0004     67            Clock Mode
	0005     D9            Check Sum
	0006     0010          Base of Program
	0008     6D9C          Base of Variables
	000A     75D4          Base of Stack
	000C     01C4          Initial Program Counter
	000E     75D8          Initial Stack Pointer
	                       File "MTX_Monitor_11.spin"
	0010     1580 0417     Link to Next Object
	0014     01B4 0000     Link to MAIN
	0018     0235 0000     Link to GETPOS
	001C     0246 0000     Link to GETADDR
	0020     0275 0004     Link to SETATTR
	0024     029F 0010     Link to BLANK
	0028     0367 000C     Link to GRAPHCHR
	002C     03DD 0000     Link to MASKWRITE
	0030     0428 0000     Link to MASKMOVE
	0034     04C2 000C     Link to SCROLL
	0038     053C 0008     Link to COPYCHRS
	003C     062D 0000     Link to CSRDOWN
	0040     0651 0000     Link to CSRRIGHT
	0044     0666 0000     Link to ADDCHR
	0048     0677 000C     Link to INSERTLINE
	004C     06B3 000C     Link to DELETELINE
	0050     06F7 0008     Link to HEX
	0054     071F 0010     Link to PLOT
	0058     0862 0004     Link to DRAW
	005C     0953 0000     Link to SETMODE
	0060     09C6 0004     Link to PRINT
	0064     14A5 0004     Link to PRINTSTR
	0068     14B9 0008     Link to DEMO
	                       OBJ
	006C     1580 0828         vga :       "MTX_Video_8"
	0070     4120 082C         inp :       "MTX_Input_2"
	0074     41F0 0830         rdb :       "MTX_ReadBack_4"
	0078     43E0 0830         vdp :       "MTX_VDP_5"
	                       DAT
	007C 000 00                clrs        byte    $00, $0C, $30, $3C, $C0, $CC, $F0, $FC  ' Compatible mode colours
	007D     0C 30 3C C0 CC F0 FC
	0084 002 00000C00          abits       long     $0C00, $3000, $C000, $000C, $0030,   $00C0, blink, gmode   ' Compatable
	0088 003 00003000 0000C000 0000000C 00000030 000000C0 08000000 01000000
	00A4 00A 02000000                      long    uscore,     0, $1000,     0, invers,  $0010, blink, gmode   ' Mono
	00A8 00B 00000000 00001000 00000000 04000000 00000010 08000000 01000000
	00C4 012 02000000                      long    uscore,     0,     0,     0, invers, xorplt, blink, gmode   ' Enhanced
	00C8 013 00000000 00000000 00000000 04000000 20000000 08000000 01000000
	00E4 01A 02000000                      long    uscore,     0,     0,     0, invers, xorplt, blink, gmode   ' Graphics
	00E8 01B 00000000 00000000 00000000 04000000 20000000 08000000 01000000
	0104 022 00000000                      long    0               ' Zero value for ReadBack to return on startup
	0108 023 00                Mode        byte    mdCompat
	0109     14                            byte    20, 04, 11      ' Version bytes
	010A     04 0B
	010C 024 00                State       byte    stNormal
	010D     50                SWth        byte    80
	010E     04                CSize       byte    4
	010F     00                YTop        byte    0
	0110 025 00002000          PAttr       long    $0000_2000
	0114 026 00002000          NAttr       long    $0000_2000
	0118 027 50                WWth        byte    cols
	0119     18                WHgt        byte    vrows
	011A     00                WXOrg       byte    0
	011B     00                WYOrg       byte    0
	011C 028 01                WScr        byte    1
	011D     00                XPos        byte    0
	011E     00                YPos        byte    0
	011F     01                CsrOn       byte    1
	0120 029 00                Page        byte    0
	0121     00                CSet        byte    csNormal
	0122     00                WMask       byte    wmBoth
	0123     00                VScr        byte    0
	0124 02A 00002000          VScr0       long    $0000_2000, $0000_2000
	0128 02B 00002000
	012C 02C 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 0
	012D     18 00 00 01 00 00 01 00 00 00 00
	0138 02F 00002000          VScr1       long    $0000_2000, $0000_2000
	013C 030 00002000
	0140 031 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 1
	0141     18 00 00 01 00 00 01 00 00 00 01
	014C 034 00002000          VScr2       long    $0000_2000, $0000_2000
	0150 035 00002000
	0154 036 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 2
	0155     18 00 00 01 00 00 01 00 00 00 02
	0160 039 00002000          VScr3       long    $0000_2000, $0000_2000
	0164 03A 00002000
	0168 03B 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 3
	0169     18 00 00 01 00 00 01 00 00 00 03
	0174 03E 00002000          VScr4       long    $0000_2000, $0000_2000
	0178 03F 00002000
	017C 040 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 4
	017D     18 00 00 01 00 00 01 00 00 00 04
	0188 043 00002000          VScr5       long    $0000_2000, $0000_2000
	018C 044 00002000
	0190 045 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 5
	0191     18 00 00 01 00 00 01 00 00 00 05
	019C 048 00002000          VScr6       long    $0000_2000, $0000_2000
	01A0 049 00002000
	01A4 04A 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 6
	01A5     18 00 00 01 00 00 01 00 00 00 06
	01B0 04D 00002000          VScr7       long    $0000_2000, $0000_2000
	01B4 04E 00002000
	01B8 04F 50                            byte    cols, vrows, 0, 0, 1, 0, 0, 1, 0, csNormal, wmBoth, 7
	01B9     18 00 00 01 00 00 01 00 00 00 07
	                       PUB Main
	                               BufIn := 0
	01C4     35            ; Push_0
	01C5     41            ; Pop_Varmem_Long_0
	                               BufOut := 0
	01C6     35            ; Push_0
	01C7     45            ; Pop_Varmem_Long_1
	                               inp.Start (@Buffer, @BufIn, @BufOut)
	01C8     01            ; Frame_Call_Noreturn
	01C9     8B 20         ; Reference_Variablemem_Byte, $0020
	01CB     43            ; Reference_Varmem_Long_0
	01CC     47            ; Reference_Varmem_Long_1
	01CD     06 1801 061A  ; Objcall, $1801, $061A
	                               Text  := vdp.GetBuffer
	01D0     00            ; Frame_Call_Return
	01D1     06 1A02 0006  ; Objcall, $1A02, $0006
	                               FontAddr := vga.FontLoc
	01D5     00            ; Frame_Call_Return
	01D6     06 1703 0148  ; Objcall, $1703, $0148
	
	<<< Many lines omitted >>>
	
	                       File "MTX_Video_8.spin"
	1590     2BA0 0004     Link to Next Object
	1594     2B30 0008     Link to START
	1598     2B84 0004     Link to STOP
	159C     2B9B 0000     Link to FONTLOC
	                       DAT
	     000                               org
	15A0 000 5C7C00A5      video           jmp     #init0
	15A4 001 A2BD47F0      vsync           mov     tpage1, par     wz      ' Test for first cog
	15A8 002 A0BD47F1                      mov     tpage1, cnt             ' Get current time
	15AC 003 082946A2              if_z    wrlong  tpage1, pagead          ' Save time for first cog to hub ram
	15B0 004 A0FD4E02                      mov     line_cnt, #vs           ' Number of lines of vertical sync
	15B4 005 5CFD0E7B                      call    #blank_vsync            ' Output sync pulse
	15B8 006 08BD48A2                      rdlong  tpage2, pagead          ' Fetch time of first cog
	15BC 007 84BD48A3                      sub     tpage2, tpage1          ' Time difference between cogs
	15C0 008 A0FD4E1A      vb_lines        mov     line_cnt, #vb - 8       ' Number of lines in vertical back porch (cog 0 starts 8 lines early)
	15C4 009 5CFD0E7B                      call    #blank_vsync            ' Output back porch
	
	<<< Many lines omitted >>>
	
	6D95     C7 81 80      ; Reference_Objectmem_Long, $0180
	6D98     33            ; Pop_Return
	6D99     32            ; Return
	6D9A     00            ; Frame_Call_Return
	6D9B     00            ; Frame_Call_Return
	                       Variables for TOP (MTX_Monitor_11.spin)
	6D9C                   long BUFIN
	6DA0                   long BUFOUT
	6DA4                   long TEXT
	6DA8                   long TTOP
	6DAC                   long DATPTR
	6DB0                   long PATPART
	6DB4                   long FONTADDR
	6DB8                   long DATCTR
	6DBC                   byte BUFFER[2048]
	75BC                   byte GDAT[6]
	                       Variables for TOP.VGA (MTX_Video_8.spin)
	75C4                   byte COG[3]
	                       Variables for TOP.INP (MTX_Input_2.spin)
	75C8                   byte NCOG
	75CC                   Reserved 8 bytes.
	75D4                   Base of stack.
	75E4                   Top of stack.
	Program size is 28060 bytes
	