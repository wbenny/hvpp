;++
;
; Copyright (c) Petr Benes. All rights reserved.
;
; Module:
;
;   noopt.asm
;
; Abstract:
;
;   This module implements dummy function to be called as an enforcement
;   to not optimize symbol out.
;
; Author:
;
;    Petr Benes (@PetrBenes) 26-Jul-2018 - Initial version
;
; Environment:
;
;    Kernel mode only.
;
;--

.CODE

;++
;
; void
; do_not_optimize_out(
;   ...
;   )
;
; Routine description:
;
;   This function does nothing.
;
; Arguments:
;
;   Desired symbols which shouldn't be optimized out.
;
; Return Value:
;
;   None.
;
;--

    do_not_optimize_out PROC
      ret
    do_not_optimize_out ENDP

END
