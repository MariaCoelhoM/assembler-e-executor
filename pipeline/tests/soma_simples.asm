; Gerado automaticamente pelo compilador NDR
; Pipeline: .ndr -> parser -> AST -> codegen -> .asm

        ORG  0

        ; -- T0 + T1 --
        LDA  T0
        ADD  T1
        STA  T2
        ; -- T2 * T3 --
        LDA  T3
        STA  T6
        LDA  T5
MLOOP8:
        LDA  T6
        JZ   MFIM8
        LDA  T5
        ADD  T2
        STA  T5
        LDA  T6
        ADD  T7
        STA  T6
        JMP  MLOOP8
MFIM8:
        LDA  T5
        STA  T4
        ; -- armazena resultado em resultado --
        LDA  T4
        STA  resultado
        HLT

; ── Dados ─────────────────────�
T0           DATA 3
T1           DATA 5
T2           DATA 0
T3           DATA 2
T5           DATA 0
T6           DATA 0
T7           DATA 255
T4           DATA 0
resultado    DATA 0
