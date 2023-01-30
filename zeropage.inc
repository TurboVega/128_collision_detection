; Zero Page custom variables

ZP_QUAD_PROJ_ARRAY_PTR = $30   ; used to loop through projectile sprites
ZP_QUAD_PROJ_ARRAY_PTR_LO = $30
ZP_QUAD_PROJ_ARRAY_PTR_HI = $31

ZP_PROJECTILE_PTR    = $32   ; used to loop through projectile sprites
ZP_PROJECTILE_PTR_LO = $32
ZP_PROJECTILE_PTR_HI = $33

ZP_TEMP_INDEX     = $3A      ; temporary index of coordinates on sprite path
ZP_TEMP_INDEX_LO  = $3A
ZP_TEMP_INDEX_HI  = $3B

ZP_PATH_INDEX_0     = $40    ; index of coordinates on sprite path #0
ZP_PATH_INDEX_0_LO  = $40
ZP_PATH_INDEX_0_HI  = $41

ZP_PATH_INDEX_1     = $42    ; index of coordinates on sprite path #1
ZP_PATH_INDEX_1_LO  = $42
ZP_PATH_INDEX_1_HI  = $43

ZP_PATH_INDEX_2     = $44    ; index of coordinates on sprite path #2
ZP_PATH_INDEX_2_LO  = $44
ZP_PATH_INDEX_2_HI  = $45

ZP_PATH_INDEX_3     = $46    ; index of coordinates on sprite path #3
ZP_PATH_INDEX_3_LO  = $46
ZP_PATH_INDEX_3_HI  = $47

ZP_PATH_INDEX_4     = $48    ; index of coordinates on sprite path #4
ZP_PATH_INDEX_4_LO  = $48
ZP_PATH_INDEX_4_HI  = $49

ZP_PATH_INDEX_5     = $4A    ; index of coordinates on sprite path #5
ZP_PATH_INDEX_5_LO  = $4A
ZP_PATH_INDEX_5_HI  = $4B

ZP_PATH_INDEX_6     = $4C    ; index of coordinates on sprite path #6
ZP_PATH_INDEX_6_LO  = $4C
ZP_PATH_INDEX_6_HI  = $4D

ZP_PATH_INDEX_7     = $4E    ; index of coordinates on sprite path #7
ZP_PATH_INDEX_7_LO  = $4E
ZP_PATH_INDEX_7_HI  = $4F

ZP_SPRITE_PTR     = $50    ; points to an 8-byte sprite attribute structure
ZP_SPRITE_PTR_LO  = $50
ZP_SPRITE_PTR_HI  = $51

ZP_PTR            = $52    ; temporary pointer used when copying RAM
ZP_PTR_LO         = $52
ZP_PTR_HI         = $53

ZP_PATH_PTR       = $54    ; used to point into any sprite path
ZP_PATH_PTR_LO    = $54
ZP_PATH_PTR_HI    = $55

;ZP_ISR_REG        = $56    ; holds a temporary copy of ISR register contents
;ZP_COLLISION_MASK = $57    ; used to compute or use a collision mask
;ZP_TEMP_VAR       = $58    ; holds a temporary value
ZP_NEXT_SPRITE_INC = $59   ; increment to reach next sprite data item for gapping
ZP_COLLISION_X    = $5A    ; copy of ISR register collision bits for X (left vs right)
ZP_COLLISION_Y    = $5B    ; copy of ISR register collision bits for Y (top vs bottom)
ZP_QUAD_PROJ_CNT  = $5C    ; quadrant projectile count
ZP_QUAD_PROJ_INDEX = $5D   ; index for looping through quadrant projectiles
