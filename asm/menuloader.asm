; build as raw binary located at ORG 0x2000

	org 0x2000
BOOT:

        CALL    $0A2A                           ;'CLS'
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


    di						; Disable interupts and switch to SLOW
    CALL    $0F2B           ; SLOW wrapper -> JP L0207 


    CALL    $0F4B           ; DEBOUNCE         
	LD      C,$FE           ; Read ULA port
wait_for_key:
    LD      B,$F7           ; Rows 1..5
    IN      A,(C)
.check1:
    BIT     0,A             ; '1'    
    JR      NZ,.check2
    LD   	HL, $2000
    jr 		.done
.check2:
    BIT     1,A             ; '2'
    JR      NZ,.check3
    LD   	HL, $2000
    jr 		.done
.check3:
    BIT     2,A             ; '3'
    JR      NZ,.check4
    LD   	HL, $2000
    jr 		.done
.check4:
    BIT     3,A             ; '4'
    JR      NZ,.check5
    LD   	HL, $2000
    jr 		.done
.check5:
    BIT     4,A             ; '5'
    JR      NZ,.check6
    LD   	HL, $2000
    jr 		.done
    ; Rows 6-0
.check6:
    LD      B,$EF
    IN      A,(C)
    BIT     0,A             ; '6'
    JR      NZ,.check7
    LD   	HL, $2000
    jr 		.done
.check7:
    BIT     1,A             ; '7'
    JR      NZ,.check8
    LD   	HL, $2000
    jr 		.done
.check8:
    BIT     2,A             ; '8'
    JR      NZ,.check9
    LD   	HL, $2000
    jr 		.done
.check9:
    BIT     3,A             ; '9'
    JR      NZ,.nodigitpressed
    LD   	HL, $2000
    jr 		.done
.nodigitpressed
    JR      wait_for_key
.done

		
    
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

MENU:	;non terminated menu ("PRESS 1-"). Must be terminated with $9
        db $35, $37, $2A, $38, $38, $00, $1D, $16, $22
        
;P-file data will go here. Menu and data is filled from p2bin
