; Gerado automaticamente pelo compilador NDR
; Pipeline: .ndr -> parser -> AST -> codegen -> .asm

        ORG  0

        ; -- T1 * T2 --
        LDA  T2
        STA  T5
        LDA  T4
MLOOP7:
        LDA  T5
        JZ   MFIM7
        LDA  T4
        ADD  T1
        STA  T4
        LDA  T5
        ADD  T6
        STA  T5
        JMP  MLOOP7
MFIM7:
        LDA  T4
        STA  T3
        ; -- T0 + T3 --
        LDA  T0
        ADD  T3
        STA  T7
        ; -- T7 - T8 --
        LDA  T8
        NOT
        ADD  T10
        STA  T11
        LDA  T7
        ADD  T11
        STA  T9
        ; -- armazena resultado em resultado --
        LDA  T9
        STA  resultado
        HLT

; ── Dados ─────────────────────�
T0           DATA 10
T1           DATA 4
T2           DATA 3
T4           DATA 0
T5           DATA 0
T6           DATA 255
T3           DATA 0
T7           DATA 0
T8           DATA 2
T10          DATA 1
T11          DATA 0
T9           DATA 0
resultado    DATA 0
