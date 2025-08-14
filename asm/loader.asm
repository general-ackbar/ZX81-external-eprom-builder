; upperrom_stub.asm (build as raw binary located at ORG 0x2000)
        ORG     $2000

BOOT:
        ; HL = start compressed data right after code + ZX7-routine
        ld      hl, COMPRESSED_START

        ; DE = destination for VERSN.. (sys var) in RAM
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

COMPRESSED_START:
        ; The actual data is added by p2bin
