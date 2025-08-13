; upperrom_stub.asm (build as raw binary located at ORG 0x2000)
        ORG     $2000

BOOT:
        ; HL = start på komprimeret data: lige efter kode + ZX7-rutinen
        ld      hl, COMPRESSED_START

        ; DE = destination for VERSN.. (systemvariabler) i RAM
        ld      de, $4009

        ; dekomp
        call    dzx7_standard

        ; sørg for at maskinen ikke vælter
        ld      (iy+0),$ff      ; ERR_NR = $FF
        xor     a
        ld      ($4006),a       ; MODE = K
        ld      (iy+1),$c0      ; FLAGS reset
        jp      $0f2b           ; exit via SLOW -> LINERUN ($0676)

; --- ZX7 standard rutine (din vedhæftede fil)
        include "dzx7_standard.asm"

COMPRESSED_START:
        ; her placerer vi komprimerede bytes direkte bagefter ved build
        ; (C++-værktøjet lægger filen her og fylder ud med 0 til $4000)
