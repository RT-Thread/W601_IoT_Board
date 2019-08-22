;----------------------------------------------------------------------
; File Name    : startup.s
; Object       :
; Author       :  
; Date         :  
;
; Copyright (c) 2014 Winner Microelectronics Co., Ltd.
; All rights reserved.
;----------------------------------------------------------------------

; Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs
	INCLUDE wm_os_config.inc
Mode_USR        EQU     0x10
Mode_FIQ        EQU     0x11
Mode_IRQ        EQU     0x12
Mode_SVC        EQU     0x13
Mode_ABT        EQU     0x17
Mode_UND        EQU     0x1B
Mode_SYS        EQU     0x1F

I_Bit           EQU     0x80            ; when I bit is set, IRQ is disabled
F_Bit           EQU     0x40            ; when F bit is set, FIQ is disabled


;// <h> Stack Configuration (Stack Sizes in Bytes)
;//   <o0> Undefined Mode      <0x0-0xFFFFFFFF:8>
;//   <o1> Supervisor Mode     <0x0-0xFFFFFFFF:8>
;//   <o2> Abort Mode          <0x0-0xFFFFFFFF:8>
;//   <o3> Fast Interrupt Mode <0x0-0xFFFFFFFF:8>
;//   <o4> Interrupt Mode      <0x0-0xFFFFFFFF:8>
;//   <o5> User/System Mode    <0x0-0xFFFFFFFF:8>
;// </h>

UND_Stack_Size  EQU     0x00000010
SVC_Stack_Size  EQU     0x00000100
ABT_Stack_Size  EQU     0x00000010
FIQ_Stack_Size  EQU     0x00000000
IRQ_Stack_Size  EQU     0x00000100
USR_Stack_Size  EQU     0x00000020

ISR_Stack_Size  EQU     (UND_Stack_Size + ABT_Stack_Size + \
                         FIQ_Stack_Size + IRQ_Stack_Size)

                AREA    STACK, NOINIT, READWRITE, ALIGN=3

Stack_Mem       SPACE   SVC_Stack_Size
__initial_sp    SPACE   ISR_Stack_Size

Stack_Top

;// <h> Heap Configuration
;//   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF>
;// </h>
  IF :DEF: DEBUG
Heap_Size       EQU     0x0000C000
  ELSE
Heap_Size       EQU     0x00010000
  ENDIF
                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

; Area Definition and Entry Point
;  Startup Code must be linked first at Address at which it expects to run.

                AREA    RESET, CODE, READONLY
                ARM
				ENTRY


; Exception Vectors
;  Mapped to Address 0.
;  Absolute addressing mode must be used.
;  Dummy Handlers are implemented as infinite loops which can be modified.

Vectors         LDR     PC, Reset_Addr
                LDR     PC, Undef_Addr
                LDR     PC, SWI_Addr
                LDR     PC, PAbt_Addr
                LDR     PC, DAbt_Addr
                NOP                            ; Reserved Vector
                LDR     PC, IRQ_Addr
                LDR     PC, FIQ_Addr
                IMPORT OS_CPU_Tick_ISR
                IMPORT  SWI_Handler

Reset_Addr      DCD     Reset_Handler
Undef_Addr      DCD     Undef_Handler
SWI_Addr        DCD     SWI_Handler
PAbt_Addr       DCD     PAbt_Handler
DAbt_Addr       DCD     DAbt_Handler
                DCD     0                      ; Reserved Address
IRQ_Addr        DCD     OS_CPU_Tick_ISR
FIQ_Addr        DCD     FIQ_Handler

Undef_Handler   B       Undef_Handler
;SWI_Handler     B      SWI_Handler
PAbt_Handler    B       PAbt_Handler
DAbt_Handler    B       DAbt_Handler
;IRQ_Handler     B       IRQ_Handler
FIQ_Handler     B       FIQ_Handler


; Reset Handler

                EXPORT  Reset_Handler
Reset_Handler

               ; flush v4 I/D caches
               mov    r0, #0
               mcr     p15, 0, r0, c7, c5, 0 ; flush v4 I-cache
               mcr     p15, 0, r0, c7, c6, 0 ; flush v4 D-cache

               ; disable MMU stuff and caches
               mrc    p15, 0, r0, c1, c0, 0
               bic      r0, r0, #0x00002300   ; clear bit 13, 9:8(--v- --RS)
			    bic     r0, r0, #0x00000087   ; clear bit 7, 2:0 (B--- -CAM)
			    orr     r0, r0, #0x00000002   ; set bit 2(A) Align
			    orr     r0, r0, #0x00001000   ; set bit 12 (I) I-cache
			    mcr     p15, 0, r0, c1, c0, 0

; copy exception table to 0x0000000
		  ldr 		  r0, =RESET
		  ldr 		  r1, =Reset_Handler
		  mov 	  r3, #0
		  
Loop         cmp        r0, r1
                ldrcc       r2, [r0], #4
                strcc       r2, [r3], #4
                bcc         Loop

				mov r0, #0
	 			mrc p15, 0, r0, c0, c0, 2

; Setup Stack for each mode

                LDR     R0, =Stack_Top

;  Enter Undefined Instruction Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_UND:OR:I_Bit:OR:F_Bit
                MOV     SP, R0
                SUB     R0, R0, #UND_Stack_Size

;  Enter Abort Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_ABT:OR:I_Bit:OR:F_Bit
                MOV     SP, R0
                SUB     R0, R0, #ABT_Stack_Size

;  Enter FIQ Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_FIQ:OR:I_Bit:OR:F_Bit
                MOV     SP, R0
                SUB     R0, R0, #FIQ_Stack_Size

;  Enter IRQ Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_IRQ:OR:I_Bit:OR:F_Bit
                MOV     SP, R0
                SUB     R0, R0, #IRQ_Stack_Size

;  Enter Supervisor Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_SVC:OR:I_Bit:OR:F_Bit
                MOV     SP, R0
                SUB     R0, R0, #SVC_Stack_Size

               ; MOV     SP, R0
               ; SUB     SL, SP, #USR_Stack_Size


        IMPORT  Init_MPU                    ; Import label to MPU init code

        B       Init_MPU

; Enter the C code

               ; IMPORT  __main
               ;B     	   __main
										

                IF      :DEF:__MICROLIB

                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE
; User Initial Stack & Heap
                AREA    |.text|, CODE, READONLY

                IMPORT  __use_two_region_memory
				EXPORT  __user_setup_stackheap
__user_setup_stackheap
				LDR		r0, = Heap_Mem
				LDR		sp, = (Stack_Mem + SVC_Stack_Size)
				LDR		r2, = (Heap_Mem	+ Heap_Size)
			    LDR     R3, = Stack_Mem
				BX		lr
                ;EXPORT  __user_initial_stackheap
;__user_initial_stackheap
 ;
  ;              LDR     R0, =  Heap_Mem
   ;             LDR     R1, =(Stack_Mem + USR_Stack_Size)
    ;            LDR     R2, = (Heap_Mem +      Heap_Size)
     ;           LDR     R3, = Stack_Mem
      ;          BX      LR
                ENDIF


                END
