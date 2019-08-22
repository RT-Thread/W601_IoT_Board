;;  Copyright ARM Ltd 2002-2008. All rights reserved.
;;
;;  This code provides basic initialization for an ARM946E-S including:
;;
;;  - creation of memory protection regions
;;  - setting of region attributes
;;
;;  - enabling the Instruction and Data caches and Write buffer
;;  - enabling Memory Protection Unit
;;  - regions must be defined for TCM memory addresses
;;
;;  This code must be run from a privileged mode

;;  MPU region size defines

Region_4K      EQU 2_01011
Region_8K      EQU 2_01100
Region_16K     EQU 2_01101
Region_32K     EQU 2_01110
Region_64K     EQU 2_01111
Region_128K    EQU 2_10000
Region_256K    EQU 2_10001
Region_512K    EQU 2_10010
Region_1M      EQU 2_10011
Region_2M      EQU 2_10100
Region_4M      EQU 2_10101
Region_8M      EQU 2_10110
Region_16M     EQU 2_10111
Region_32M     EQU 2_11000
Region_64M     EQU 2_11001
Region_128M    EQU 2_11010
Region_256M    EQU 2_11011
Region_512M    EQU 2_11100
Region_1G      EQU 2_11101
Region_2G      EQU 2_11110
Region_4G      EQU 2_11111

Region_Enable  EQU 2_1



        AREA   InitMPU, CODE, READONLY

        EXPORT Init_MPU

Init_MPU FUNCTION

; MPU region definitions/properties
; =================================
;
; Note each Instruction region must have a corresponding data region
; so inline data accesses will work
;
; Note each region base address must be a multiple of its size

; 区域0       : 0   ~     4GB                      NCNB	             No  access
; 区域1       : 0   ~     0x20000,   指令区(128K)       Cached NB		 Rea d only
;									 (debug 256)
; 区域3       : 0x20000 ~ 0x60000    数据区(256K)        Cached Buffered    Full access





; Set up region 0 - Background and enable
        MOV     r0,#(Region_4G << 1) :OR: Region_Enable
        MCR     p15, 0, r0, c6, c0, 0

; Set up region 1 - instrcture and enable

        MOV     r0,#(Region_128K <<1):OR:Region_Enable
		;MOV     r0,#(Region_256K <<1):OR:Region_Enable
        MCR     p15, 0, r0, c6, c1, 0

; Set up region 2 - RAM and enable
      ;  LDR     r0, = 0x1C000 :OR: (Region_16K << 1) :OR: Region_Enable
      ;  MCR     p15, 0, r0, c6, c2, 0

; Set up region 3 - RAM and enable
        LDR     r0, = 0x20000 :OR: (Region_256K << 1) :OR: Region_Enable
        MCR     p15, 0, r0, c6, c3, 0

; Set up region 4 - DSRAM_SD and enable
;	  MOV     r0, #0x30000
;        LDR     r0, = 0x30000 :OR: (Region_32K <<1):OR:Region_Enable
;        MCR     p15, 0, r0, c6, c4, 0

;
; Set up cacheable /bufferable attributes
       ; MOV     r0, #2_001000               ; cache bits set for SRAM and FLASH
       ; MCR     p15, 0, r0, c2, c0, 0       ; data cacheable

       ; MOV     r0, #2_000010               ; cache bits set for SRAM and FLASH
       ; MCR     p15, 0, r0, c2, c0, 1       ; instr cacheable

       ; MOV     r0, #2_001000               ; bufferable bit set for RAM
       ; MCR     p15, 0, r0, c3, c0, 0       ; sets Write Back Cacheing

; Set up access permissions

        MOV     r0,#2_0011
        ;ORR     r0,r0,#(2_0011 << 4)        ; INS   set to P: RW,    U: RW
	 ORR     r0,r0,#(2_0110 << 4)        ; INS   set to P: RO,    U: RO
        ORR     r0,r0,#(2_0011 << 8)        ; RAM   set to P: RW     U: RW
        ORR     r0,r0,#(2_0011 << 12)       ; RAM   set to P: RW     U: RW
        ;ORR     r0,r0,#(2_0011 << 16)       ; RAM   set to P: RW     U: RW
;
; In this example the access permissions are the same for both instruction and data sides
; Apply these to both instruction and data side
        MCR     p15, 0, r0, c5, c0, 2       ; data AP
        MCR     p15, 0, r0, c5, c0, 3       ; instr AP

;
; Set global core configurations
;===============================
;
        MRC     p15, 0, r0, c1, c0, 0       ; read CP15 register 1
        BIC     r0, r0, #(0x1 <<12)         ; ensure I Cache disabled before MPU enable
        BIC     r0, r0, #(0x1 <<2)          ; enable D Cache disabled before MPU enable
        ORR     r0, r0, #0x1                ; enable MPU bit
        MCR     p15, 0, r0, c1, c0, 0       ; write cp15 register 1
        

;        MRC     p15, 0, r0, c1, c0, 0       ; read CP15 register 1
;        ORR     r0, r0, #(0x1  <<12)        ; enable I Cache
;        ORR     r0, r0, #(0x1  <<2)         ; enable D Cache
;        MCR     p15, 0, r0, c1, c0, 0       ; write CP15 register 1        



		MRC		p15, 0, r0, c2, c0, 0
		ORR		r0, r0, #(0x6)					;region 1,2
		MCR		p15, 0, r0, c2, c0, 0 	 		;enable data cachable bits
			
		MRC	  	p15, 0, r0, c2, c0, 1
		ORR	  	r0, r0, #(0xff)
		MCR	  	p15, 0, r0, c2, c0, 1 	 		;enable Instruction cachable bits

	 	MRC     p15, 0, r0, c1, c0, 0       				; read CP15 register 1
        	ORR     r0, r0, #(0x1  <<12)         			; enable I Cache
        	ORR     r0, r0, #(0x1  <<2)          			; enable D Cache
        	MCR     p15, 0, r0, c1, c0, 0       				; write CP15 register 1
	
		MRC 	p15, 0, r0, c3, c0, 0
		ORR 	r0, r0, #(0xff)
		MCR 	p15, 0, r0, c3, c0, 0			;enable data buffer bits


        IMPORT  __main                      ; import label to __main
        BL       __main                      ; branch to C Library entry

        ENDFUNC

        END
