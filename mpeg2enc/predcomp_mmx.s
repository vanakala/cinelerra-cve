;;; 
;;;  predcomp_00_mmx.s:
;;;
;;; 		  Extended MMX prediction composition
;;;  routines handling the four different interpolation cases...
;;; 
;;;  Copyright (C) 2000 Andrew Stevens <as@comlab.ox.ac.uk>

;;;
;;;  This program is free software; you can reaxstribute it and/or
;;;  modify it under the terms of the GNU General Public License
;;;  as published by the Free Software Foundation; either version 2
;;;  of the License, or (at your option) any later version.
;;;
;;;  This program is distributed in the hope that it will be useful,
;;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;  GNU General Public License for more details.
;;;
;;;  You should have received a copy of the GNU General Public License
;;;  along with this program; if not, write to the Free Software
;;;  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
;;;  02111-1307, USA.
;;;
;;;
;;;

;;; The no interpolation case...

global predcomp_00_mmx

;;; void predcomp_<ix><iy>_mmx(char *src,char *dst,int lx, int w, int h, int addflag);

;;; ix - Interpolation in x iy - Interpolation in y
		
;;; eax = pdst
;;; ebx = psrc
;;; ecx = h left
;;; edx = lx;
;;; edi = w (8 or 16)


;;; mm1 = one's mask for src
;;; mm0 = zero mask for src...
		


align 32
predcomp_00_mmx:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi
	push esi
		
	mov	eax, 0x00010001
	movd  mm1, eax
	punpckldq mm1,mm1

	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	mov esi, [ebp+28]			; get addflag
		;; Extend addflag into bit-mask
	pxor mm0, mm0
	jmp predrow00m		; align for speed
align 32
predrow00m:
	movq mm4, [ebx]		; first 8 bytes of row
	cmp esi, 0
	jz	noadd00					

	movq mm5, mm4
	punpcklbw	mm4, mm0
	punpckhbw	mm5, mm0

	movq mm2, [eax]
	movq mm3, mm2
	punpcklbw	mm2, mm0
	punpckhbw	mm3, mm0
	paddw		mm4, mm2
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
	packuswb	mm4, mm5
noadd00:		
	movq		[eax], mm4
						
	cmp	edi, 8
	jz  eightwide00

	movq mm4, [ebx+8]		; first 8 bytes of row
	cmp esi, 0
	jz	noadd00w
		
	movq mm5, mm4
	punpcklbw	mm4, mm0
	punpckhbw	mm5, mm0
							
	movq mm2, [eax+8]
	movq mm3, mm2
	punpcklbw	mm2, mm0
	punpckhbw	mm3, mm0
	paddw		mm4, mm2
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
	packuswb	mm4, mm5					
noadd00w:		
	movq	[eax+8], mm4

eightwide00:	
	add eax, edx		; update pointer to next row
	add ebx, edx		; ditto
	
	sub  ecx, 1					; check h left
	jnz near predrow00m

	pop esi
	pop edi	
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret	


;;; The x-axis interpolation case...

global predcomp_10_mmx


align 32
predcomp_10_mmx:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi
	push esi
		
	mov	eax, 0x00010001
	movd  mm1, eax
	punpckldq mm1,mm1

	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	mov esi, [ebp+28]			; get addflag
		;; Extend addflag into bit-mask
	pxor mm0, mm0
	jmp predrow10m		; align for speed
align 32
predrow10m:
	movq mm4, [ebx]		; first 8 bytes of row
	movq mm5, mm4
	punpcklbw	mm4, mm0
	punpckhbw	mm5, mm0
	movq mm2, [ebx+1]
	movq mm3, mm2
	punpcklbw	mm2, mm0
	punpckhbw	mm3, mm0
		
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
		
	cmp esi, 0
	jz	noadd10					

	movq mm2, [eax]				;  Add 
	movq mm3, mm2
	punpcklbw	mm2, mm0	
	punpckhbw	mm3, mm0
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
noadd10:		
	packuswb	mm4, mm5
	movq		[eax], mm4

	cmp	edi, 8
	jz  eightwide10
		
	movq mm4, [ebx+8]		; first 8 bytes of row
	movq mm5, mm4
	punpcklbw	mm4, mm0
	punpckhbw	mm5, mm0
	movq mm2, [ebx+9]
	movq mm3, mm2
	punpcklbw	mm2, mm0
	punpckhbw	mm3, mm0
		
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
		
	cmp esi, 0
	jz	noadd10w					

	movq mm2, [eax+8]				;  Add 
	movq mm3, mm2
	punpcklbw	mm2, mm0	
	punpckhbw	mm3, mm0
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
noadd10w:		
	packuswb	mm4, mm5
	movq		[eax+8], mm4		


eightwide10:	
	add eax, edx		; update pointer to next row
	add ebx, edx		; ditto
	
	sub  ecx, 1					; check h left
	jnz near predrow10m

	pop esi
	pop edi	
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret	

;;; The y-axis interpolation case...

global predcomp_01_mmx


align 32
predcomp_01_mmx:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi
	push esi
		
	mov	eax, 0x00010001
	movd  mm1, eax
	punpckldq mm1,mm1

	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	mov esi, [ebp+28]			; get addflag
	pxor mm0, mm0
	jmp predrow01m		; align for speed

align 32
predrow01m:
	movq mm4, [ebx]		; first 8 bytes of row
	movq mm5, mm4
	add			ebx, edx				; Next row
	punpcklbw	mm4, mm0
	punpckhbw	mm5, mm0

	movq mm2, [ebx]	
	movq mm3, mm2
	punpcklbw	mm2, mm0
	punpckhbw	mm3, mm0
		
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
		
	cmp esi, 0
	jz	noadd01

	movq mm2, [eax]				;  Add 
	movq mm3, mm2
	punpcklbw	mm2, mm0	
	punpckhbw	mm3, mm0
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
noadd01:		
	packuswb	mm4, mm5
	movq		[eax], mm4

	cmp	edi, 8
	jz  eightwide01

	sub			ebx, edx		;  Back to first row...
	movq mm4, [ebx+8]		; first 8 bytes of row
	movq mm5, mm4
	add			ebx, edx				; Next row
	punpcklbw	mm4, mm0
	punpckhbw	mm5, mm0
	movq mm2, [ebx+8]
	movq mm3, mm2
	punpcklbw	mm2, mm0
	punpckhbw	mm3, mm0
		
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
		
	cmp esi, 0
	jz	noadd01w					

	movq mm2, [eax+8]				;  Add 
	movq mm3, mm2
	punpcklbw	mm2, mm0	
	punpckhbw	mm3, mm0
	paddw		mm4, mm2		;  Average mm4/mm5 and mm2/mm3
	paddw		mm5, mm3
	paddw		mm4, mm1
	paddw		mm5, mm1
	psrlw		mm4, 1
	psrlw		mm5, 1
noadd01w:		
	packuswb	mm4, mm5
	movq		[eax+8], mm4		


eightwide01:	
	add eax, edx		; ditto
	
	sub  ecx, 1					; check h left
	jnz near predrow01m

	pop esi
	pop edi	
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret	

		
;;; The x-axis and y-axis interpolation case...
		
global predcomp_11_mmx

;;; mm0 = [0,0,0,0]W
;;; mm1 = [1,1,1,1]W		
;;; mm2 = [2,2,2,2]W
align 32
predcomp_11_mmx:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi
	push esi
		
	mov	eax, 0x00020002
	movd  mm2, eax
	punpckldq mm2,mm2
	mov	eax, 0x00010001
	movd  mm1, eax
	punpckldq mm1,mm1
	pxor mm0, mm0
						
	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	mov esi, [ebp+28]			;  Addflags
		;; Extend addflag into bit-mask

	
	jmp predrow11		; align for speed
align 32
predrow11:
	movq mm4, [ebx]		; mm4 and mm6 accumulate partial sums for interp.
	movq mm6, mm4
	punpcklbw	mm4, mm0
	punpckhbw	mm6, mm0

	movq mm5, [ebx+1]	
	movq mm7, mm5
	punpcklbw	mm5, mm0
	paddw		mm4, mm5
	punpckhbw	mm7, mm0
	paddw		mm6, mm7

	add ebx, edx		; update pointer to next row
		
	movq mm5, [ebx]		; first 8 bytes 1st row:	 avg src in x
	movq mm7, mm5
	punpcklbw	mm5, mm0		;  Accumulate partial interpolation
	paddw		mm4, mm5
	punpckhbw   mm7, mm0
	paddw		mm6, mm7

	movq mm5, [ebx+1]
	movq mm7, mm5		
	punpcklbw	mm5, mm0
	paddw		mm4, mm5
	punpckhbw	mm7, mm0
	paddw		mm6, mm7

		;; Now round 
	paddw		mm4, mm2
	paddw		mm6, mm2
	psrlw		mm4, 2
	psrlw		mm6, 2

	cmp esi, 0
	jz  noadd11

	movq mm5, [eax]				;  Add 
	movq mm7, mm5
	punpcklbw	mm5, mm0	
	punpckhbw	mm7, mm0
	paddw		mm4, mm5		;  Average mm4/mm6 and mm5/mm7
	paddw		mm6, mm7
	paddw		mm4, mm1
	paddw		mm6, mm1
	psrlw		mm4, 1
	psrlw		mm6, 1
		
noadd11:		
	packuswb	mm4, mm6
	movq [eax], mm4

	cmp   edi, 8
	jz near eightwide11

	sub ebx, edx		; Back to first row...

	movq mm4, [ebx+8]		; mm4 and mm6 accumulate partial sums for interp.
	movq mm6, mm4
	punpcklbw	mm4, mm0
	punpckhbw	mm6, mm0

	movq mm5, [ebx+9]	
	movq mm7, mm5
	punpcklbw	mm5, mm0
	paddw		mm4, mm5
	punpckhbw	mm7, mm0
	paddw		mm6, mm7

	add ebx, edx		; update pointer to next row
		
	movq mm5, [ebx+8]		; first 8 bytes 1st row:	 avg src in x
	movq mm7, mm5
	punpcklbw	mm5, mm0		;  Accumulate partial interpolation
	paddw		mm4, mm5
	punpckhbw   mm7, mm0
	paddw		mm6, mm7

	movq mm5, [ebx+9]
	movq mm7, mm5		
	punpcklbw	mm5, mm0
	paddw		mm4, mm5
	punpckhbw	mm7, mm0
	paddw		mm6, mm7

		;; Now round 
	paddw		mm4, mm2
	paddw		mm6, mm2
	psrlw		mm4, 2
	psrlw		mm6, 2

	cmp esi, 0
	jz  noadd11w

	movq mm5, [eax+8]				;  Add and average
	movq mm7, mm5
	punpcklbw	mm5, mm0	
	punpckhbw	mm7, mm0
	paddw		mm4, mm5		;  Average mm4/mm6 and mm5/mm7
	paddw		mm6, mm7
	paddw		mm4, mm1
	paddw		mm6, mm1
	psrlw		mm4, 1
	psrlw		mm6, 1
noadd11w:		
	packuswb	mm4, mm6
	movq [eax+8], mm4

eightwide11:	
	add eax, edx		; update pointer to next row

		
	sub  ecx, 1			; check h left
	jnz near predrow11

	pop esi
	pop edi		
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret



