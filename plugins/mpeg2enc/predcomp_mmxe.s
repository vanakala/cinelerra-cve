;;; 
;;;  predcomp_00_mmxe.s:
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

global predcomp_00_mmxe

;;; void predcomp_<ix><iy>_mmxe(char *src,char *dst,int lx, int w, int h, int mask);

;;; ix - Interpolation in x iy - Interpolation in y
		
;;; eax = pdst
;;; ebx = psrc
;;; ecx = h left
;;; edx = lx;
;;; edi = w (8 or 16)


;;; mm1 = one's mask for src
;;; mm0 = zero mask for src...
		

align 32
predcomp_00_mmxe:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi

	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	movd mm0, [ebp+28]
		;; Extend addflag into bit-mask
	pxor mm2, mm2
	punpckldq	mm0,mm0
	pcmpeqd		mm0, mm2
	movq		mm1, mm0
	pcmpeqd		mm0, mm2

	jmp predrow00		; align for speed
align 32
predrow00:
	movq mm4, [ebx]		; first 8 bytes of row
	movq mm2, [eax]
	pand mm2, mm0
	movq mm3, mm4
	pand mm3, mm1
	por  mm2, mm3
	pavgb mm4, mm2		; 
	movq [eax], mm4		; 

			
	cmp	edi, 8
	jz  eightwide00
		
	movq mm4, [ebx+8]		; first 8 bytes of row
	movq mm2, [eax+8]
	pand mm2, mm0
	movq mm3, mm4
	pand mm3, mm1
	por  mm2, mm3
	pavgb mm4, mm2		; 
	movq [eax+8], mm4		; 

		
eightwide00:	
	add eax, edx		; update pointer to next row
	add ebx, edx		; ditto
	
	sub  ecx, 1					; check h left
	jnz predrow00

	pop edi	
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret	


;;; The x-axis interpolation case...

global predcomp_10_mmxe


align 32
predcomp_10_mmxe:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi
		
	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	movd mm0, [ebp+28]
		;; Extend addflag into bit-mask
	pxor mm2,mm2		
	punpckldq	mm0,mm0
	pcmpeqd		mm0, mm2
	movq		mm1, mm0
	pcmpeqd		mm0, mm2

	jmp predrow10		; align for speed
align 32
predrow10:
	movq mm4, [ebx]		; first 8 bytes row:	 avg src in x
	pavgb mm4, [ebx+1]
	movq mm2,  [eax]
	pand mm2, mm0
	movq mm3, mm4
	pand mm3, mm1
	por  mm2, mm3
	pavgb mm4, mm2	; combine
	movq [eax], mm4

    cmp  edi, 8
	jz eightwide10


	movq mm4, [ebx+8]		; 2nd 8 bytes row:	 avg src in x
	pavgb mm4, [ebx+9]
	movq mm2,  [eax+8]
	pand mm2, mm0
	movq mm3, mm4
	pand mm3, mm1
	por  mm2, mm3
	pavgb mm4, mm2	; combine
	movq [eax+8], mm4

		
eightwide10:			
	add eax, edx		; update pointer to next row
	add ebx, edx		; ditto
	

	sub  ecx, 1			; check h left
	jnz near predrow10

	pop edi		
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret


;;; The x-axis and y-axis interpolation case...
		
global predcomp_11_mmxe

;;; mm2 = [0,0,0,0]W
;;; mm3 = [2,2,2,2]W
align 32
predcomp_11_mmxe:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi

	mov	eax, 0x00020002
	movd  mm3, eax
	punpckldq mm3,mm3
	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	movd mm0, [ebp+28]
		;; Extend addflag into bit-mask

	pxor mm2,mm2
	punpckldq	mm0, mm0
	pcmpeqd		mm0, mm2
	movq		mm1, mm0
	pcmpeqd		mm0, mm2
	
	jmp predrow11		; align for speed
align 32
predrow11:
	movq mm4, [ebx]		; mm4 and mm6 accumulate partial sums for interp.
	movq mm6, mm4
	punpcklbw	mm4, mm2
	punpckhbw	mm6, mm2

	movq mm5, [ebx+1]	
	movq mm7, mm5
	punpcklbw	mm5, mm2
	paddw		mm4, mm5
	punpckhbw	mm7, mm2
	paddw		mm6, mm7

	add ebx, edx		; update pointer to next row
		
	movq mm5, [ebx]		; first 8 bytes 1st row:	 avg src in x
	movq mm7, mm5
	punpcklbw	mm5, mm2		;  Accumulate partial interpolation
	paddw		mm4, mm5
	punpckhbw   mm7, mm2
	paddw		mm6, mm7

	movq mm5, [ebx+1]
	movq mm7, mm5		
	punpcklbw	mm5, mm2
	paddw		mm4, mm5
	punpckhbw	mm7, mm2
	paddw		mm6, mm7

		;; Now round and repack...
	paddw		mm4, mm3
	paddw		mm6, mm3
	psrlw		mm4, 2
	psrlw		mm6, 2
	packuswb	mm4, mm6

	movq  mm7, [eax]
	pand mm7, mm0
	movq mm6, mm4
	pand mm6, mm1
	por  mm7, mm6
	pavgb mm4, mm7
	movq [eax], mm4

	cmp   edi, 8
	jz eightwide11
		
	sub ebx, edx		 		;  Back to 1st row

	movq mm4, [ebx+8]		; mm4 and mm6 accumulate partial sums for interp.
	movq mm6, mm4
	punpcklbw	mm4, mm2
	punpckhbw	mm6, mm2

	movq mm5, [ebx+9]	
	movq mm7, mm5
	punpcklbw	mm5, mm2
	paddw		mm4, mm5
	punpckhbw	mm7, mm2
	paddw		mm6, mm7

	add ebx, edx		; update pointer to next row
		
	movq mm5, [ebx+8]		; first 8 bytes 1st row:	 avg src in x
	movq mm7, mm5
	punpcklbw	mm5, mm2		;  Accumulate partial interpolation
	paddw		mm4, mm5
	punpckhbw   mm7, mm2
	paddw		mm6, mm7

	movq mm5, [ebx+9]	
	movq mm7, mm5
	punpcklbw	mm5, mm2
	paddw		mm4, mm5
	punpckhbw	mm7, mm2
	paddw		mm6, mm7

		;; Now round and repack...
	paddw		mm4, mm3
	paddw		mm6, mm3
	psraw		mm4, 2
	psraw		mm6, 2
	packuswb	mm4, mm6
		
	movq  mm7, [eax+8]
	pand mm7, mm0
	movq mm6, mm4
	pand mm6, mm1
	por  mm7, mm6
	pavgb mm4, mm7
	movq [eax+8], mm4

eightwide11:	
	add eax, edx		; update pointer to next row

		
	sub  ecx, 1			; check h left
	jnz near predrow11

	pop edi		
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret



;;; The  y-axis interpolation case...
		
global predcomp_01_mmxe

align 32
predcomp_01_mmxe:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push eax
	push ebx
	push ecx
	push edx
	push edi

	mov ebx, [ebp+8]	; get psrc
	mov eax, [ebp+12]	; get pdst
	mov edx, [ebp+16]	; get lx
	mov edi, [ebp+20]	;  get w
	mov ecx, [ebp+24]	; get h
	movd mm0, [ebp+28]
		;; Extend addflag into bit-mask
	pxor		mm2, mm2
	punpckldq	mm0,mm0
	pcmpeqd		mm0, mm2
	movq		mm1, mm0
	pcmpeqd		mm0, mm2

	jmp predrow01		; align for speed
align 32
predrow01:
	movq mm4, [ebx]		; first 8 bytes row
	add ebx, edx		; update pointer to next row
	pavgb mm4, [ebx]			;  Average in y
	
	movq mm2, [eax]
	pand mm2, mm0
	movq mm3, mm4
	pand mm3, mm1
	por  mm2, mm3
	pavgb mm4, mm2
	movq [eax], mm4

	cmp	edi, 8
	jz eightwide01

	sub	ebx, edx				; Back to prev row
	movq mm4, [ebx+8]			; first 8 bytes row
	add ebx, edx				; update pointer to next row
	pavgb mm4, [ebx+8]			;  Average in y
	
	movq mm2, [eax+8]
	pand mm2, mm0
	movq mm3, mm4
	pand mm3, mm1
	por  mm2, mm3
	pavgb mm4, mm2
	movq [eax+8], mm4

eightwide01:					
	add eax, edx		; update pointer to next row


	sub  ecx, 1			; check h left
	jnz predrow01

	pop edi		
	pop edx	
	pop ecx	
	pop ebx	
	pop eax	
	pop ebp	
	emms
	ret
