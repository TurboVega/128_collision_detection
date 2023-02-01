; Program: DEMO128C
; File: demo128.asm
; Purpose: Show 128 sprites moving simultaneously, with collision detection
; Copyright (C) 2023 by Curtis Whitley
;

.org $080D
.segment "STARTUP"
.segment "INIT"
.segment "ONCE"
.segment "CODE"

.feature dollar_in_identifiers

   jmp start      ; skip the data definitions, and go to code

.include "pathpts.inc"
.include "x16.inc"
.include "zeropage.inc"
.include "macros.inc"
.include "spritepx.inc"
.include "spritedt.inc"
.include "hit_decisions.inc"
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

    stz     ZP_COLLISION_X              ; assume no collisions in X
    stz     ZP_COLLISION_Y              ; assume no collisions in Y

    jsr     init_all_sprite_positions

    RAM2VRAM sprite_bitmap, SPRITE_BITMAP_ADDR, (SPRITE_BITMAP_SIZE*2)

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
    lda     #VSYNC_BIT|SPRCOL_BIT
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

    stz     ZP_COLLISION_X              ; assume no collisions in X
    stz     ZP_COLLISION_Y              ; assume no collisions in Y

    bit     #SPRCOL_BIT                 ; is this a collision interrupt?
    beq     no_collision                ; go if no
 
    tax                                 ; save ISR value
    and     #SPRITE_COLL_MASK_30_L_R    ; keep X collision bits only
    sta     ZP_COLLISION_X              ; save X collision bits
    txa                                 ; restore ISR value
    and     #SPRITE_COLL_MASK_C0_T_B    ; keep Y collision bits only
    sta     ZP_COLLISION_Y              ; save Y collision bits

no_collision:
    jsr     update_all_sprite_positions ; go move/destroy all sprites

    ; clear the sprite collision indicator
    lda     #SPRCOL_BIT
    sta     VERA_isr

continue:
    ; continue to default IRQ handler
    jmp (default_irq_vector)
    ; RTI will happen after jump
