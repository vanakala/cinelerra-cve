;;; 
;;;  mblockq_sad_mmxe.s:  
;;; 
;;; Enhanced MMX optimized Sum Absolute Differences routines for macroblock 
;;; quads (2 by 2 squares of adjacent macroblocks)

;;; Explanation: the motion compensation search at 1-pel and 2*2 sub-sampled
;;; evaluates macroblock quads.  A lot of memory accesses can be saved
;;; if each quad is done together rather than each macroblock in the
;;; quad handled individually.

;;; TODO:		Really there ought to be MMX versions and the function's
;;; specification should be documented...
;
; Copyright (C) 2000 Andrew Stevens <as@comlab.ox.ac.uk>	


;
;  This program is free software; you can reaxstribute it and/or
;  modify it under the terms of the GNU General Public License
;  as published by the Free Software Foundation; either version 2
;  of the License, or (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
;
;

;;; CURRENTLY not used but used in testing as reference for tweaks...
global mblockq_sad1_REF

; void mblockq_dist1_REF(char *blk1,char *blk2,int lx,int h,int *weightvec);
; eax = p1
; ebx = p2
; ecx = unused
; edx = lx;
; edi = rowsleft
; esi = h
		
; mm0 = SAD (x+0,y+0)
; mm1 = SAD (x+2,y+0)
; mm2 = SAD (x+0,y+2)
; mm3 = SAD (x+2,y+2)
; mm4 = temp
; mm5 = temp
; mm6 = temp
; mm7 = temp						

align 32
mblockq_dist1_REF:
	push ebp					; save frame pointer
	mov ebp, esp				; link
	push eax
	push ebx
	push ecx
	push edx
	push edi
	push esi

	pxor mm0, mm0		; zero accumulators
	pxor mm1, mm1
	pxor mm2, mm2
	pxor mm3, mm3
	mov eax, [ebp+8]	; get p1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx
	
	mov edi, [ebp+20]	; get rowsleft
	mov esi, edi

	jmp nextrow_block_d1
align 32
nextrow_block_d1:		

		;; Do the (+0,+0) SAD
		
	movq mm4, [eax]		; load 1st 8 bytes of p1
	movq mm6, mm4
	movq mm5, [ebx]
	psadbw mm4, mm5	; compare to 1st 8 bytes of p2 
	paddd mm0, mm4		; accumulate difference
	movq mm4, [eax+8]	; load 2nd 8 bytes of p1
	movq mm7, mm4		
	psadbw mm4, [ebx+8]	; compare to 2nd 8 bytes of p2 
	paddd mm0, mm4		; accumulate difference

		
    cmp edi, esi
	jz  firstrow0

		;; Do the (0,+2) SAD
	sub ebx, edx
	psadbw mm6, [ebx]	; compare to next 8 bytes of p2 (row 1)
	paddd mm2, mm6		; accumulate difference
	psadbw mm7, [ebx+8]	;  next 8 bytes of p1 (row 1)
	add ebx, edx
	paddd mm2, mm7	

firstrow0:

		;; Do the (+2,0) SAD
	
	movq mm4, [eax+1]
				
	movq mm6, mm4
	psadbw mm4, mm5	; compare to 1st 8 bytes of p2
	paddd mm1, mm4		; accumulate difference
	movq mm4, [eax+9]
	movq mm7, mm4
	psadbw mm4, [ebx+8]	; compare to 2nd 8 bytes of p2
	paddd mm1, mm4		; accumulate difference

    cmp edi, esi
	jz  firstrow1

		;; Do the (+2, +2 ) SAD
	sub ebx, edx
	psadbw mm6, [ebx]	; compare to 1st 8 bytes of prev p2 
	psadbw mm7, [ebx+8]	;  2nd 8 bytes of prev p2
	add ebx, edx
	paddd mm3, mm6		; accumulate difference
	paddd mm3, mm7	
firstrow1:		

	add eax, edx				; update pointer to next row
	add ebx, edx		; ditto
		
	sub edi, 1
	jnz near nextrow_block_d1

		;; Do the last row of the (0,+2) SAD

	movq mm4, [eax]		; load 1st 8 bytes of p1
	movq mm5, [eax+8]	; load 2nd 8 bytes of p1
	sub  ebx, edx
	psadbw mm4, [ebx]	; compare to next 8 bytes of p2 (row 1)
	psadbw mm5, [ebx+8]	;  next 8 bytes of p1 (row 1)
	paddd mm2, mm4		; accumulate difference
	paddd mm2, mm5
		
	movq mm4, [eax+1]
	movq mm5, [eax+9]
		
		;; Do the last row of rhw (+2, +2) SAD
	psadbw mm4, [ebx]	; compare to 1st 8 bytes of prev p2 
	psadbw mm5, [ebx+8]	;  2nd 8 bytes of prev p2
	paddd mm3, mm4		; accumulate difference
	paddd mm3, mm5
		

	mov eax, [ebp+24]			; Weightvec
	movd [eax+0], mm0
	movd [eax+4], mm1
	movd [eax+8], mm2
	movd [eax+12], mm3
		
	pop esi
	pop edi
	pop edx	
	pop ecx	
	pop ebx	
	pop eax
		
	pop ebp	
	emms
	ret	



global mblockq_dist1_mmxe

; void mblockq_dist1_mmxe(char *blk1,char *blk2,int lx,int h,int *weightvec);

; eax = p1
; ebx = p2
; ecx = unused
; edx = lx;
; edi = rowsleft
; esi = h
		
; mm0 = SAD (x+0,y+0),SAD (x+0,y+2)
; mm1 = SAD (x+2,y+0),SAD (x+2,y+2)
		
; mm4 = temp
; mm5 = temp
; mm6 = temp
; mm7 = temp						

align 32
mblockq_dist1_mmxe:
	push ebp					; save frame pointer
	mov ebp, esp				; link
	push eax
	push ebx
	push ecx
	push edx
	push edi
	push esi

	mov eax, [ebp+8]	; get p1
	prefetcht0 [eax]
	pxor mm0, mm0		; zero accumulators
	pxor mm1, mm1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx
	
	mov edi, [ebp+20]	; get rowsleft
	mov esi, edi

	jmp nextrow_block_e1
align 32
nextrow_block_e1:		

		;; Do the (+0,+0) SAD
	prefetcht0 [eax+edx]		
	movq mm4, [eax]		; load 1st 8 bytes of p1
	movq mm6, mm4
	movq mm5, [ebx]
	psadbw mm4, mm5	; compare to 1st 8 bytes of p2 
	paddd mm0, mm4		; accumulate difference
	movq mm4, [eax+8]	; load 2nd 8 bytes of p1
	movq mm7, mm4		
	psadbw mm4, [ebx+8]	; compare to 2nd 8 bytes of p2 
	paddd mm0, mm4		; accumulate difference

		
    cmp edi, esi
	jz  firstrowe0

		;; Do the (0,+2) SAD
	sub ebx, edx
	pshufw  mm0, mm0, 2*1 + 3 * 4 + 0 * 16 + 1 * 64
	movq   mm2, [ebx]
	psadbw mm6, mm2	    ; compare to next 8 bytes of p2 (row 1)
	paddd mm0, mm6		; accumulate difference
	movq  mm3, [ebx+8]
	psadbw mm7, mm3	;  next 8 bytes of p1 (row 1)
	add ebx, edx
	paddd mm0, mm7	
	pshufw  mm0, mm0, 2*1 + 3 * 4 + 0 * 16 + 1 * 64 
firstrowe0:

		;; Do the (+2,0) SAD
	
	movq mm4, [eax+1]
	movq mm6, mm4

	psadbw mm4, mm5	; compare to 1st 8 bytes of p2
	paddd mm1, mm4		; accumulate difference

	movq mm4, [eax+9]
	movq mm7, mm4

	psadbw mm4, [ebx+8]	; compare to 2nd 8 bytes of p2
	paddd mm1, mm4		; accumulate difference

    cmp edi, esi
	jz  firstrowe1

		;; Do the (+2, +2 ) SAD
	sub ebx, edx
	pshufw  mm1, mm1, 2*1 + 3 * 4 + 0 * 16 + 1 * 64 
	psadbw mm6, mm2	; compare to 1st 8 bytes of prev p2 
	psadbw mm7, mm3	;  2nd 8 bytes of prev p2
	add ebx, edx
	paddd mm1, mm6		; accumulate difference
	paddd mm1, mm7
	pshufw  mm1, mm1, 2*1 + 3 * 4 + 0 * 16 + 1 * 64 
firstrowe1:		

	add eax, edx				; update pointer to next row
	add ebx, edx		; ditto
		
	sub edi, 1
	jnz near nextrow_block_e1

		;; Do the last row of the (0,+2) SAD
	pshufw  mm0, mm0, 2*1 + 3 * 4 + 0 * 16 + 1 * 64
	movq mm4, [eax]		; load 1st 8 bytes of p1
	movq mm5, [eax+8]	; load 2nd 8 bytes of p1
	sub  ebx, edx
	psadbw mm4, [ebx]	; compare to next 8 bytes of p2 (row 1)
	psadbw mm5, [ebx+8]	;  next 8 bytes of p1 (row 1)
	paddd mm0, mm4		; accumulate difference
	paddd mm0, mm5

		
		;; Do the last row of rhw (+2, +2) SAD
	pshufw  mm1, mm1, 2*1 + 3 * 4 + 0 * 16 + 1 * 64				
	movq mm4, [eax+1]
	movq mm5, [eax+9]

	psadbw mm4, [ebx]	; compare to 1st 8 bytes of prev p2 
	psadbw mm5, [ebx+8]	;  2nd 8 bytes of prev p2
	paddd mm1, mm4		; accumulate difference
	paddd mm1, mm5
		

	mov eax, [ebp+24]			; Weightvec
	movd [eax+8], mm0
	pshufw  mm0, mm0, 2*1 + 3 * 4 + 0 * 16 + 1 * 64
	movd [eax+12], mm1
	pshufw  mm1, mm1, 2*1 + 3 * 4 + 0 * 16 + 1 * 64
	movd [eax+0], mm0
	movd [eax+4], mm1
		
	pop esi
	pop edi
	pop edx	
	pop ecx	
	pop ebx	
	pop eax
		
	pop ebp	
	emms
	ret

global mblockq_dist22_mmxe

; void mblockq_dist22_mmxe(unsigned char *blk1,unsigned char *blk2,int flx,int fh, int* resvec);

; eax = p1
; ebx = p2
; ecx = counter temp
; edx = flx;

; mm0 = distance accumulator
; mm1 = distance accumulator
; mm2 = previous p1 row
; mm3 = previous p1 displaced by 1 byte...
; mm4 = temp
; mm5 = temp
; mm6 = temp
; mm7 = temp / 0 if first row 0xff otherwise


align 32
mblockq_dist22_mmxe:
	push ebp		; save frame pointer
	mov ebp, esp
	push eax
	push ebx	
	push ecx
	push edx	

	pxor mm0, mm0		; zero acculumator
	pxor mm1, mm1		; zero acculumator
	pxor mm2, mm2		; zero acculumator
	pxor mm3, mm3		; zero acculumator						

	mov eax, [ebp+8]	; get p1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx
	mov ecx, [ebp+20]
	movq mm2, [eax+edx]
	movq mm3, [eax+edx+1]
	jmp nextrowbd22
align 32
nextrowbd22:
	movq   mm5, [ebx]			; load previous row reference block
								; mm2 /mm3 containts current row target block
		
	psadbw mm2, mm5				; Comparse (x+0,y+2)
	paddd  mm1, mm2
		
	psadbw mm3, mm5				; Compare (x+2,y+2)
	pshufw  mm1, mm1, 2*1 + 3 * 4 + 0 * 16 + 1 * 64
	paddd  mm1, mm3

	pshufw  mm1, mm1, 2*1 + 3 * 4 + 0 * 16 + 1 * 64 					

	movq mm2, [eax]				; Load current row traget block into mm2 / mm3
	movq mm6, mm2
	movq mm3, [eax+1]
	sub	   eax, edx
	sub	   ebx, edx
	prefetcht0 [eax]
	movq mm7, mm3		

	psadbw	mm6, mm5			; Compare (x+0,y+0)
	paddd   mm0, mm6
	pshufw  mm0, mm0, 2*1 + 3 * 4 + 0 * 16 + 1 * 64
	psadbw  mm7, mm5			; Compare (x+2,y+0)
	paddd   mm0, mm7
	pshufw  mm0, mm0, 2*1 + 3 * 4 + 0 * 16 + 1 * 64

	sub ecx, 1
	jnz nextrowbd22

	mov  eax, [ebp+24]
	movq [eax+0], mm0
	movq [eax+8], mm1
	pop edx	
	pop ecx	
	pop ebx	
	pop eax
	pop ebp

	emms
	ret
		




