;
;  predict.s:  mmX optimized block summing differencing routines
;
;  Believed to be original Copyright (C) 2000 Brent Byeler
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


;void sub_pred_mmx(unsigned char *pred,
;                  unsigned char *cur,
;                  int lx, short *blk)

align 32
global sub_pred_mmx
sub_pred_mmx:

		push ebp				; save frame pointer
		mov ebp, esp		; link
		push eax
		push ebx
		push ecx
		push esi     
		push edi

        mov   eax, [ebp+12] ;cur
        mov   ebx, [ebp+8]  ;pred
        mov   ecx, [ebp+20] ;blk
        mov   edi, [ebp+16] ;lx
        mov   esi, 8
        pxor  mm7, mm7
sub_top:
        movq  mm0, [eax]
        add   eax, edi
        movq  mm2, [ebx]
        add   ebx, edi
        movq  mm1, mm0
        punpcklbw mm0, mm7
        punpckhbw mm1, mm7
        movq  mm3, mm2
        punpcklbw mm2, mm7
        punpckhbw mm3, mm7

        psubw  mm0, mm2
        psubw  mm1, mm3

        movq   [ecx], mm0
        movq   [ecx+8], mm1
        add    ecx, 16

        dec    esi
        jg     sub_top
	
		pop edi
		pop esi
		pop ecx
		pop ebx
		pop eax
		pop ebp			; restore stack pointer

		emms			; clear mmx registers
		ret		

; add prediction and prediction error, saturate to 0...255
;void add_pred_mmx(unsigned char *pred,
;                  unsigned char *cur,
;                  int lx, short *blk)

align 32
global add_pred_mmx
add_pred_mmx:

		push ebp				; save frame pointer
		mov ebp, esp		; link
		push eax
		push ebx
		push ecx
		push esi     
		push edi
	
        mov   eax, [ebp+12] ;cur
        mov   ebx, [ebp+8]  ;pred
        mov   ecx, [ebp+20] ;blk
        mov   edi, [ebp+16] ;lx
        mov   esi, 8
        pxor  mm7, mm7
add_top:
        movq  mm0, [ecx]
        movq  mm1, [ecx+8]
        add   ecx, 16
        movq  mm2, [ebx]
        add   ebx, edi
        movq  mm3, mm2
        punpcklbw mm2, mm7
        punpckhbw mm3, mm7

        paddw  mm0, mm2
        paddw  mm1, mm3
        packuswb mm0, mm1

        movq   [eax], mm0
        add    eax, edi

        dec    esi
        jg     add_top
	
		pop edi
		pop esi
		pop ecx
		pop ebx
		pop eax
		pop ebp			; restore stack pointer

		emms			; clear mmx registers
		ret		


