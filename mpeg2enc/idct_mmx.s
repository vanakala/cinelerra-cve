;
; MMX32 iDCT algorithm  (IEEE-1180 compliant) :: idct_mmx32()
;
; MPEG2AVI
; --------
;  v0.16B33 initial release
;
; This was one of the harder pieces of work to code.
; Intel's app-note focuses on the numerical issues of the algorithm, but
; assumes the programmer is familiar with IDCT mathematics, leaving the
; form of the complete function up to the programmer's imagination.
;
;  ALGORITHM OVERVIEW
;  ------------------
; I played around with the code for quite a few hours.  I came up
; with *A* working IDCT algorithm, however I'm not sure whether my routine
; is "the correct one."  But rest assured, my code passes all six IEEE 
; accuracy tests with plenty of margin.
;
;   My IDCT algorithm consists of 4 steps:
;
;   1) IDCT-row transformation (using the IDCT-row function) on all 8 rows
;      This yields an intermediate 8x8 matrix.
;
;   2) intermediate matrix transpose (mandatory)
;
;   3) IDCT-row transformation (2nd time) on all 8 rows of the intermediate
;      matrix.  The output is the final-result, in transposed form.
;
;   4) post-transformation matrix transpose 
;      (not necessary if the input-data is already transposed, this could
;       be done during the MPEG "zig-zag" scan, but since my algorithm
;       requires at least one transpose operation, why not re-use the
;       transpose-code.)
;
;   Although the (1st) and (3rd) steps use the SAME row-transform operation,
;   the (3rd) step uses different shift&round constants (explained later.)
;
;   Also note that the intermediate transpose (2) would not be neccessary,
;   if the subsequent operation were a iDCT-column transformation.  Since
;   we only have the iDCT-row transform, we transpose the intermediate
;   matrix and use the iDCT-row transform a 2nd time.
;
;   I had to change some constants/variables for my method to work :
;
;      As given by Intel, the #defines for SHIFT_INV_COL and RND_INV_COL are
;      wrong.  Not surprising since I'm not using a true column-transform 
;      operation, but the row-transform operation (as mentioned earlier.)
;      round_inv_col[], which is given as "4 short" values, should have the
;      same dimensions as round_inv_row[].  The corrected variables are 
;      shown.
;
;      Intel's code defines a different table for each each row operation.
;      The tables given are 0/4, 1/7, 2/6, and 5/3.  My code only uses row#0.
;      Using the other rows messes up the overall transform.
;
;   IMPLEMENTATION DETAILs
;   ----------------------
; 
;   I divided the algorithm's work into two subroutines,
;    1) idct_mmx32_rows() - transforms 8 rows, then transpose
;    2) idct_mmx32_cols() - transforms 8 rows, then transpose
;       yields final result ("drop-in" direct replacement for INT32 IDCT)
;
;   The 2nd function is a clone of the 1st, with changes made only to the
;   shift&rounding instructions.
;
;      In the 1st function (rows), the shift & round instructions use 
;       SHIFT_INV_ROW & round_inv_row[] (renamed to r_inv_row[])
;
;      In the 2nd function (cols)-> r_inv_col[], and
;       SHIFT_INV_COL & round_inv_col[] (renamed to r_inv_col[])
;
;   Each function contains an integrated transpose-operator, which comes
;   AFTER the primary transformation operation.  In the future, I'll optimize
;   the code to do more of the transpose-work "in-place".  Right now, I've
;   left the code as two subroutines and a main calling function, so other
;   people can read the code more easily.
;
;   liaor@umcc.ais.org  http:;members.tripod.com/~liaor
;  

;;;
;;; A.Stevens Jul 2000 easy-peasy quick port to nasm
;;; Isn't open source a sensible idea...
;;; 

;=============================================================================
;
;  AP-922   http:;developer.intel.com/vtune/cbts/strmsimd
; These examples contain code fragments for first stage iDCT 8x8
; (for rows) and first stage DCT 8x8 (for columns)
;
;============================================================================
		
%define INP eax		; pointer to (short *blk)
%define OUT ecx		; pointer to output (temporary store space qwTemp[])
%define TABLE ebx	; pointer to idct_tab_01234567[]
%define round_inv_row edx
%define round_inv_col edx

		
%define ROW_STRIDE 16 							; for 8x8 matrix transposer
%define BITS_INV_ACC	4						; 4 or 5 for IEEE
%define SHIFT_INV_ROW	(16 - BITS_INV_ACC)
%define SHIFT_INV_COL	(1 + BITS_INV_ACC +14 ) ;  changed from Intel's val)

		;;
		;; Variables and tables defined in C for convenience
		;;
extern	idct_r_inv_row				; 2 DWORDSs
extern  idct_r_inv_col				;    "
extern  idct_r_inv_corr				;    "
extern idct_tab_01234567		; Catenated table of coefficients
		
		;; 
		;;  private variables and functions
		;; 

SECTION .bss
align 16
; qwTemp:		resw 64				; temporary storage space, 8x8 of shorts

		
SECTION .text
		
		;; static void idct_mmx( short *blk 
global idct_mmx

idct_mmx:
	push ebp			; save frame pointer
	mov ebp, esp		; link

	push ebx		
	push ecx		
	push edx
	push edi
		
		;; 
		;; transform all 8 rows of 8x8 iDCT block
		;;

	; this subroutine performs two operations
	; 1) iDCT row transform
	;		for( i = 0; i < 8; ++ i)
	;			DCT_8_INV_ROW_1( blk[i*8], qwTemp[i] );
	;
	; 2) transpose the matrix (which was stored in qwTemp[])
	;        qwTemp[] -> [8x8 matrix transpose] -> blk[]

	mov INP, [ebp+8]			; INP = blk
	mov edi, 0x00;				; x = 0
	lea TABLE,[idct_tab_01234567]; ; row 0


;	lea OUT,   [qwTemp];
	mov OUT,    [ebp+12];
	lea round_inv_row,   [idct_r_inv_row]
    jmp lpa
				
	; for ( x = 0; x < 8; ++x )  ; transform one row per iteration
align 32		
lpa:
	movq mm0,   [INP] 		; 0 ; x3 x2 x1 x0

	movq mm1,   [INP+8] 	; 1 ; x7 x6 x5 x4
	movq mm2, mm0 ;				; 2 ; x3 x2 x1 x0

	movq mm3,   [TABLE] 	; 3 ; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

; ----------
	movq mm5, mm0 ;					; 5 ; x5 x1 x4 x0
	punpckldq mm0, mm0 ;			; x4 x0 x4 x0

	movq mm4,   [TABLE+8] ;	; 4 ; w07 w05 w03 w01
	punpckhwd mm2, mm1 ;			; 1 ; x7 x3 x6 x2

	pmaddwd mm3, mm0 ;				; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 ;				; 6 ; x7 x3 x6 x2

	movq mm1,   [TABLE+32] ;; 1 ; w22 w20 w18 w16
	punpckldq mm2, mm2 ;			; x6 x2 x6 x2

	pmaddwd mm4, mm2 ;				; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 ;			; x5 x1 x5 x1

	pmaddwd mm0,   [TABLE+16] ;; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 ;			; x7 x3 x7 x3

	movq mm7,   [TABLE+40] ;; 7 ; w23 w21 w19 w17
	pmaddwd mm1, mm5 ;				; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3,   [round_inv_row];; +rounder
	pmaddwd mm7, mm6 ;				; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2,   [TABLE+24] ;; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 ;				; 4 ; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5,   [TABLE+48] ;; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 ;				; 4 ; a1 a0

	pmaddwd mm6,   [TABLE+56] ;; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 ;				; 7 ; b1=sum(odd1) b0=sum(odd0)

	paddd mm0,   [round_inv_row];; +rounder
	psubd mm3, mm1 ;				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW ;		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 ;				; 4 ; a1+b1 a0+b0

	paddd mm0, mm2 ;				; 2 ; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW ;		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 ;				; 6 ; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 ;				; 4 ; a3 a2

	paddd mm0, mm5 ;				; a3+b3 a2+b2
	psubd mm4, mm5 ;				; 5 ; a3-b3 a2-b2

	add INP, 16;					; increment INPUT pointer -> row 1
	psrad mm4, SHIFT_INV_ROW ;		; y4=a3-b3 y5=a2-b2

;	add TABLE, 0;					;  TABLE += 64 -> row 1
	psrad mm0, SHIFT_INV_ROW ;		; y3=a3+b3 y2=a2+b2

;	movq mm2,   [INP] ;		; row+1; 0;  x3 x2 x1 x0
	packssdw mm4, mm3 ;				; 3 ; y6 y7 y4 y5

	packssdw mm1, mm0 ;				; 0 ; y3 y2 y1 y0
	movq mm7, mm4 ;				; 7 ; y6 y7 y4 y5

;	movq mm0, mm2 ;					; row+1;  2 ; x3 x2 x1 x0
	psrld mm4, 16 ;					; 0 y6 0 y4

	movq   [OUT], mm1 ;	; 1 ; save y3 y2 y1 y0
	pslld mm7, 16 ;					; y7 0 y5 0

;	movq mm1,   [INP+8] ;	; row+1;  1 ; x7 x6 x5 x4
	por mm7, mm4 ;					; 4 ; y7 y6 y5 y4

	movq mm3,   [TABLE] ;	; 3 ; w06 w04 w02 w00
;	 punpcklwd mm0, mm1 ;			; row+1;  x5 x1 x4 x0

   ; begin processing row 1
	movq   [OUT+8], mm7 ;	; 7 ; save y7 y6 y5 y4
	add edi, 0x01;

	add OUT, 16;					; increment OUTPUT pointer -> row 1
	cmp edi, 0x08;
	jl near lpa;		; end for ( x = 0; x < 8; ++x )  

	; done with the iDCT row-transformation

	; now we have to transpose the output 8x8 matrix
	; 8x8 (OUT) -> 8x8't' (IN)
	; the transposition is implemented as 4 sub-operations.
	; 1) transpose upper-left quad
	; 2) transpose lower-right quad
	; 3) transpose lower-left quad
	; 4) transpose upper-right quad

 
	; mm0 = 1st row [ A B C D ] row1
	; mm1 = 2nd row [ E F G H ] 2
	; mm2 = 3rd row [ I J K L ] 3
	; mm3 = 4th row [ M N O P ] 4

			; 1) transpose upper-left quad
;	lea OUT,   [qwTemp];
	mov OUT,    [ebp+12];

	movq mm0,   [OUT + ROW_STRIDE * 0 ]

	movq mm1,   [OUT + ROW_STRIDE * 1 ]
	movq mm4, mm0;	; mm4 = copy of row1[A B C D]
	
	movq mm2,   [OUT + ROW_STRIDE * 2 ]
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	
	movq mm3,   [OUT + ROW_STRIDE * 3]
	punpckhwd mm4, mm1; ; mm4 = [ 2 6 3 7]

	movq mm6, mm2;
	punpcklwd mm2, mm3;	; mm2 = [ 8 12 9 13]

	punpckhwd mm6, mm3;	; mm6 = 10 14 11 15]
	movq mm1, mm0;	; mm1 = [ 0 4 1 5]

	mov INP,   [ebp+8];	; load input address
	punpckldq mm0, mm2;	; final result mm0 = row1 [0 4 8 12]

	movq mm3, mm4;	; mm3 = [ 2 6 3 7]
	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]

	movq   [ INP + ROW_STRIDE * 0 ], mm0; ; store row 1
	punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

		; begin reading next quadrant (lower-right)
	movq mm0,   [OUT + ROW_STRIDE*4 + 8]; 
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]

	movq   [ INP +ROW_STRIDE * 2], mm4; ; store row 3
	movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq   [ INP +ROW_STRIDE * 1], mm1; ; store row 2

	movq mm1,   [OUT + ROW_STRIDE*5 + 8]

	movq   [ INP +ROW_STRIDE * 3], mm3; ; store row 4
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]

			; 2) transpose lower-right quadrant

;	movq mm0,   [OUT + ROW_STRIDE*4 + 8]

;	movq mm1,   [OUT + ROW_STRIDE*5 + 8]
;	 movq mm4, mm0;	; mm4 = copy of row1[A B C D]
	
	movq mm2,   [OUT + ROW_STRIDE*6 + 8]
;	 punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	punpckhwd mm4, mm1; ; mm4 = [ 2 6 3 7]
	
	movq mm3,   [OUT + ROW_STRIDE*7 + 8]
	movq mm6, mm2;

	punpcklwd mm2, mm3;	; mm2 = [ 8 12 9 13]
	movq mm1, mm0;	; mm1 = [ 0 4 1 5]

	punpckhwd mm6, mm3;	; mm6 = 10 14 11 15]
	movq mm3, mm4;	; mm3 = [ 2 6 3 7]

	punpckldq mm0, mm2;	; final result mm0 = row1 [0 4 8 12]

	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]
	; ; slot

	movq   [ INP + ROW_STRIDE*4 + 8], mm0; ; store row 1
	 punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

	movq mm0,   [OUT + ROW_STRIDE * 4 ]
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]
	movq   [ INP +ROW_STRIDE*6 + 8], mm4; ; store row 3
	 movq mm4, mm0;	; mm4 = copy of row1[A B C D]
	movq   [ INP +ROW_STRIDE*5 + 8], mm1; ; store row 2
	 ; ;  slot
	movq mm1,   [OUT + ROW_STRIDE * 5 ]
	 ; ;  slot

	movq   [ INP +ROW_STRIDE*7 + 8], mm3; ; store row 4
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]

  ; 3) transpose lower-left
;	movq mm0,   [OUT + ROW_STRIDE * 4 ]

;	movq mm1,   [OUT + ROW_STRIDE * 5 ]
;	 movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq mm2,   [OUT + ROW_STRIDE * 6 ]
;	 punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	punpckhwd mm4, mm1; ; mm4 = [ 2 6 3 7]

	movq mm3,   [OUT + ROW_STRIDE * 7 ]
	movq mm6, mm2;

	punpcklwd mm2, mm3;	; mm2 = [ 8 12 9 13]
	movq mm1, mm0;	; mm1 = [ 0 4 1 5]

	punpckhwd mm6, mm3;	; mm6 = 10 14 11 15]
	 movq mm3, mm4;	; mm3 = [ 2 6 3 7]

	punpckldq mm0, mm2;	; final result mm0 = row1 [0 4 8 12]

	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]
	 ; ; slot

	movq   [ INP + ROW_STRIDE * 0 + 8 ], mm0; ; store row 1
	 punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

; begin reading next quadrant (upper-right)
	movq mm0,   [OUT + ROW_STRIDE*0 + 8];
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]

	movq   [ INP +ROW_STRIDE * 2 + 8], mm4; ; store row 3
	movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq   [ INP +ROW_STRIDE * 1 + 8 ], mm1; ; store row 2
	movq mm1,   [OUT + ROW_STRIDE*1 + 8]

	movq   [ INP +ROW_STRIDE * 3 + 8], mm3; ; store row 4
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]


	; 2) transpose lower-right quadrant

;	movq mm0,   [OUT + ROW_STRIDE*4 + 8]

;	movq mm1,   [OUT + ROW_STRIDE*5 + 8]
;	 movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq mm2,   [OUT + ROW_STRIDE*2 + 8]
;	 punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	 punpckhwd mm4, mm1; ; mm4 = [ 2 6 3 7]

	movq mm3,   [OUT + ROW_STRIDE*3 + 8]
	movq mm6, mm2;

	punpcklwd mm2, mm3;	; mm2 = [ 8 12 9 13]
	movq mm1, mm0;	; mm1 = [ 0 4 1 5]

	punpckhwd mm6, mm3;	; mm6 = 10 14 11 15]
	movq mm3, mm4;	; mm3 = [ 2 6 3 7]

	punpckldq mm0, mm2;	; final result mm0 = row1 [0 4 8 12]

	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]
	; ; slot

	movq   [ INP + ROW_STRIDE*4 ], mm0; ; store row 1
	punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

	movq   [ INP +ROW_STRIDE*5 ], mm1; ; store row 2
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]

	movq   [ INP +ROW_STRIDE*6 ], mm4; ; store row 3
	 ; ;  slot

	movq   [ INP +ROW_STRIDE*7 ], mm3; ; store row 4

	; Conceptually this is the column transform.
	; Actually, the matrix is transformed
	; row by row.  This function is identical to idct_mmx32_rows(),
	; except for the SHIFT amount and ROUND_INV amount.

	; this subroutine performs two operations
	; 1) iDCT row transform
	;		for( i = 0; i < 8; ++ i)
	;			DCT_8_INV_ROW_1( blk[i*8], qwTemp[i] );
	;
	; 2) transpose the matrix (which was stored in qwTemp[])
	;        qwTemp[] -> [8x8 matrix transpose] -> blk[]


	mov INP,   [ebp+8];		; ; row 0
	mov edi, 0x00;	; x = 0
 
	lea TABLE,   [idct_tab_01234567]; ; row 0
;	lea OUT,   [qwTemp];
	mov OUT,    [ebp+12];
;	 mov OUT, INP;	; algorithm writes data in-place  -> row 0

	lea round_inv_col,   [idct_r_inv_col]
    jmp acc_idct_colloop1
			
	; for ( x = 0; x < 8; ++x )  ; transform one row per iteration
align 32		
acc_idct_colloop1:

	movq mm0,   [INP] ;		; 0 ; x3 x2 x1 x0

	movq mm1,   [INP+8] ;	; 1 ; x7 x6 x5 x4
	movq mm2, mm0 ;				; 2 ; x3 x2 x1 x0

	movq mm3,   [TABLE] ;	; 3 ; w06 w04 w02 w00
	punpcklwd mm0, mm1 ;			; x5 x1 x4 x0

; ----------
	movq mm5, mm0 ;					; 5 ; x5 x1 x4 x0
	punpckldq mm0, mm0 ;			; x4 x0 x4 x0

	movq mm4,   [TABLE+8] ;	; 4 ; w07 w05 w03 w01
	punpckhwd mm2, mm1 ;			; 1 ; x7 x3 x6 x2

	pmaddwd mm3, mm0 ;				; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 ;				; 6 ; x7 x3 x6 x2

	movq mm1,   [TABLE+32] ;; 1 ; w22 w20 w18 w16
	punpckldq mm2, mm2 ;			; x6 x2 x6 x2

	pmaddwd mm4, mm2 ;				; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 ;			; x5 x1 x5 x1

	pmaddwd mm0,   [TABLE+16] ;; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 ;			; x7 x3 x7 x3

	movq mm7,   [TABLE+40] ;; 7 ; w23 w21 w19 w17
	pmaddwd mm1, mm5 ;				; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3,   [round_inv_col] ;; +rounder
	pmaddwd mm7, mm6 ;				; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2,   [TABLE+24] ;; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 ;				; 4 ; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5,   [TABLE+48] ;; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 ;				; 4 ; a1 a0

	pmaddwd mm6,   [TABLE+56] ;; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 ;				; 7 ; b1=sum(odd1) b0=sum(odd0)

	paddd mm0,   [round_inv_col] ;; +rounder
	psubd mm3, mm1 ;				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_COL;		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 ;				; 4 ; a1+b1 a0+b0

	paddd mm0, mm2 ;				; 2 ; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_COL;		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 ;				; 6 ; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 ;				; 4 ; a3 a2

	paddd mm0, mm5 ;				; a3+b3 a2+b2
	psubd mm4, mm5 ;				; 5 ; a3-b3 a2-b2

	add INP, 16;					; increment INPUT pointer -> row 1
	psrad mm4, SHIFT_INV_COL;		; y4=a3-b3 y5=a2-b2

	add TABLE, 0;					;  TABLE += 64 -> row 1
	psrad mm0, SHIFT_INV_COL;		; y3=a3+b3 y2=a2+b2

;	movq mm2,   [INP] ;		; row+1; 0;  x3 x2 x1 x0
	packssdw mm4, mm3 ;				; 3 ; y6 y7 y4 y5

	packssdw mm1, mm0 ;				; 0 ; y3 y2 y1 y0
	movq mm7, mm4 ;				; 7 ; y6 y7 y4 y5

;	movq mm0, mm2 ;					; row+1;  2 ; x3 x2 x1 x0
; 	por mm1,   dct_one_corr ;	; correction y2 +0.5
	psrld mm4, 16 ;					; 0 y6 0 y4

	movq   [OUT], mm1 ;	; 1 ; save y3 y2 y1 y0
	pslld mm7, 16 ;					; y7 0 y5 0

;	movq mm1,   [INP+8] ;	; row+1;  1 ; x7 x6 x5 x4
; 	por mm7,   dct_one_corr ;	; correction y2 +0.5
	por mm7, mm4 ;					; 4 ; y7 y6 y5 y4

;	movq mm3,   [TABLE] ;	; 3 ; w06 w04 w02 w00
;	 punpcklwd mm0, mm1 ;			; row+1;  x5 x1 x4 x0

   ; begin processing row 1
	movq   [OUT+8], mm7 ;	; 7 ; save y7 y6 y5 y4
	add edi, 0x01;

	add OUT, 16;
	cmp edi, 0x08;	; compare x <> 8

	jl near  acc_idct_colloop1;	; end for ( x = 0; x < 8; ++x )  

	; done with the iDCT column-transformation

		; now we have to transpose the output 8x8 matrix
		; 8x8 (OUT) -> 8x8't' (IN)

		; the transposition is implemented as 4 sub-operations.
	; 1) transpose upper-left quad
	; 2) transpose lower-right quad
	; 3) transpose lower-left quad
	; 4) transpose upper-right quad


 
	; mm0 = 1st row [ A B C D ] row1
	; mm1 = 2nd row [ E F G H ] 2
	; mm2 = 3rd row [ I J K L ] 3
	; mm3 = 4th row [ M N O P ] 4

	; 1) transpose upper-left quad
;	lea OUT,   [qwTemp];
	mov OUT,    [ebp+12];

	movq mm0,   [OUT + ROW_STRIDE * 0 ]

	movq mm1,   [OUT + ROW_STRIDE * 1 ]
	movq mm4, mm0;	; mm4 = copy of row1[A B C D]
	
	movq mm2,   [OUT + ROW_STRIDE * 2 ]
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	
	movq mm3,   [OUT + ROW_STRIDE * 3]
	punpckhwd mm4, mm1 ; mm4 = [ 2 6 3 7]

	movq mm6, mm2
	punpcklwd mm2, mm3	; mm2 = [ 8 12 9 13]

	punpckhwd mm6, mm3	; mm6 = 10 14 11 15]
	movq mm1, mm0	; mm1 = [ 0 4 1 5]

	mov INP,   [ebp+8]	; load input address
	punpckldq mm0, mm2	; final result mm0 = row1 [0 4 8 12]

	movq mm3, mm4;	; mm3 = [ 2 6 3 7]
	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]

	movq   [ INP + ROW_STRIDE * 0 ], mm0; ; store row 1
	punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

; begin reading next quadrant (lower-right)
	movq mm0,   [OUT + ROW_STRIDE*4 + 8]; 
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]

	movq   [ INP +ROW_STRIDE * 2], mm4; ; store row 3
	movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq   [ INP +ROW_STRIDE * 1], mm1; ; store row 2

	movq mm1,   [OUT + ROW_STRIDE*5 + 8]

	movq   [ INP +ROW_STRIDE * 3], mm3; ; store row 4
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]

	; 2) transpose lower-right quadrant

;	movq mm0,   [OUT + ROW_STRIDE*4 + 8]

;	movq mm1,   [OUT + ROW_STRIDE*5 + 8]
;	 movq mm4, mm0;	; mm4 = copy of row1[A B C D]
	
	movq mm2,   [OUT + ROW_STRIDE*6 + 8]
;	 punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	 punpckhwd mm4, mm1; ; mm4 = [ 2 6 3 7]
	
	movq mm3,   [OUT + ROW_STRIDE*7 + 8]
	movq mm6, mm2;

	punpcklwd mm2, mm3;	; mm2 = [ 8 12 9 13]
	movq mm1, mm0;	; mm1 = [ 0 4 1 5]

	punpckhwd mm6, mm3;	; mm6 = 10 14 11 15]
	movq mm3, mm4;	; mm3 = [ 2 6 3 7]

	punpckldq mm0, mm2;	; final result mm0 = row1 [0 4 8 12]

	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]
	; ;  slot

	movq   [ INP + ROW_STRIDE*4 + 8], mm0; ; store row 1
	punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

	movq mm0,   [OUT + ROW_STRIDE * 4 ]
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]
	movq   [ INP +ROW_STRIDE*6 + 8], mm4; ; store row 3
	movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq   [ INP +ROW_STRIDE*5 + 8], mm1; ; store row 2
	 ; ;  slot
	movq mm1,   [OUT + ROW_STRIDE * 5 ]
	 ; ;  slot

	movq   [ INP +ROW_STRIDE*7 + 8], mm3; ; store row 4
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]

  ; 3) transpose lower-left
;	movq mm0,   [OUT + ROW_STRIDE * 4 ]

;	movq mm1,   [OUT + ROW_STRIDE * 5 ]
;	 movq mm4, mm0;	; mm4 = copy of row1[A B C D]
	
	movq mm2,   [OUT + ROW_STRIDE * 6 ]
;	 punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	punpckhwd mm4, mm1; ; mm4 = [ 2 6 3 7]
	
	movq mm3,   [OUT + ROW_STRIDE * 7 ]
	movq mm6, mm2;

	punpcklwd mm2, mm3;	; mm2 = [ 8 12 9 13]
	movq mm1, mm0;	; mm1 = [ 0 4 1 5]

	punpckhwd mm6, mm3;	; mm6 = 10 14 11 15]
	movq mm3, mm4;	; mm3 = [ 2 6 3 7]

	punpckldq mm0, mm2;	; final result mm0 = row1 [0 4 8 12]

	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]
	 ; ; slot

	movq   [ INP + ROW_STRIDE * 0 + 8 ], mm0; ; store row 1
	punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

; begin reading next quadrant (upper-right)
	movq mm0,   [OUT + ROW_STRIDE*0 + 8];
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]

	movq   [ INP +ROW_STRIDE * 2 + 8], mm4; ; store row 3
	movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq   [ INP +ROW_STRIDE * 1 + 8 ], mm1; ; store row 2
	movq mm1,   [OUT + ROW_STRIDE*1 + 8]

	movq   [ INP +ROW_STRIDE * 3 + 8], mm3; ; store row 4
	punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]


	; 2) transpose lower-right quadrant

;	movq mm0,   [OUT + ROW_STRIDE*4 + 8]

;	movq mm1,   [OUT + ROW_STRIDE*5 + 8]
;	 movq mm4, mm0;	; mm4 = copy of row1[A B C D]

	movq mm2,   [OUT + ROW_STRIDE*2 + 8]
;	 punpcklwd mm0, mm1; ; mm0 = [ 0 4 1 5]
	punpckhwd mm4, mm1; ; mm4 = [ 2 6 3 7]

	movq mm3,   [OUT + ROW_STRIDE*3 + 8]
	movq mm6, mm2;

	punpcklwd mm2, mm3;	; mm2 = [ 8 12 9 13]
	movq mm1, mm0;	; mm1 = [ 0 4 1 5]

	punpckhwd mm6, mm3;	; mm6 = 10 14 11 15]
	movq mm3, mm4;	; mm3 = [ 2 6 3 7]

	punpckldq mm0, mm2;	; final result mm0 = row1 [0 4 8 12]

	punpckhdq mm1, mm2; ; mm1 = final result mm1 = row2 [1 5 9 13]
	; ;  slot

	movq  [ INP + ROW_STRIDE*4 ], mm0; ; store row 1
	punpckldq mm4, mm6; ; final result mm4 = row3 [2 6 10 14]

	movq  [ INP +ROW_STRIDE*5 ], mm1; ; store row 2
	punpckhdq mm3, mm6; ; final result mm3 = row4 [3 7 11 15]

	movq  [ INP +ROW_STRIDE*6 ], mm4; ; store row 3
	 ; ;  slot

	movq  [ INP +ROW_STRIDE*7 ], mm3; ; store row 4

	pop edi
	pop edx
	pop ecx
	pop ebx

	pop ebp			; restore frame pointer

	emms
	ret
