@----------------------------------------------------------------------
@ File Name    : startup.s
@ Object       :
@ Author       : 
@ Date         : 
@
@ Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
@ All rights reserved.
@----------------------------------------------------------------------

@ Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs
	.include "../../Include/wm_config_gcc.inc"
.equ Mode_USR ,0x10                                              
.equ Mode_FIQ ,0x11                                              
.equ Mode_IRQ ,0x12                                              
.equ Mode_SVC ,0x13                                              
.equ Mode_ABT ,0x17                                              
.equ Mode_UND ,0x1B                                              
.equ Mode_SYS ,0x1F                                              
                                                                 
.equ I_Bit, 0x80            @ when I bit is set, IRQ is disabled 
.equ F_Bit, 0x40            @ when F bit is set, FIQ is disabled 


@// <h> Stack Configuration (Stack Sizes in Bytes)
@//   <o0> Undefined Mode      <0x0-0xFFFFFFFF:8>
@//   <o1> Supervisor Mode     <0x0-0xFFFFFFFF:8>
@//   <o2> Abort Mode          <0x0-0xFFFFFFFF:8>
@//   <o3> Fast Interrupt Mode <0x0-0xFFFFFFFF:8>
@//   <o4> Interrupt Mode      <0x0-0xFFFFFFFF:8>
@//   <o5> User/System Mode    <0x0-0xFFFFFFFF:8>
@// </h>

.equ UND_Stack_Size, 0x00000010                             
.equ SVC_Stack_Size, 0x00000100                             
.equ ABT_Stack_Size, 0x00000010                             
.equ FIQ_Stack_Size, 0x00000000                             
.equ IRQ_Stack_Size, 0x00000100                             
.equ USR_Stack_Size, 0x00000020                             
                                                            
.equ ISR_Stack_Size, (UND_Stack_Size + ABT_Stack_Size + FIQ_Stack_Size + IRQ_Stack_Size)   


.section .vector, "ax"

.global  Vectors
Vectors:
				LDR     PC, Reset_Addr
                LDR     PC, Undef_Addr
                LDR     PC, SWI_Addr
                LDR     PC, PAbt_Addr
                LDR     PC, DAbt_Addr
                NOP                            @ Reserved Vector
                LDR     PC, IRQ_Addr
                LDR     PC, FIQ_Addr
             .extern OS_CPU_Tick_ISR
			 .extern SWI_Handler
Reset_Addr: .word     Reset_Handler
Undef_Addr: .word     Undef_Handler
SWI_Addr:   .word     SWI_Handler
PAbt_Addr:  .word     PAbt_Handler
DAbt_Addr:  .word     DAbt_Handler
NULL_ADDR:  .word     Reset_Handler            @ Reserved Address
IRQ_Addr:   .word     OS_CPU_Tick_ISR
FIQ_Addr:   .word     FIQ_Handler

Undef_Handler:
			B       Undef_Handler
SWI_Handler:
			B       SWI_Handler
PAbt_Handler:
			B       PAbt_Handler
DAbt_Handler:
			B       DAbt_Handler
FIQ_Handler :
			B       FIQ_Handler   

@ Reset Handler

.global  Reset_Handler
Reset_Handler:

               @ flush v4 I/D caches
               mov     r0, #0
               mcr     p15, 0, r0, c7, c5, 0 @ flush v4 I-cache
               mcr     p15, 0, r0, c7, c6, 0 @ flush v4 D-cache
               @ disable MMU stuff and caches
               mrc     p15, 0, r0, c1, c0, 0
               bic     r0, r0, #0x00002300   @ clear bit 13, 9:8(--v- --RS)
			   bic     r0, r0, #0x00000087   @ clear bit 7, 2:0 (B--- -CAM)
			   orr     r0, r0, #0x00000002   @ set bit 2(A) Align
			   orr     r0, r0, #0x00001000   @ set bit 12 (I) I-cache
			   mcr     p15, 0, r0, c1, c0, 0

@ copy exception table to 0x0000000
				ldr r0, =Vectors
				ldr r1, =Reset_Handler
				mov r3, #0
Loop:
				cmp         r0, r1
                ldrcc       r2, [r0], #4
                strcc       r2, [r3], #4
                bcc         Loop

				mov r0, #0
	 			mrc p15, 0, r0, c0, c0, 2

@ Setup Stack for each mode

                LDR     R0, =_stack

@  Enter Undefined Instruction Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_UND|I_Bit|F_Bit
                MOV     SP, R0
                SUB     R0, R0, #UND_Stack_Size

@  Enter Abort Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_ABT|I_Bit|F_Bit
                MOV     SP, R0
                SUB     R0, R0, #ABT_Stack_Size

@  Enter FIQ Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_FIQ|I_Bit|F_Bit
                MOV     SP, R0
                SUB     R0, R0, #FIQ_Stack_Size

@  Enter IRQ Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_IRQ|I_Bit|F_Bit
                MOV     SP, R0
                SUB     R0, R0, #IRQ_Stack_Size

@  Enter Supervisor Mode and set its Stack Pointer
                MSR     CPSR_c, #Mode_SVC|I_Bit|F_Bit
                MOV     SP, R0
                SUB     R0, R0, #SVC_Stack_Size


				LDR		r0, =__bss_start
				LDR		r1, =__bss_end
				MOV		r2, #0
initbss:
				STR		r2, [r0], #4
				CMP		r0, r1
				BLT		initbss
	
        .extern Init_MPU                    @ Import label to MPU init code

        B Init_MPU
        
				
.end
