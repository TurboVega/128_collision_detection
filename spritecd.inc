; File: spritecd.inc
; Purpose: Sprite initialization and movement code
; Copyright (c) 2023 by Curtis Whitley

;-------------------------------------------------------------------
; Initialize all sprite positions
;-------------------------------------------------------------------
; CopySpriteXYFields: copy some array fields from RAM to VRAM
;
; This macro copies the coordinates from a sprite data structure.
;
.macro CopySpriteXYFields
    .scope

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

    .endscope
.endmacro

; InitSpriteDefinition: set all sprite definition information
;
; This macro expects that ZP_PATH_PTR points to the coordinate pair,
; and the x register equals the current sprite index (0..127).
;
.macro InitSpriteDefinition
    .scope

    cpx     #SPRITE_RING_COUNT_0        ; are we doing the first ring?
    bmi     first_ring                  ; go if yes
    lda     #<SPRITE_TARGET_BITMAP_ADDR_SHIFTED ; get pointer to pixel data (LO)
    ldy     #(>SPRITE_TARGET_BITMAP_ADDR_SHIFTED)|SPRITE_MODE_8_BPP ; pixel data (HI)
    bra     set_address
first_ring:
    lda     #<SPRITE_PROJECTILE_BITMAP_ADDR_SHIFTED ; get pointer to pixel data (LO)
    ldy     #(>SPRITE_PROJECTILE_BITMAP_ADDR_SHIFTED)|SPRITE_MODE_8_BPP ; pixel data (HI)
set_address:
    sta     VERA_data0                   ; set pointer to pixel data (LO)
    sty     VERA_data0                   ; set pointer to pixel data (HI)
 
    CopySpriteXYFields
 
    iny
    lda     (ZP_PATH_PTR),y     ; get collision mask from path table
    sta     VERA_data0          ; set collision mask in VERA
    sta     sprite_collision_mask,x ; set the collision mask

    lda     #SPRITE_HEIGHT_16_PIXELS|SPRITE_WIDTH_16_PIXELS
    sta     VERA_data0          ; set sprite width & height

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
    InitSpriteDefinition

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
    lda     #27*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 27 path point data structures
    sta     ZP_NEXT_SPRITE_INC  ; save the increment amount

    InitRing sprite_path_0,SPRITE_END_RING_0,ZP_PATH_INDEX_0

    lda     #9*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 9 path point data structures
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
;   path:    address of path point array
;   end:     address after end of path point array
;
; This macro expects that ZP_PATH_PTR points to the path structure,
; and the x register equals the current sprite index (0..127).
;
.macro MoveCoordinatesAndMask path,end
    .scope

    ; check whether this sprite had a collision in the past
    lda     sprite_collision_mask,x ; get the old collision mask
    cmp     #SPRITE_ZDEPTH_ABOVE_L1 ; is this a dead sprite?
    bne     sprite_active           ; go if no (still alive)
    jmp     some_collision          ; go if already hit
sprite_active:

    ; check whether this sprite had a quadrant collision just now
    tay                                 ; copy sprite collision mask
    and     ZP_COLLISION_X              ; compare to ISR X bits as of the VSYNC time
    beq     keep_rolling2               ; go if nothing happened
    tya                                 ; restore sprite collision mask
    and     ZP_COLLISION_Y              ; compare to ISR Y bits as of the VSYNC time
    beq     keep_rolling2               ; go if nothing happened

    ; this sprite is involved in a collision in some quadrant
    ; we will need its old (prior) path structure pointer
    lda     ZP_PATH_PTR_HI              ; get pointer to path structure (HI)
    cmp     #>(path)                    ; compare to start of path (HI)
    bne     not_start                   ; go if not start of path
    lda     ZP_PATH_PTR_LO              ; get pointer to path structure (LO)
    cmp     #<(path)                    ; compare to start of path (LO)
    bne     not_start                   ; go if not start of path

    ; the old (prior) path pointer is at the end of the path array
    lda     #<(end-SPRITE_PATH_STRUCT_SIZE) ; point to last array item (LO)
    sta     ZP_OLD_PATH_PTR_LO          ; save pointer to last item (LO)
    lda     #>(end-SPRITE_PATH_STRUCT_SIZE) ; point to last array item (HI)
    sta     ZP_OLD_PATH_PTR_HI          ; save pointer to last item (HI)
    bra     use_old_path_ptr

keep_rolling2:
    jmp     keep_rolling

not_start:
    ; the old (prior) path pointer is one slot prior to the current pointer
    lda     ZP_PATH_PTR_LO              ; get pointer to path structure (LO)
    sec                                 ; prepare to subtract structure size
    sbc     #<SPRITE_PATH_STRUCT_SIZE   ; subtract structure size (LO)
    sta     ZP_OLD_PATH_PTR_LO          ; save pointer to prior item (LO)
    lda     ZP_PATH_PTR_HI              ; get pointer to path structure (HI)
    sbc     #>SPRITE_PATH_STRUCT_SIZE   ; subtract structure size (HI)
    sta     ZP_OLD_PATH_PTR_HI          ; save pointer to prior item (HI)

use_old_path_ptr:

    .if .not(.defined(TARGET))

    ; this sprite is a projectile, not a target
 
    ; save a pointer to this projectile, for use when moving targets
    lda     ZP_OLD_PATH_PTR_LO          ; get old pointer to path structure (LO)
    sta     (ZP_QUAD_PROJ_ARRAY_PTR)    ; save old pointer to path structure (LO)
    ldy     #1                          ; offset to HI part
    lda     ZP_OLD_PATH_PTR_HI          ; get old pointer to path structure (HI)
    sta     (ZP_QUAD_PROJ_ARRAY_PTR),y  ; save old pointer to path structure (HI)
    inc     ZP_QUAD_PROJ_CNT            ; count quadrant projectiles discovered

    lda     ZP_QUAD_PROJ_ARRAY_PTR_LO   ; advance array item pointer (LO)
    clc
    adc     #2                          ; sizeof a pointer
    sta     ZP_QUAD_PROJ_ARRAY_PTR_LO   ; set array item pointer (LO)
    lda     ZP_QUAD_PROJ_ARRAY_PTR_HI   ; advance array item pointer (HI)
    adc     #0
    sta     ZP_QUAD_PROJ_ARRAY_PTR_HI   ; set array item pointer (HI)

    ; fall through to keep_rolling
 
    .endif

    .if .defined(TARGET)

    ; this sprite is a target, not a projectile

    ; check whether this sprite had a spatial collision just now
    ; (loop through quadrant projectiles and compare spatial masks)

    lda     ZP_QUAD_PROJ_CNT                ; get number of quadrant projectiles discovered
    beq     keep_rolling2                   ; go if no projectile has a collision
    sta     ZP_QUAD_PROJ_INDEX              ; set loop index for traversing quadrant projectiles
    lda     #<quadrant_projectiles          ; get start of projectile array (LO)
    sta     ZP_QUAD_PROJ_ARRAY_PTR_LO       ; set start of projectile array (LO)
    lda     #>quadrant_projectiles          ; get start of projectile array (HI)
    sta     ZP_QUAD_PROJ_ARRAY_PTR_HI       ; set start of projectile array (HI)
 
check_spatial_collision:
    lda     (ZP_QUAD_PROJ_ARRAY_PTR)        ; get pointer to projectile sprite (LO)
    sta     ZP_PROJECTILE_PTR_LO            ; set pointer to projectile sprite (LO)
    ldy     #1                              ; offset to HI part
    lda     (ZP_QUAD_PROJ_ARRAY_PTR),y      ; get pointer to projectile sprite (HI)
    sta     ZP_PROJECTILE_PTR_HI            ; set pointer to projectile sprite (HI)

    lda     ZP_QUAD_PROJ_ARRAY_PTR_LO       ; advance array item pointer (LO)
    clc
    adc     #2                              ; sizeof a pointer
    sta     ZP_QUAD_PROJ_ARRAY_PTR_LO       ; set array item pointer (LO)
    lda     ZP_QUAD_PROJ_ARRAY_PTR_HI       ; advance array item pointer (HI)
    adc     #0
    sta     ZP_QUAD_PROJ_ARRAY_PTR_HI       ; set array item pointer (HI)

    ldy     #SPRITE_PATH_SPATIAL_MASK_X_LO   ; offset into path structure for mask X (LO)
    lda     (ZP_OLD_PATH_PTR),y              ; get target spatial mask X (LO)
    and     (ZP_PROJECTILE_PTR),y            ; compare to projectile spatial mask X (LO) 
    beq     check_x_hi                       ; go if no collision in X (LO)
    iny                                      ; offset into path structure for mask X (HI)
    bra     x_collision                      ; go if sprites collide in X
check_x_hi:
    iny                                      ; offset into path structure for mask X (HI)
    lda     (ZP_OLD_PATH_PTR),y              ; get target spatial mask X (HI)
    and     (ZP_PROJECTILE_PTR),y            ; compare to projectile spatial mask X (HI) 
    beq     next_quad_sprite                 ; go if sprites do not collide in X
x_collision:

    iny                                      ; offset into path structure for mask Y (LO)
    lda     (ZP_OLD_PATH_PTR),y              ; get target spatial mask Y (LO)
    and     (ZP_PROJECTILE_PTR),y            ; compare to projectile spatial mask Y (LO) 
    bne     y_collision                      ; go if sprites collide in Y, spatially
    iny                                      ; offset into path structure for mask Y (HI)
    lda     (ZP_OLD_PATH_PTR),y              ; get target spatial mask Y (HI)
    and     (ZP_PROJECTILE_PTR),y            ; compare to projectile spatial mask Y (HI) 
    bne     y_collision                      ; go if sprites collide in Y, spatially

next_quad_sprite:
    dec     ZP_QUAD_PROJ_INDEX               ; countdown quadrant projectile sprites
    bne     check_spatial_collision          ; go back if more projectiles to check
    bra     keep_rolling2                    ; there is no collision
y_collision:

    ; check for a collision at the pixel level
    ; given target coordinates (tx, ty) via ZP_OLD_PATH_PTR
    ; given projectile coordinates (px, py) via ZP_PROJECTILE_PTR
    ;
    ; dy = ty - py + 16
    ; if (dy & $E0), stop (no collision)
    ;
    ; dx = tx - px + 16
    ; if (dx & $E0), stop (no collision)
    ;

    ; compute and adjust delta Y (DY)

    ldy     #SPRITE_PATH_Y_LO                ; offset to Y (LO) in path structure
    lda     (ZP_OLD_PATH_PTR),y              ; get target Y (LO)
    sec
    sbc     (ZP_PROJECTILE_PTR),y            ; subtract projectile Y (LO)
    sta     ZP_DELTA_Y_LO                    ; save delta Y (LO)
    iny                                      ; offset to Y (HI) in path structure
    lda     (ZP_OLD_PATH_PTR),y              ; get target Y (HI)
    sbc     (ZP_PROJECTILE_PTR),y            ; subtract projectile Y (HI)
    sta     ZP_DELTA_Y_HI                    ; save delta Y (HI)

    lda     ZP_DELTA_Y_LO                    ; load delta Y (LO)
    clc
    adc     #16                              ; add 16 to delta Y
    sta     ZP_DELTA_Y_LO                    ; save delta Y (LO)
    sta     ZP_TEMP_INDEX_LO                 ; save temporary index (LO)
    bcc     no_delta_wrap                    ; go if no overflow
    inc     ZP_DELTA_Y_HI                    ; save delta Y (HI)
no_delta_wrap:
    lda     ZP_DELTA_Y_HI                    ; load delta Y (HI)
    sta     ZP_TEMP_INDEX_HI                 ; save temporary index (HI)
    bne     keep_rolling                     ; go if DY >= 256, or < 0
    lda     ZP_DELTA_Y_LO                    ; get delta Y (LO)
    and     #$E0                             ; make sure it's below 32
    bne     keep_rolling                     ; go if DY >= 32, or < 0

    ; compute and possibly adjust delta X (DX)

    ldy     #SPRITE_PATH_X_LO                ; offset to X (LO) in path structure
    lda     (ZP_OLD_PATH_PTR),y              ; get target X (LO)
    sec
    sbc     (ZP_PROJECTILE_PTR),y            ; subtract projectile X (LO)
    sta     ZP_DELTA_X_LO                    ; save delta X (LO)
    iny                                      ; offset to X (HI) in path structure
    lda     (ZP_OLD_PATH_PTR),y              ; get target X (HI)
    sbc     (ZP_PROJECTILE_PTR),y            ; subtract projectile X (HI)
    sta     ZP_DELTA_X_HI                    ; save delta X (HI)

    lda     ZP_DELTA_X_LO                    ; load delta X (LO)
    clc
    adc     #16                              ; add 16 to delta X
    sta     ZP_DELTA_X_LO                    ; save delta X (LO)
    sta     ZP_TEMP_INDEX_LO                 ; save temporary index (LO)
    bcc     no_delta_wrap2                   ; go if no overflow
    inc     ZP_DELTA_X_HI                    ; save delta X (HI)
no_delta_wrap2:
    lda     ZP_DELTA_X_HI                    ; load delta X (HI)
    sta     ZP_TEMP_INDEX_HI                 ; save temporary index (HI)
    bne     keep_rolling                     ; go if DX >= 256, or < 0
    lda     ZP_DELTA_X_LO                    ; get delta X (LO)
    and     #$E0                             ; make sure it's below 32
    bne     keep_rolling                     ; go if DX >= 32, or < 0

    ; index the sprite_hit_decision table to see if the sprites collide
 
    ; the table is indexed as sprite_hit_decision[dy][dx], so we must mutliply
    ; DY by 32, and add DX, to get the proper table index.

    lda     ZP_TEMP_INDEX_LO                 ; get temporary index (LO)
    asl                                      ; multiply DY by 2
    rol     ZP_TEMP_INDEX_HI
    asl                                      ; multiply DY by 2
    rol     ZP_TEMP_INDEX_HI
    asl                                      ; multiply DY by 2
    rol     ZP_TEMP_INDEX_HI
    asl                                      ; multiply DY by 2
    rol     ZP_TEMP_INDEX_HI
    asl                                      ; multiply DY by 2
    rol     ZP_TEMP_INDEX_HI
    clc                                      ; add DX to DY*32
    adc     ZP_DELTA_X_LO
    bcc     no_table_wrap
    inc     ZP_TEMP_INDEX_HI
no_table_wrap:

    ; add the start of the table, to get the address of the array element
    clc
    adc     #<sprite_hit_decision            ; add table start to index (LO)
    sta     ZP_TEMP_INDEX_LO                 ; save array element address (LO)
    lda     ZP_TEMP_INDEX_HI                    ; get table index (HI)
    adc     #>sprite_hit_decision            ; add table start to index (HI)
    sta     ZP_TEMP_INDEX_HI                 ; save array element address (HI)

    ; get the collision indicator from the table
    lda     (ZP_TEMP_INDEX)
    bne     new_collision                    ; go if a collision at pixel level

    .endif

keep_rolling:
    ; no collision, or no need to destroy the sprite

    lda     VERA_data0          ; skip 1 byte (pixel address LO)
    lda     VERA_data0          ; skip 1 byte (pixel address HI)

    CopySpriteXYFields
 
    ; use mask for next path point
    iny
    lda     (ZP_PATH_PTR),y     ; get collision mask from path table
    sta     sprite_collision_mask,x ; set the collision mask
    sta     VERA_data0          ; set collision mask in VERA
    bra     have_mask 

new_collision:
some_collision:
    lda     #<SPRITE_HIT_BITMAP_ADDR_SHIFTED ; get pointer to pixel data (LO)
    ldy     #(>SPRITE_HIT_BITMAP_ADDR_SHIFTED)|SPRITE_MODE_8_BPP ; pixel data (HI)
    sta     VERA_data0                   ; set pointer to pixel data (LO)
    sty     VERA_data0                   ; set pointer to pixel data (HI)

    CopySpriteXYFields
 
    ; for any collision, change the target sprite so it cannot collide
    lda     #SPRITE_ZDEPTH_ABOVE_L1 ; includes empty collision mask
    sta     sprite_collision_mask,x ; set initial sprite state
    sta     VERA_data0              ; show sprite, but prevent collision
 
have_mask:
 
    lda     VERA_data0              ; skip 1 byte (sprite width, height, etc.)
 
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

    ; multiply the path index by 9 (size of one X,Y point with masks)
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
     MoveCoordinatesAndMask path,end

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
    lda     #27*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 27 path point data structures
    sta     ZP_NEXT_SPRITE_INC  ; save the increment amount

    lda     #<quadrant_projectiles      ; get start of projectile array (LO)
    sta     ZP_QUAD_PROJ_ARRAY_PTR_LO   ; set start of projectile array (LO)
    lda     #>quadrant_projectiles      ; get start of projectile array (HI)
    sta     ZP_QUAD_PROJ_ARRAY_PTR_HI   ; set start of projectile array (HI)
    stz     ZP_QUAD_PROJ_CNT            ; assume no quadrant projectiles discovered

    MoveRing sprite_path_0,end_sprite_path_0,SPRITE_END_RING_0,ZP_PATH_INDEX_0,RING_0_POINTS

    lda     #9*SPRITE_PATH_STRUCT_SIZE ; # of bytes between 9 path point data structures
    sta     ZP_NEXT_SPRITE_INC  ; save the increment amount
 
; for sprites on the "horizontal" rings
TARGET = 1

    MoveRing sprite_path_1,end_sprite_path_1,SPRITE_END_RING_1,ZP_PATH_INDEX_1,RING_1_POINTS
    MoveRing sprite_path_2,end_sprite_path_2,SPRITE_END_RING_2,ZP_PATH_INDEX_2,RING_2_POINTS
    MoveRing sprite_path_3,end_sprite_path_3,SPRITE_END_RING_3,ZP_PATH_INDEX_3,RING_3_POINTS
    MoveRing sprite_path_4,end_sprite_path_4,SPRITE_END_RING_4,ZP_PATH_INDEX_4,RING_4_POINTS
    MoveRing sprite_path_5,end_sprite_path_5,SPRITE_END_RING_5,ZP_PATH_INDEX_5,RING_5_POINTS
    MoveRing sprite_path_6,end_sprite_path_6,SPRITE_END_RING_6,ZP_PATH_INDEX_6,RING_6_POINTS
    MoveRing sprite_path_7,end_sprite_path_7,SPRITE_END_RING_7,ZP_PATH_INDEX_7,RING_7_POINTS
    rts

break_here:
    rts
