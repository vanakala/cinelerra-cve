;;; 
;;;  mblock_sad_mmxe.s:  
;;; 
;;; Enhanced MMX optimized Sum Absolute Differences routines for macroblocks
;;; (interpolated, 1-pel, 2*2 sub-sampled pel and 4*4 sub-sampled pel)
;
;  dist1_* Original Copyright (C) 2000 Chris Atenasio <chris@crud.net>
;  Enhancements and rest Copyright (C) 2000 Andrew Stevens <as@comlab.ox.ac.uk>

		;; Yes, I tried prefetch-ing.  It makes no difference or makes
		;; stuff *slower*.

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
;


global dist1_00_mmxe

; int dist1_00(char *blk1,char *blk2,int lx,int h,int distlim);
; distlim unused - costs more to check than the savings of
; aborting the computation early from time to time...
; eax = p1
; ebx = p2
; ecx = rowsleft
; edx = lx;

; mm0 = distance accumulator
; mm1 = temp
; mm2 = temp
; mm3 = temp
; mm4 = temp
; mm5 = temp
; mm6 = temp


align 32
dist1_00_mmxe:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push ebx
	push ecx
	push edx

	pxor mm0, mm0		; zero acculumator

	mov eax, [ebp+8]	; get p1
dist1_00_0misalign:		
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx

	mov ecx, [ebp+20]	; get rowsleft
	jmp nextrow00sse
align 32
nextrow00sse:
	movq mm4, [eax]		; load first 8 bytes of p1 (row 1)
	psadbw mm4, [ebx]	; compare to first 8 bytes of p2 (row 1)
	movq mm5, [eax+8]	; load next 8 bytes of p1 (row 1)
	add eax, edx		; update pointer to next row
	paddd mm0, mm4		; accumulate difference
	
	psadbw mm5, [ebx+8]	; compare to next 8 bytes of p2 (row 1)
	add ebx, edx		; ditto
	paddd mm0, mm5		; accumulate difference


	movq mm6, [eax]		; load first 8 bytes of p1 (row 2)
	psadbw mm6, [ebx]	; compare to first 8 bytes of p2 (row 2)
	movq mm4, [eax+8]	; load next 8 bytes of p1 (row 2)
	add eax, edx		; update pointer to next row
	paddd mm0, mm6		; accumulate difference
	
	psadbw mm4, [ebx+8]	; compare to next 8 bytes of p2 (row 2)
	add ebx, edx		; ditto
	paddd mm0, mm4		; accumulate difference

	;psubd mm2, mm3		; decrease rowsleft
	;movq mm5, mm1		; copy distlim
	;pcmpgtd mm5, mm0	; distlim > dist?
	;pand mm2, mm5		; mask rowsleft with answer
	;movd ecx, mm2		; move rowsleft to ecx

	;add eax, edx		; update pointer to next row
	;add ebx, edx		; ditto
	
	;test ecx, ecx		; check rowsleft
	sub  ecx, 2
	jnz nextrow00sse

	movd eax, mm0		; store return value
	
	pop edx	
	pop ecx	
	pop ebx	

	pop ebp	
	emms
	ret	


				

global dist1_00_Ammxe
		;; This is a special version that only does aligned accesses...
		;; Wonder if it'll make it faster on a P-III
		;; ANSWER:		 NO its slower hence no longer used.

; int dist1_00(char *blk1,char *blk2,int lx,int h,int distlim);
; distlim unused - costs more to check than the savings of
; aborting the computation early from time to time...
; eax = p1
; ebx = p2
; ecx = rowsleft
; edx = lx;

; mm0 = distance accumulator
; mm1 = temp
; mm2 = right shift to adjust for mis-align
; mm3 = left shift to adjust for mis-align
; mm4 = temp
; mm5 = temp
; mm6 = temp


align 32
dist1_00_Ammxe:
	push ebp					; save frame pointer
	mov ebp, esp				; link

	push ebx
	push ecx
	push edx
		
	pxor mm0, mm0		; zero acculumator

	mov eax, [ebp+8]	; get p1
	mov ebx, eax
	and ebx, 7					; Misalignment!
	cmp ebx, 0
	jz	near dist1_00_0misalign
	sub eax, ebx				; Align eax
	mov ecx, 8					; ecx = 8-misalignment
	sub ecx, ebx
	shl ebx, 3					; Convert into bit-shifts...
	shl ecx, 3					
	movd mm2, ebx				; mm2 = shift to start msb
	movd mm3, ecx				; mm3 = shift to end lsb

	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx
	mov ecx, [ebp+20]	; get rowsleft
	jmp nextrow00ssea
align 32
nextrow00ssea:
	movq mm4, [eax]				; load first 8 bytes of aligned p1 (row 1)
	movq mm5, [eax+8]			; load next 8 bytes of aligned p1 (row 1)
	movq mm6, mm5
	psrlq mm4, mm2				; mm4 first 8 bytes of p1 proper
	psllq mm5, mm3
	por	  mm4, mm5
	psadbw mm4, [ebx]	; compare to first 8 bytes of p2 

	movq mm7, [eax+16]			; load last 8 bytes of aligned p1
	add eax, edx		; update pointer to next row
	psrlq mm6, mm2				; mm6 2nd 8 bytes of p1 proper
	psllq mm7, mm3
	por   mm6, mm7


	paddd mm0, mm4		; accumulate difference
	
	psadbw mm6, [ebx+8]	; compare to next 8 bytes of p2 (row 1)
	add ebx, edx		; ditto
	paddd mm0, mm6		; accumulate difference

	sub  ecx, 1
	jnz nextrow00ssea

	movd eax, mm0		; store return value

	pop edx	
	pop ecx	
	pop ebx	

	pop ebp	
	emms
	ret	


global dist1_01_mmxe

; int dist1_01(char *blk1,char *blk2,int lx,int h);

; eax = p1
; ebx = p2
; ecx = counter temp
; edx = lx;

; mm0 = distance accumulator
; mm1 = distlim
; mm2 = rowsleft
; mm3 = 2 (rows per loop)
; mm4 = temp
; mm5 = temp
; mm6 = temp


align 32
dist1_01_mmxe:
	push ebp	
	mov ebp, esp

	push ebx
	push ecx
	push edx

	pxor mm0, mm0		; zero acculumator

	mov eax, [ebp+8]	; get p1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx

	mov ecx, [ebp+20]	; get rowsleft
	jmp nextrow01		; snap to it
align 32
nextrow01:
	movq mm4, [eax]				; load first 8 bytes of p1 (row 1)
	pavgb mm4, [eax+1]			; Interpolate...
	psadbw mm4, [ebx]			; compare to first 8 bytes of p2 (row 1)
	paddd mm0, mm4				; accumulate difference

	movq mm5, [eax+8]			; load next 8 bytes of p1 (row 1)
	pavgb mm5, [eax+9]			; Interpolate
	psadbw mm5, [ebx+8]			; compare to next 8 bytes of p2 (row 1)
	paddd mm0, mm5				; accumulate difference

	add eax, edx				; update pointer to next row
	add ebx, edx				; ditto

	movq mm6, [eax]				; load first 8 bytes of p1 (row 2)
	pavgb mm6, [eax+1]			; Interpolate
	psadbw mm6, [ebx]	; compare to first 8 bytes of p2 (row 2)
	paddd mm0, mm6		; accumulate difference
	
	movq mm7, [eax+8]	; load next 8 bytes of p1 (row 2)
	pavgb mm7, [eax+9]
	psadbw mm7, [ebx+8]	; compare to next 8 bytes of p2 (row 2)
	paddd mm0, mm7		; accumulate difference

	add eax, edx		; update pointer to next row
	add ebx, edx		; ditto
	
	sub ecx, 2			; check rowsleft
	jnz nextrow01		; rinse and repeat

	movd eax, mm0		; store return value
	
	pop edx	
	pop ecx		
	pop ebx	

	pop ebp			; restore stack pointer

	emms			; clear mmx registers
	ret			; we now return you to your regular programming


global dist1_10_mmxe

; int dist1_10(char *blk1,char *blk2,int lx,int h);

; eax = p1
; ebx = p2
; ecx = counter temp
; edx = lx;
; edi = p1+lx  

; mm0 = distance accumulator
; mm2 = rowsleft
; mm3 = 2 (rows per loop)
; mm4 = temp
; mm5 = temp
; mm6 = temp


align 32
dist1_10_mmxe:
	push ebp		; save stack pointer
	mov ebp, esp

	push ebx
	push ecx
	push edx
	push edi

	pxor mm0, mm0		; zero acculumator

	mov eax, [ebp+8]	; get p1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx
	mov edi, eax
	add edi, edx
	mov ecx, [ebp+20]	; get rowsleft
	jmp nextrow10		; snap to it
align 32
nextrow10:
	movq mm4, [eax]				; load first 8 bytes of p1 (row 1)
	pavgb mm4, [edi]			; Interpolate...
	psadbw mm4, [ebx]			; compare to first 8 bytes of p2 (row 1)
	paddd mm0, mm4				; accumulate difference

	movq mm5, [eax+8]			; load next 8 bytes of p1 (row 1)
	pavgb mm5, [edi+8]			; Interpolate
	psadbw mm5, [ebx+8]			; compare to next 8 bytes of p2 (row 1)
	paddd mm0, mm5				; accumulate difference

	add eax, edx				; update pointer to next row
	add ebx, edx				; ditto
	add edi, edx

	movq mm6, [eax]				; load first 8 bytes of p1 (row 2)
	pavgb mm6, [edi]			; Interpolate
	psadbw mm6, [ebx]	; compare to first 8 bytes of p2 (row 2)
	paddd mm0, mm6		; accumulate difference
	
	movq mm7, [eax+8]	; load next 8 bytes of p1 (row 2)
	pavgb mm7, [edi+8]
	psadbw mm7, [ebx+8]	; compare to next 8 bytes of p2 (row 2)
	paddd mm0, mm7		; accumulate difference

	psubd mm2, mm3		; decrease rowsleft

	add eax, edx		; update pointer to next row
	add ebx, edx		; ditto
	add edi, edx
	
	sub ecx, 2			; check rowsleft (we're doing 2 at a time)
	jnz nextrow10		; rinse and repeat

	movd eax, mm0		; store return value
	
	pop edi
	pop edx	
	pop ecx	
	pop ebx	

	pop ebp			; restore stack pointer

	emms			; clear mmx registers
	ret			; we now return you to your regular programming


global dist1_11_mmxe

; int dist1_11(char *blk1,char *blk2,int lx,int h);

; eax = p1
; ebx = p2
; ecx = counter temp
; edx = lx;
; edi = p1+lx  

		  
; mm0 = distance accumulator
; mm2 = rowsleft
; mm3 = 2 (rows per loop)
; mm4 = temp
; mm5 = temp
; mm6 = temp


align 32
dist1_11_mmxe:
	push ebp		; save stack pointer
	mov ebp, esp		; so that we can do this

	push ebx		; save the pigs
	push ecx		; make them squeal
	push edx		; lets have pigs for every meal
	push edi

	pxor mm0, mm0		; zero acculumator

	mov eax, [ebp+8]	; get p1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx
	mov edi, eax
	add edi, edx
	mov ecx, [ebp+20]	; get rowsleft
	jmp nextrow11		; snap to it
align 32
nextrow11:
	movq mm4, [eax]				; load first 8 bytes of p1 (row 1)
	pavgb mm4, [edi]			; Interpolate...
	movq mm5, [eax+1]
	pavgb mm5, [edi+1]
	pavgb mm4, mm5
	psadbw mm4, [ebx]			; compare to first 8 bytes of p2 (row 1)
	paddd mm0, mm4				; accumulate difference

	movq mm6, [eax+8]			; load next 8 bytes of p1 (row 1)
	pavgb mm6, [edi+8]			; Interpolate
	movq mm7, [eax+9]
	pavgb mm7, [edi+9]
	pavgb mm6, mm7
	psadbw mm6, [ebx+8]			; compare to next 8 bytes of p2 (row 1)
	paddd mm0, mm6				; accumulate difference

	add eax, edx				; update pointer to next row
	add ebx, edx				; ditto
	add edi, edx

	movq mm4, [eax]				; load first 8 bytes of p1 (row 1)
	pavgb mm4, [edi]			; Interpolate...
	movq mm5, [eax+1]
	pavgb mm5, [edi+1]
	pavgb mm4, mm5
	psadbw mm4, [ebx]			; compare to first 8 bytes of p2 (row 1)
	paddd mm0, mm4				; accumulate difference

	movq mm6, [eax+8]			; load next 8 bytes of p1 (row 1)
	pavgb mm6, [edi+8]			; Interpolate
	movq mm7, [eax+9]
	pavgb mm7, [edi+9]
	pavgb mm6, mm7
	psadbw mm6, [ebx+8]			; compare to next 8 bytes of p2 (row 1)
	paddd mm0, mm6				; accumulate difference
		
	add eax, edx		; update pointer to next row
	add ebx, edx		; ditto
	add edi, edx

			
	sub ecx, 2				; check rowsleft
	jnz near nextrow11			; rinse and repeat

	movd eax, mm0				; store return value
	
	pop edi
	pop edx	
	pop ecx		
	pop ebx			
	
	pop ebp			; restore stack pointer

	emms			; clear mmx registers
	ret			; we now return you to your regular programming

global dist22_mmxe

; int dist22_mmxe(unsigned char *blk1,unsigned char *blk2,int flx,int fh);

; eax = p1
; ebx = p2
; ecx = counter temp
; edx = flx;

; mm0 = distance accumulator
; mm2 = rowsleft
; mm3 = 2 (rows per loop)
; mm4 = temp
; mm5 = temp
; mm6 = temp


align 32
dist22_mmxe:
	push ebp		; save frame pointer
	mov ebp, esp

	push ebx	
	push ecx
	push edx	

	pxor mm0, mm0		; zero acculumator

	mov eax, [ebp+8]	; get p1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get lx

	mov ecx, [ebp+20]
	jmp nextrowfd
align 32
nextrowfd:
	movq   mm4, [eax]	 ; load first 8 bytes of p1 (row 1) 
	add eax, edx		; update pointer to next row
	psadbw mm4, [ebx]	; compare to first 8 bytes of p2 (row 1)
	add ebx, edx		; ditto
	paddd  mm0, mm4		; accumulate difference
	

	movq mm6, [eax]		; load first 8 bytes of p1 (row 2)
	add eax, edx		; update pointer to next row
	psadbw mm6, [ebx]	; compare to first 8 bytes of p2 (row 2)
	add ebx, edx		; ditto
	paddd mm0, mm6		; accumulate difference
	

	sub ecx, 2
	jnz nextrowfd

	movd eax, mm0
	
	pop edx	
	pop ecx	
	pop ebx	

	pop ebp

	emms
	ret





global dist44_mmxe

; int dist44_mmxe(unsigned char *blk1,unsigned char *blk2,int qlx,int qh);

; eax = p1
; ebx = p2
; ecx = temp
; edx = qlx;
; esi = rowsleft

; mm0 = distance accumulator left block p1
; mm1 = distance accumulator right block p1
; mm2 = 0
; mm3 = 0
; mm4 = temp
; mm5 = temp
; mm6 = temp


align 32
dist44_mmxe:
	push ebp
	mov ebp, esp

	push ebx
	push ecx
	push edx
	push esi     

	pxor mm0, mm0		; zero acculumator
	pxor mm1, mm1				
	pxor mm2, mm2
	mov eax, [ebp+8]	; get p1
	mov ebx, [ebp+12]	; get p2
	mov edx, [ebp+16]	; get qlx

	mov esi, [ebp+20]	; get rowsleft
	jmp nextrowqd		; snap to it
align 32
nextrowqd: 
	movq mm4, [eax]				; load 8 bytes of p1 (two blocks!)
	add eax, edx		; update pointer to next row
	movq mm6, mm4				  ;
	mov  ecx, [ebx]       ; load 4 bytes of p2
    punpcklbw mm4, mm2			; mm4 = bytes 0..3 p1 (spaced out)
	movd mm5, ecx
	punpcklbw mm5, mm2      ; mm5 = bytes 0..3 p2  (spaced out)
	psadbw mm4, mm5	    		; compare to left block
	add ebx, edx		; ditto

;	punpckhbw mm6, mm2          ; mm6 = bytes 4..7 p1 (spaced out)

	paddd mm0, mm4				; accumulate difference left block

;	psadbw mm6,mm5				; compare to right block
	

;	paddd mm1, mm6				; accumulate difference right block
		
	sub esi, 1
	jnz nextrowqd

	movd eax, mm0
;	movd ebx, mm1				
;	sal  ebx, 16
;	or   eax, ebx
	
	pop esi
	pop edx
	pop ecx
	pop ebx

	pop ebp			; restore stack pointer

	emms			; clear mmx registers
	ret			; we now return you to your regular programming
