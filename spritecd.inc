; File: spritecd.inc
; Purpose: Sprite initialization and movement code

; offset allows using Y register to index both structures
OFFSET = SPRITE_X_LO - SPRITE_PATH_X_LO

;-------------------------------------------------------------------
; Initialize all sprite positions
;-------------------------------------------------------------------

; InitSpriteDefinition: set all sprite definition information
;
; This macro expects that ZP_PATH_PTR points to the coordinate pair,
; and the x register equals the current sprite index (0..127).
;
.macro InitSpriteDefinition
    .scope

    lda     #<SPRITE_BITMAP_ADDR_SHIFTED ; get pointer to pixel data (LO)
    sta     VERA_data0                   ; set pointer to pixel data (LO)

    lda     #(>SPRITE_BITMAP_ADDR_SHIFTED)|SPRITE_MODE_8_BPP ; pixel data (HI)
    sta     VERA_data0                   ; set pointer to pixel data (LO)
 
    ldy     #0                  ; offset to path X,Y coordinate
    lda     (ZP_PATH_PTR),y     ; get X (LO) from path table
    sta     VERA_data0          ; set X (LO) in VERA
 
    iny
    lda     (ZP_PATH_PTR),y     ; get X (HI) from path table
    sta     VERA_data0          ; set X (HI) in VERA

    iny
    lda     (ZP_PATH_PTR),y     ; get Y (LO) from path table
    sta     VERA_data0          ; set Y (LO) in VERA
 
    iny
    lda     (ZP_PATH_PTR),y     ; get Y (HI) from path table
    sta     VERA_data0          ; set Y (HI) in VERA
 
    ; check whether this sprite had a collision in the past
    lda     sprite_collision_mask,x ; get the collision mask
    beq     old_collision       ; go if already hit

    ; check whether this sprite had a collision just now
    and     ZP_COLLISION_X      ; get the ISR X bits as of the VSYNC time
    beq     no_collision        ; go if nothing happened
    lda     sprite_collision_mask,x ; get the collision mask
    and     ZP_COLLISION_Y      ; get the ISR Y bits as of the VSYNC time
    bne     new_collision       ; go if something happened

no_collision:
    ; no collision; use mask for next path point
    iny
    lda     (ZP_PATH_PTR),y     ; get collision mask from path table
    sta     VERA_data0          ; set collision mask in VERA
    sta     sprite_collision_mask,x ; set the collision mask
    bra     have_mask 
 
new_collision:
    ; new collision; change the sprite
    stz     sprite_collision_mask,x ; clear the collision mask
old_collision:
    stz     VERA_data0              ; disable the sprite
have_mask:

    .endscope
.endmacro

; InitRing: initialize the sprite data for an entire ring.
;   path:    address of path point array
;   sprtend: sprite index no longer on the path
;   index:   address of ZP start index into path point array
;
.macro InitRing path,sprtend,index
    .scope

    lda     #<(path)            ; get path address (LO)
    sta     ZP_PATH_PTR_LO      ; set path address (LO)
    lda     #>(path)            ; get path address (HI)
    sta     ZP_PATH_PTR_HI      ; set path address (HI)
 
    stz     index               ; zero out index (LO)
    stz     index+1             ; zero out index (HI)

    ; initialize all of the sprites in this ring
init_sprite:
    lda     #SPRITE_ZDEPTH_ABOVE_L1 ; includes empty collision mask
    sta     sprite_collision_mask,x ; set initial sprite state

    InitSpriteDefinition

    lda     #SPRITE_HEIGHT_16_PIXELS|SPRITE_WIDTH_16_PIXELS
    sta     VERA_data0

    ; leave some space between sprites
    lda     ZP_PATH_PTR_LO
    clc
    adc     ZP_NEXT_SPRITE_INC
    sta     ZP_PATH_PTR_LO
    bcc     no_rollover
    inc     ZP_PATH_PTR_HI
no_rollover:

    inx                         ; advance to next sprite index
    cpx     #sprtend            ; done with this ring of sprites?
    bne     init_sprite         ; go if no

    .endscope
.endmacro

init_all_sprite_positions:

    SetupData0 VRAM_sprattr

    ldx     #0                  ; init loop counter (sprite index)
    lda     #28*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 28 path point data structures
    sta     ZP_NEXT_SPRITE_INC  ; save the increment amount

    InitRing sprite_path_0,SPRITE_END_RING_0,ZP_PATH_INDEX_0

    lda     #4*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 4 path point data structures
    sta     ZP_NEXT_SPRITE_INC  ; save the increment amount

    InitRing sprite_path_1,SPRITE_END_RING_1,ZP_PATH_INDEX_1
    InitRing sprite_path_2,SPRITE_END_RING_2,ZP_PATH_INDEX_2
    InitRing sprite_path_3,SPRITE_END_RING_3,ZP_PATH_INDEX_3
    InitRing sprite_path_4,SPRITE_END_RING_4,ZP_PATH_INDEX_4
    InitRing sprite_path_5,SPRITE_END_RING_5,ZP_PATH_INDEX_5
    InitRing sprite_path_6,SPRITE_END_RING_6,ZP_PATH_INDEX_6
    InitRing sprite_path_7,SPRITE_END_RING_7,ZP_PATH_INDEX_7
    rts

; MoveCoordinatesAndMask: set X,Y coordinates and compute collision mask
;
; This macro expects that ZP_PATH_PTR points to the coordinate pair,
; and the x register equals the current sprite index (0..127).
;
.macro MoveCoordinatesAndMask
    .scope

    ; check whether this sprite had a collision in the past
    lda     sprite_collision_mask,x ; get the collision mask
    beq     new_collision       ; go if already hit

    ; check whether this sprite had a collision just now
    and     ZP_COLLISION_X      ; get the ISR X bits as of the VSYNC time
    beq     no_collision        ; go if nothing happened
    lda     sprite_collision_mask,x ; get the collision mask
    and     ZP_COLLISION_Y      ; get the ISR Y bits as of the VSYNC time
    bne     new_collision       ; go if something happened

no_collision:
    ; no collision

    lda     VERA_data0          ; skip 1 byte
    lda     VERA_data0          ; skip 1 byte

    ldy     #0                  ; offset to path X,Y coordinate
    lda     (ZP_PATH_PTR),y     ; get X (LO) from path table
    sta     VERA_data0          ; set X (LO) in VERA
 
    iny
    lda     (ZP_PATH_PTR),y     ; get X (HI) from path table
    sta     VERA_data0          ; set X (HI) in VERA

    iny
    lda     (ZP_PATH_PTR),y     ; get Y (LO) from path table
    sta     VERA_data0          ; set Y (LO) in VERA
 
    iny
    lda     (ZP_PATH_PTR),y     ; get Y (HI) from path table
    sta     VERA_data0          ; set Y (HI) in VERA
 
    ; use mask for next path point
    iny
    lda     (ZP_PATH_PTR),y     ; get collision mask from path table
    sta     VERA_data0          ; set collision mask in VERA
    sta     sprite_collision_mask,x ; set the collision mask
    bra     have_mask 
 
new_collision:
    lda     #<SPRITE_HIT_BITMAP_ADDR_SHIFTED ; get pointer to pixel data (LO)
    sta     VERA_data0                   ; set pointer to pixel data (LO)

    lda     #(>SPRITE_HIT_BITMAP_ADDR_SHIFTED)|SPRITE_MODE_8_BPP ; pixel data (HI)
    sta     VERA_data0                   ; set pointer to pixel data (LO)

    ldy     #0                  ; offset to path X,Y coordinate
    lda     (ZP_PATH_PTR),y     ; get X (LO) from path table
    sta     VERA_data0          ; set X (LO) in VERA
 
    iny
    lda     (ZP_PATH_PTR),y     ; get X (HI) from path table
    sta     VERA_data0          ; set X (HI) in VERA

    iny
    lda     (ZP_PATH_PTR),y     ; get Y (LO) from path table
    sta     VERA_data0          ; set Y (LO) in VERA
 
    iny
    lda     (ZP_PATH_PTR),y     ; get Y (HI) from path table
    sta     VERA_data0          ; set Y (HI) in VERA
 
    ; new collision; change the sprite
    lda     #SPRITE_ZDEPTH_ABOVE_L1 ; includes empty collision mask
    sta     sprite_collision_mask,x ; set initial sprite state
    stz     VERA_data0              ; disable the sprite
have_mask:

    .endscope
.endmacro

;-------------------------------------------------------------------
; Update (move) all sprite positions
;-------------------------------------------------------------------

; MoveRing: move the sprite positions for an entire ring.
;   path:    address of path point array
;   end:     address after end of path point array
;   sprtend: sprite index no longer on the path
;   index:   address of ZP start index into path point array
;   points:  number of X,Y coordinates in the path point array
;
.macro MoveRing path,end,sprtend,index,points
    .scope

    ; increment the array index
    lda     index               ; get path array index (LO)
    inc                         ; advance to next index
    sta     index               ; set path array index (LO)
    bne     no_index_wrap       ; go if no overflow
    inc     index+1             ; update array index (HI)
no_index_wrap:

    ; check for end-of-ring
    cmp     #<(points)          ; compare next index (LO)
    bne     not_eor             ; go if not end-of-ring
    lda     index+1             ; get next index (HI)
    cmp     #>(points)          ; compare next index (HI)
    bne     not_eor             ; go if not end-of-ring
    stz     index               ; restart the index (LO)
    stz     index+1             ; restart the index (HI)
not_eor:

    ; multiply the path index by  (size of one X,Y point with masks)
    lda     index+1             ; get index (HI)
    sta     ZP_TEMP_INDEX_HI    ; save temporary index (HI)
    lda     index               ; get index (LO)
    asl                         ; multiply index by 2
    rol     ZP_TEMP_INDEX_HI
    asl                         ; multiply index by 2
    rol     ZP_TEMP_INDEX_HI
    asl                         ; multiply index by 2
    rol     ZP_TEMP_INDEX_HI
    clc                         ; prepare to add index to index*4
    adc     index               ; add index (LO)
    sta     ZP_TEMP_INDEX_LO    ; save temporary index (LO)
    lda     ZP_TEMP_INDEX_HI    ; get new index (HI)
    adc     index+1             ; get index (HI)
    sta     ZP_TEMP_INDEX_HI    ; update new index (HI)

    ; add to base path address to yield address in path
    lda     ZP_TEMP_INDEX_LO    ; get temporary index (LO)
    clc
    adc     #<(path)            ; add path address (LO)
    sta     ZP_PATH_PTR_LO      ; set path address (LO)
    lda     ZP_TEMP_INDEX_HI    ; get index (HI)
    adc     #>(path)            ; add path address (HI)
    sta     ZP_PATH_PTR_HI      ; set path address (HI)
 
    ; move all of the sprites in this ring
move_sprite:
     MoveCoordinatesAndMask

    lda     VERA_data0          ; skip 1 byte
 
    ; leave some space between sprites
    lda     ZP_PATH_PTR_LO
    clc
    adc     ZP_NEXT_SPRITE_INC
    sta     ZP_PATH_PTR_LO
    bcc     no_rollover
    inc     ZP_PATH_PTR_HI
no_rollover:
 
    ; if the next path address goes past the end of the array, adjust it
    lda     ZP_PATH_PTR_HI      ; get new address (HI)
    cmp     #>(end)             ; compare new address (HI)
    bne     decide              ; go if no need to check LO
    lda     ZP_PATH_PTR_LO      ; get new address (LO)
    cmp     #<(end)             ; compare new address (LO)
decide:
    bcc     no_path_wrap        ; go if not past end of array
 
    ; get difference between next address and end of array
    ; note: carry bit is already set, so we don't set it here

    lda     ZP_PATH_PTR_LO      ; get new address (LO)
    sbc     #<(end)             ; subtract path end (LO)
    sta     ZP_PATH_PTR_LO      ; save difference (LO)
    lda     ZP_PATH_PTR_HI      ; get next path address (HI)
    sbc     #>(end)             ; subtract path end (HI)
    sta     ZP_PATH_PTR_HI      ; save difference (HI)
 
    ; add the difference to the start of the path array

    clc
    lda     ZP_PATH_PTR_LO      ; get difference (LO)
    adc     #<(path)            ; add to start of path array (LO)
    sta     ZP_PATH_PTR_LO      ; save new path address (LO)
    lda     ZP_PATH_PTR_HI      ; get difference (HI)
    adc     #>(path)            ; add to start of path array (HI)
    sta     ZP_PATH_PTR_HI      ; save new path address (HI)
no_path_wrap:

    inx                         ; advance to next sprite index
    cpx     #sprtend            ; done with this ring of sprites?
    beq     move_done           ; go if yes
    jmp     move_sprite         ; go if no
move_done:

    .endscope
.endmacro

RING_0_POINTS = ((end_sprite_path_0 - sprite_path_0) / SPRITE_PATH_STRUCT_SIZE)
RING_1_POINTS = ((end_sprite_path_1 - sprite_path_1) / SPRITE_PATH_STRUCT_SIZE)
RING_2_POINTS = ((end_sprite_path_2 - sprite_path_2) / SPRITE_PATH_STRUCT_SIZE)
RING_3_POINTS = ((end_sprite_path_3 - sprite_path_3) / SPRITE_PATH_STRUCT_SIZE)
RING_4_POINTS = ((end_sprite_path_4 - sprite_path_4) / SPRITE_PATH_STRUCT_SIZE)
RING_5_POINTS = ((end_sprite_path_5 - sprite_path_5) / SPRITE_PATH_STRUCT_SIZE)
RING_6_POINTS = ((end_sprite_path_6 - sprite_path_6) / SPRITE_PATH_STRUCT_SIZE)
RING_7_POINTS = ((end_sprite_path_7 - sprite_path_7) / SPRITE_PATH_STRUCT_SIZE)

update_all_sprite_positions:

    SetupData0 VRAM_sprattr

    ldx     #0                  ; init loop counter (sprite index)
    lda     #28*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 28 path point data structures
    sta     ZP_NEXT_SPRITE_INC  ; save the increment amount

    MoveRing sprite_path_0,end_sprite_path_0,SPRITE_END_RING_0,ZP_PATH_INDEX_0,RING_0_POINTS

    lda     #4*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 4 path point data structures
    sta     ZP_NEXT_SPRITE_INC  ; save the increment amount

    MoveRing sprite_path_1,end_sprite_path_1,SPRITE_END_RING_1,ZP_PATH_INDEX_1,RING_1_POINTS
    MoveRing sprite_path_2,end_sprite_path_2,SPRITE_END_RING_2,ZP_PATH_INDEX_2,RING_2_POINTS
    MoveRing sprite_path_3,end_sprite_path_3,SPRITE_END_RING_3,ZP_PATH_INDEX_3,RING_3_POINTS
    MoveRing sprite_path_4,end_sprite_path_4,SPRITE_END_RING_4,ZP_PATH_INDEX_4,RING_4_POINTS
    MoveRing sprite_path_5,end_sprite_path_5,SPRITE_END_RING_5,ZP_PATH_INDEX_5,RING_5_POINTS
    MoveRing sprite_path_6,end_sprite_path_6,SPRITE_END_RING_6,ZP_PATH_INDEX_6,RING_6_POINTS
    MoveRing sprite_path_7,end_sprite_path_7,SPRITE_END_RING_7,ZP_PATH_INDEX_7,RING_7_POINTS
    rts
