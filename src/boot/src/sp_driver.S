;******************************************************************************
;* $Revision: 1153 $
;* $Date: 2007-12-18 09:48:23 +0100 (ti, 18 des 2007) $
;*
;* Copyright (c) 2007, Atmel Corporation All rights reserved.
;*
;* Redistribution and use in source and binary forms, with or without
;* modification, are permitted provided that the following conditions are met:
;*
;* 1. Redistributions of source code must retain the above copyright notice,
;* this list of conditions and the following disclaimer.
;*
;* 2. Redistributions in binary form must reproduce the above copyright notice,
;* this list of conditions and the following disclaimer in the documentation
;* and/or other materials provided with the distribution.
;*
;* 3. The name of ATMEL may not be used to endorse or promote products derived
;* from this software without specific prior written permission.
;*
;* THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
;* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
;* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
;* SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
;* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
;* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
;* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
;* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
;* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;******************************************************************************
;*
;* XMEGA Self-programming driver assembly source file.
;*
;*      This file contains the low-level implementations for the
;*      XMEGA Self-programming driver. It is written for the GCC Assembler.
;*
;*      If any SPM instructions are used, the linker file must define a segment
;*      named bootloader which must be located in the device Boot section.
;*      This can be done by passing "-Wl,--section-start=.BOOT=0x020000" to the
;*      linker with the correct address for the boot section.
;*
;*      None of these routines clean up the NVM Command Register after use. It
;*      is therefore important to write NVM_CMD_NO_OPERATION_gc (0x00) to this
;*      register when finished using any of the functions in this driver.
;*
;*      For all routines, it is important that any interrupt handlers do not
;*      perform NVM operations. The user must implement a scheme for mutually
;*      exclusive access to NVM. However, the 4-cycle timeout will work fine,
;*      since writing to the Configuration Change Protection register (CCP)
;*      automatically disables interrupts for 4 instruction cycles.
;*
;*      Note on IAR calling convention:
;*         Scratch registers:   R18-R27, R30-R31
;*         Preserved registers: R2-R17, R28-R29
;*         Parameter registers: R8-R25 (2-,4-, or 8- byte alignment)
;*         Return registers:    R18-R25 (up to 64-bit)
;*
;* Application note:
;*      AVR1316: XMEGA Self-programming
;*
;* Documentation
;*      For comprehensive code documentation, supported compilers, compiler
;*      settings and supported devices see readme.html
;*
;*      Atmel Corporation: http:;www.atmel.com \n
;*      Support email: avr@atmel.com

#include <avr/io.h>

; Defines not yet included in header file.
#define NVM_CMD_NO_OPERATION_gc            0x00
#define NVM_CMD_READ_USER_SIG_ROW_gc       0x01
#define NVM_CMD_READ_CALIB_ROW_gc          0x02
#define NVM_CMD_READ_EEPROM_gc             0x06
#define NVM_CMD_READ_FUSES_gc              0x07
#define NVM_CMD_WRITE_LOCK_BITS_gc         0x08
#define NVM_CMD_ERASE_USER_SIG_ROW_gc      0x18
#define NVM_CMD_WRITE_USER_SIG_ROW_gc      0x1a
#define NVM_CMD_ERASE_APP_gc               0x20
#define NVM_CMD_ERASE_APP_PAGE_gc          0x22
#define NVM_CMD_LOAD_FLASH_BUFFER_gc       0x23
#define NVM_CMD_WRITE_APP_PAGE_gc          0x24
#define NVM_CMD_ERASE_WRITE_APP_PAGE_gc    0x25
#define NVM_CMD_ERASE_FLASH_BUFFER_gc      0x26
#define NVM_CMD_ERASE_BOOT_PAGE_gc         0x2a
#define NVM_CMD_WRITE_BOOT_PAGE_gc         0x2c
#define NVM_CMD_ERASE_WRITE_BOOT_PAGE_gc   0x2d
#define NVM_CMD_ERASE_EEPROM_gc            0x30
#define NVM_CMD_ERASE_EEPROM_PAGE_gc       0x32
#define NVM_CMD_LOAD_EEPROM_BUFFER_gc      0x33
#define NVM_CMD_WRITE_EEPROM_PAGE_gc       0x34
#define NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc 0x35
#define NVM_CMD_ERASE_EEPROM_BUFFER_gc     0x36
#define NVM_CMD_APP_CRC_gc                 0x38
#define NVM_CMD_BOOT_CRC_gc                0x39
#define NVM_CMD_FLASH_RANGE_CRC_gc         0x3a
#define CCP_SPM_gc                         0x9d
#define CCP_IOREG_gc                       0xd8


; Reads a byte from flash given by the address in R25:R24:R23:R22.
;
; Input: R25:R24:R23:R22.
; Returns: R24 - Read byte.
.section .text
.global SP_ReadByte

SP_ReadByte:
    in   r19, RAMPZ      ; Save RAMPZ.
    out  RAMPZ, r24      ; Load RAMPZ with the MSB of the address.
    movw ZL, r22         ; Move the low bytes to the Z pointer
    elpm r24, Z          ; Extended load byte from address pointed to by Z.
    out  RAMPZ, r19      ; Restore RAMPZ register.
    ret


; Reads a word from flash given by the address in R25:R24:R23:R22.
;
; Input: R25:R24:R23:R22.
; Returns: R25:R24 - Read word.
.section .text
.global SP_ReadWord

SP_ReadWord:
    in   r19, RAMPZ      ; Save RAMPZ.
    out  RAMPZ, r24      ; Load RAMPZ with the MSB of the address.
    movw ZL, r22         ; Move the low bytes to the Z pointer
    elpm r24, Z+         ; Extended load byte from address pointed to by Z.
    elpm r25, Z          ; Extended load byte from address pointed to by Z.
    out  RAMPZ, r19      ; Restore RAMPZ register.
    ret


; Reads the calibration byte given by the index in R24.
;
; Input: R24 - Byte index.
; Returns: R24 - Calibration byte.
.section .text
.global SP_ReadCalibrationByte

SP_ReadCalibrationByte:
    ldi  r20, NVM_CMD_READ_CALIB_ROW_gc    ; Prepare NVM command in R20.
    rjmp SP_CommonLPM                      ; Jump to common LPM code.


; Reads the user signature byte given by the index in R25:R24.
;
; Input: R25:R24 - Byte index.
; Returns: R24 - Signature byte.
.section .text
.global SP_ReadUserSignatureByte

SP_ReadUserSignatureByte:
    ldi  r20, NVM_CMD_READ_USER_SIG_ROW_gc  ; Prepare NVM command in R20.
    rjmp SP_CommonLPM                       ; Jump to common LPM code.


; Reads the fuse byte given by the index in R24.
;
; Input: R24 - Byte index.
; Returns: R24 - Fuse byte.
.section .text
.global SP_ReadFuseByte

SP_ReadFuseByte:
    sts   NVM_ADDR0, r24             ; Load fuse index into NVM Address Reg 0.
    clr   r24                        ; Prepare a zero.
    sts   NVM_ADDR1, r24             ; Load zero into NVM Address Register 1.
    sts   NVM_ADDR2, r24             ; Load zero into NVM Address Register 2.
    ldi   r20, NVM_CMD_READ_FUSES_gc ; Prepare NVM command in R20.
    rcall SP_CommonCMD               ; Jump to common NVM Action code.
    movw  r24, r22                   ; Move low byte to 1 byte return address.
    ret


; Erases the user signature row.
.section .text
.global SP_EraseUserSignatureRow

SP_EraseUserSignatureRow:
    in  r19, RAMPZ                       ; Save RAMPZ, restored in SP_CommonSPM.
    ldi r20, NVM_CMD_ERASE_USER_SIG_ROW_gc ; Prepare NVM command in R20.
    jmp SP_CommonSPM                       ; Jump to common SPM code.


; Writes the flash buffer to the user signature row.
.section .text
.global SP_WriteUserSignatureRow

SP_WriteUserSignatureRow:
    in  r19, RAMPZ                       ; Save RAMPZ, restored in SP_CommonSPM.
    ldi r20, NVM_CMD_WRITE_USER_SIG_ROW_gc  ; Prepare NVM command in R20.
    jmp SP_CommonSPM                        ; Jump to common SPM code.


; Erases the entire application section.
.section .text
.global SP_EraseApplicationSection

SP_EraseApplicationSection:
    in  r19, RAMPZ                 ; Save RAMPZ, restored in SP_CommonSPM.
    clr r24                        ; Prepare a zero.
    clr r25
    out RAMPZ, r24                 ; Point into Application area.
    ldi r20, NVM_CMD_ERASE_APP_gc  ; Prepare NVM command in R20.
    jmp SP_CommonSPM               ; Jump to common SPM code.


; Writes the word from R23:R22 into the Flash page buffer at address R25:R24.
;
; Input:
;     R25:R24 - Byte address into Flash page.
;     R23:R22 - Word to write.
.section .text
.global SP_LoadFlashWord

SP_LoadFlashWord:
    in   r19, RAMPZ                      ; Save RAMPZ, restored in SP_CommonSPM.
    movw r0, r22                            ; Prepare flash word in R1:R0.
    ldi  r20, NVM_CMD_LOAD_FLASH_BUFFER_gc  ; Prepare NVM command in R20.
    jmp  SP_CommonSPM                       ; Jump to common SPM code.


; Writes an entire page from the SRAM buffer at
; address R25:R24 into the Flash page buffer.
;
; Note that you must define "-Wl,--section-start=.BOOT=0x020000" for the
; linker to place this function in the boot section with the correct address.
;
; Input: R25:R24 - 16-bit pointer to SRAM buffer.
.section .text
.global SP_LoadFlashPage

SP_LoadFlashPage:
    clr  ZL              ; Clear low byte of Z, to indicate start of page.
    clr  ZH              ; Clear high byte of Z, to indicate start of page.

    out  RAMPX, r1       ; Clear RAMPX pointer.
    movw XL, r24         ; Load X with data buffer address.

    ldi r20, NVM_CMD_LOAD_FLASH_BUFFER_gc  ; Prepare NVM command code in R20.
    sts NVM_CMD, r20                       ; Load it into NVM command register.

#if APP_SECTION_PAGE_SIZE > 512
    ldi r22, ((APP_SECTION_PAGE_SIZE / 2) >> 8)
#endif
    ldi r21, ((APP_SECTION_PAGE_SIZE / 2) & 0xff) ; Load R21 page word count.
    ldi r18, CCP_SPM_gc                  ; Prepare Protect SPM signature in R16.

SP_LoadFlashPage_1:
    ld  r0, X+        ; Load low byte from buffer into R0.
    ld  r1, X+        ; Load high byte from buffer into R1.
    sts CCP, r18      ; Enable SPM operation (disables interrupts for 4 cycles).
    spm               ; Self-program.
    adiw ZL, 2        ; Move Z to next Flash word.

#if APP_SECTION_PAGE_SIZE > 512
    subi r21, 1         ; Decrement word count.
    sbci r22, 0
#else
    dec r21             ; Decrement word count.
#endif

    brne SP_LoadFlashPage_1  ; Repeat until word cont is zero.
    clr r1                   ; Clear R1 for GCC _zero_reg_ to function properly.
    ret


; Writes the page buffer to Flash at address R25:R24:R23:R22
; in the application section. The address can point anywhere inside the page.
;
; Input: R25:R24:R23:R22 - Byte address into Flash page.
.section .text
.global SP_WriteApplicationPage

SP_WriteApplicationPage:
    in   r19, RAMPZ            ; Save RAMPZ, restored in SP_CommonSPM.
    out  RAMPZ, r24            ; Load RAMPZ with the MSB of the address.
    movw r24, r22              ; Move low bytes of address to ZH:ZL from R23:R22
    ldi  r20, NVM_CMD_WRITE_APP_PAGE_gc   ; Prepare NVM command in R20.
    jmp  SP_CommonSPM                     ; Jump to common SPM code.


; Erases first and then writes the page buffer to the
; Flash page at address R25:R24:R23:R22 in the application section. The address
; can point anywhere inside the page.
;
; Input: R25:R24:R23:R22 - Byte address into Flash page.
.section .text
.global SP_EraseWriteApplicationPage

SP_EraseWriteApplicationPage:
    in   r19, RAMPZ            ; Save RAMPZ, restored in SP_CommonSPM.
    out  RAMPZ, r24            ; Load RAMPZ with the MSB of the address.
    movw r24, r22              ; Move low bytes of address to ZH:ZL from R23:R22
    ldi  r20, NVM_CMD_ERASE_WRITE_APP_PAGE_gc  ; Prepare NVM command in R20.
    jmp  SP_CommonSPM                          ; Jump to common SPM code.



; Locks all further access to SPM operations until next reset.
.section .text
.global SP_LockSPM

SP_LockSPM:
    ldi r18, CCP_IOREG_gc     ; Prepare Protect IO-register signature in R18.
    sts CCP, r18              ; Enable IO-register operation
                              ; (disables interrupts for 4 cycles).
    ldi r18, NVM_SPMLOCK_bm   ; Prepare bitmask for locking SPM into R18.
    sts NVM_CTRLB, r18        ; Load bitmask into NVM Control Register B,
                              ; which locks SPM.
    ret


; Wait for the SPM to finish and clears the command register.
;
; Note that this routine is blocking, and will halt any execution until the SPM
; is finished.
.section .text
.global SP_WaitForSPM

SP_WaitForSPM:
    lds  r18, NVM_STATUS     ; Load the NVM Status register.
    sbrc r18, NVM_NVMBUSY_bp ; Check if bit is cleared.
    rjmp SP_WaitForSPM       ; Repeat check if bit is not cleared.
    clr  r18
    sts  NVM_CMD, r18        ; Clear up command register to NO_OPERATION.
    ret


; Called by several other routines, and contains common code
; for executing an NVM command, including the return statement itself.
;
; If the operation (NVM command) requires the NVM Address registers to be
; prepared, this must be done before jumping to this routine.
;
; Note that R25:R24:R23:R22 is used for returning results, even if the
; C-domain calling function only expects a single byte or even void.
;
; Input: R20 - NVM Command code.
; Returns: R25:R24:R23:R22 - 32-bit result from NVM operation.
.section .text

SP_CommonCMD:
    sts NVM_CMD, r20        ; Load command into NVM Command register.
    ldi r18, CCP_IOREG_gc   ; Prepare Protect IO-register signature in R18.
    ldi r19, NVM_CMDEX_bm   ; Prepare bitmask for setting NVM Command Execute
                            ; bit into R19.
    sts CCP, r18            ; Enable IO-register operation
                            ; (disables interrupts for 4 cycles).
    sts NVM_CTRLA, r19      ; Load bitmask into NVM Control Register A,
                            ; which executes the command.
    lds r22, NVM_DATA0      ; Load NVM Data Register 0 into R22.
    lds r23, NVM_DATA1      ; Load NVM Data Register 1 into R23.
    lds r24, NVM_DATA2      ; Load NVM Data Register 2 into R24.
    clr r25                 ; Clear R25 in order to return a clean 32-bit value.
    ret


; Called by several other routines, and contains common code
; for executing an LPM command, including the return statement itself.
;
; Note that R24 is used for returning results, even if the
; C-domain calling function expects a void.
;
; Input:
;     R25:R24 - Low bytes of Z pointer.
;     R20     - NVM Command code.
;
; Returns: R24     - Result from LPM operation.
.section .text

SP_CommonLPM:
    movw ZL, r24             ; Load index into Z.
    sts  NVM_CMD, r20        ; Load prepared command into NVM Command register.
    lpm  r24,Z
    ret


; Called by several other routines, and contains common code
; for executing an SPM command, including the return statement itself.
;
; If the operation (SPM command) requires the R1:R0 registers to be
; prepared, this must be done before jumping to this routine.
;
; Note that you must define "-Wl,--section-start=.BOOT=0x020000" for the
; linker to place this function in the boot section with the correct address.
;
; Input:
;     R1:R0    - Optional input to SPM command.
;     R25:R24  - Low bytes of Z pointer.
;     R20      - NVM Command code.
.section .text

SP_CommonSPM:
    movw ZL, r24         ; Load R25:R24 into Z.
    sts  NVM_CMD, r20    ; Load prepared command into NVM Command register.
    ldi  r18, CCP_SPM_gc ; Prepare Protect SPM signature in R18
    sts  CCP, r18     ; Enable SPM operation (disables interrupts for 4 cycles).
    spm                  ; Self-program.
    clr  r1              ; Clear R1 for GCC _zero_reg_ to function properly.
    out  RAMPZ, r19      ; Restore RAMPZ register.
    ret
