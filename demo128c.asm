; Program: Fruit Salad
; File: demo128.asm
; Purpose: Show 128 sprites moving simultaneously
; By: Curtis Whitley
;

.org $080D
.segment "STARTUP"
.segment "INIT"
.segment "ONCE"
.segment "CODE"

.feature dollar_in_identifiers

   jmp start      ; skip the data definitions, and go to code

; This file must align on a 16-byte boundary!
.include "pathpts.inc"

.include "x16.inc"
.include "zeropage.inc"
.include "macros.inc"
.include "spritepx.inc"
.include "spritedt.inc"
.include "spritecd.inc"

default_irq_vector: .addr 0

start:
    stz     VERA_ien            ; disable interrupts from VERA
    stz     VERA_ctrl
    stz     VERA_dc_video       ; disable display
    lda     #DISPLAY_SCALE
    sta     VERA_dc_hscale
    sta     VERA_dc_vscale

    lda     #$07 ; bitmap 8bpp
    sta     VERA_L0_config
    lda     #($00000 >> 9)|1  ;VRAM address, 640px
    sta     VERA_L0_tilebase
    stz     VERA_L0_hscroll_l ; horizontal scroll = 0
    stz     VERA_L0_hscroll_h
    stz     VERA_L0_vscroll_l ; vertical scroll = 0
    stz     VERA_L0_vscroll_h

    FILLVRAM $01,$00000,$8000
    FILLVRAM $02,$08000,$8000
    FILLVRAM $03,$10000,$2C00

    stz     ZP_ISR_COLLISIONS           ; assume no collisions
    jsr     init_all_sprite_positions

    RAM2VRAM sprite_bitmap, SPRITE_BITMAP_ADDR, SPRITE_BITMAP_SIZE

    lda     #$71            ; sprites, L1, L0, VGA
    sta     VERA_dc_video

    ; backup default RAM IRQ vector
    lda     IRQVec
    sta     default_irq_vector
    lda     IRQVec+1
    sta     default_irq_vector+1

    ; overwrite RAM IRQ vector with custom handler address
    sei                     ; disable IRQ while vector is changing
    lda     #<custom_irq_handler
    sta     IRQVec
    lda     #>custom_irq_handler
    sta     IRQVec+1
    lda     #VSYNC_BIT;|SPRCOL_BIT
    sta     VERA_ien
    cli                     ; enable IRQ now that vector is properly set

; do nothing in main loop, just let ISR do everything
@main_loop:
    wai
    bra @main_loop          ; never return, just wait for reset


custom_irq_handler:
    lda     VERA_isr                    ; get interrupt information
    bit     #VSYNC_BIT                  ; is this a video synch interrupt?
    beq     continue                    ; go if no
    stz     ZP_ISR_COLLISIONS           ; assume no collisions
    bit     #SPRCOL_BIT                 ; is this a collision interrupt?
    beq     no_collision                ; go if no
    and     #SPRITE_COLL_MASK_F0_ALL    ; keep collision bits only
    sta     ZP_ISR_COLLISIONS           ; save collision bits
no_collision:
    jsr     update_all_sprite_positions
;    lda     #SPRCOL_BIT
;    ???     VERA_isr                    ; clear the sprite collision indicator

continue:
    ; continue to default IRQ handler
    jmp (default_irq_vector)
    ; RTI will happen after jump
