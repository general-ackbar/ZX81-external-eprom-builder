        incbin "../backup/zx81_org.rom"
        org     $2000          ; 8192

BOOT:


   
        CALL    $0A2A                           ; Call the 'CLS' routine, which will inflate the display file and clear it to all spaces.
print:
        ld bc, MENU
.loop:
        ld a, (bc)
        cp 9
        jr z, .done
        RST $10 ; PRINT
        inc bc
        jr .loop
.done:


.after_print:

    ; GÅ KORREKT I SLOW (tænd NMI sikkert)
    di
    CALL    $0F2B              ; SLOW wrapper -> JP L0207 (SLOW/FAST)  :contentReference[oaicite:8]{index=8}

    ; Nulstil debounce før vi læser en tast
;    CALL    $0F4B              ; DEBOUNCE        
 
	LD      C,$FE           ; ULA port

wait_for_key:
    ; --- scan rækken med '1'..'5' ---
    LD      B,$F7           ; vælg række (1..5)
    IN      A,(C)
.check1:
    BIT     0,A             ; '1'    
    JR      NZ,.check2
    LD   	HL, PACMAN
    jr 		.done
.check2:
    BIT     1,A             ; '2'
    JR      NZ,.check3
    LD   	HL, GULP
    jr 		.done
.check3:
    BIT     2,A             ; '3'
    JR      NZ,.check4
    ld 		a, $1F
    jr 		.done
.check4:
    BIT     3,A             ; '4'
    JR      NZ,.check5
    ld 		a, $20
    jr 		.done
.check5:
    BIT     4,A             ; '5'
    JR      NZ,.check6
    ld 		a, $21
    jr 		.done
    ; --- scan rækken med '6'..'0' ---    
.check6:
    LD      B,$EF
    IN      A,(C)
    BIT     0,A             ; '6'
    JR      NZ,.check7
    ld 		a, $22
    jr 		.done
.check7:
    BIT     1,A             ; '7'
    JR      NZ,.check8
    ld 		a, $23
    jr 		.done
.check8:
    BIT     2,A             ; '8'
    JR      NZ,.check9
    ld 		a, $24
    jr 		.done
.check9:
    BIT     3,A             ; '9'
    JR      NZ,.nodigitpressed
    ld 		a, $25
    jr 		.done

.nodigitpressed
    JR      wait_for_key


.done:

		
    
    CALL   $02E7              ; Switch back to FAST

    ; HL = start compressed data right after code + ZX7-routine
    ;DE = destination for VERSN.. (sys var) in RAM
    ld      de, $4009

    ; decompress
    call    dzx7_standard

    ; set sys vars
    ld      (iy+0),$ff      ; ERR_NR = $FF
    xor     a
    ld      ($4006),a       ; MODE = K
    ld      (iy+1),$c0      ; FLAGS reset
    jp      $0f2b           ; exit via SLOW -> LINERUN ($0676)


    include "dzx7_standard.asm"

	
MENU:
        db 53, 55, 42, 56, 56, 0, 29, 22, 34, 9


GULP:
PACMAN:
        incbin "../examples/Pacman.p.zx7"