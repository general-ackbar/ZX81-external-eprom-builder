; print something to the screen
        CALL    $0A2A                           ; Call the 'CLS' routine, which will inflate the display file and clear it to all spaces.

        LD A,1 ; start with 1
myloop:
        RST $10 ; PRINT
        INC A ; increment A
        CP 36 ; stop at 36
        JP NZ,myloop ; if not 36, then goto/jump to LOOP
