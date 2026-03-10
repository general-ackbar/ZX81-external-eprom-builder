        org     $2000          ; 8192


    
NEW:    CALL    $02E7           ;NEW ENTRY POINT
        LD      HL,($4004)      ;GET RAMTOP
        SCF                     ;FLAG AS NEW,NOT RESET

RST2:   LD      ($4004),HL      ;SAVE RAMTOP
        DEC     HL;
        LD      (HL),$3E
        DEC     HL
        LD      SP, HL          ;NEW STACKPOINTER
        DEC     HL
        DEC     HL
        LD      ($4002),HL      ;ERR-SP
        LD      A,$1E           ;POINTER TO VIDEO
        LD      I,A             ;FONT TABLE (MSB)
        IM      1               ;SET INTERRUPT MODE 1
        LD      IY,$4000        ;SYSVAR POINTER
        LD      HL,$407D;
        LD      (IY+$59),H      ;INIT CDFLAG
        LD      ($400C),HL      ;DFILE=407D
        LD      ($4029),HL      ;NXTLIN=407D
        LD      B,$19           ;COLLAPSED DISPLAY
NL:     LD      (HL),$76        ;HAS 25 N/L CHR
        INC     HL
        DJNZ    NL
        LD      ($4010),HL      ;VARS=4096
        PUSH    AF              ;NEW/RESET FLAG
        CALL    $149A           ;CLEAR POINTERS
        CALL    $0A2A           ; routine CLS
        POP     AF              ;NEW/RESET FLAG
        LD      HL,$0676        ;LOAD LINERUN
        PUSH    HL              ;AS RETURN ADDRESS
        JR      C,NEW1          ;JR IF NEW
        LD      A,$FE           ;TEST SHIFT KEY
        IN      A,$FE
        RRA
        CALL    $0F4B           ;RESET DEBOUNCE
        JP      C,BOOT          ;JP TO UPPER_ROM
        

NEW1:   CALL    $149A           ; routine CLEAR sets $80 end-marker and the
                                ; dynamic memory pointers E_LINE, STKBOT and
                                ; STKEND.

;; N/L-ONLY
L0413:  CALL    $14AD           ; routine CURSOR-IN inserts the cursor and
                                ; end-marker in the Edit Line also setting
                                ; size of lower display to two lines.

        CALL    $0207           ; routine SLOW/FAST selects COMPUTE and DISPLAY

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
    ; --- scan rækken med '6'..'0' ---    
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
        db $35, $37, $2A, $38, $38, $00, $1D, $16, $22

