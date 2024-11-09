COPY    START   1000
        LDA     ALPHA
        STA     BETA
        +JSUB   SUBRTN
ALPHA   WORD    5
BETA    RESW    1
SUBRTN  RESB    2
        END     COPY
