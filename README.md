# 128_collision_detection
Demo of moving 128 sprites with collision detected, for Commander X16

This is the second part of a 2-part demo, where the code handles
collision detection with 128 moving sprites. This particular repository
has code that shows 128 sprites moving in concentric elliptical
paths (that I called rings). There is now collision detection
in this code.

```
Sprites by indexes:
  0..9   (10): ring 0
 10..25  (16): ring 1
 26..37  (12): ring 2
 38..51  (14): ring 3
 52..67  (16): ring 4
 68..85  (18): ring 5
 86..105 (20): ring 6
106..127 (22): ring 7

Approach on collision detection:

The idea is to combine several optimizations to yield reasonable speed
for detecting sprite collisions, when so many objects are on the screen.
This <i>could</i> involve one or more of several methods:

 1. Let the VERA <b>hardware</b> detect the collision <i>quadrants</i>, rather than
    detecting which <i>kinds</i> of sprites collided. This implies dividing
    the screen into 4 logical parts, since there are 4 collision bits available.

    a. First collision mask bit represents left half of screen.
       Sprite attribute offset 6: [---Czdvh]

    b. Second collision mask bit represents right half of screen.
       Sprite attribute offset 6: [--C-zdvh]

    c. Third collision mask bit represents top half of screen.
       Sprite attribute offset 6: [-C--zdvh]

    d. Fourth collision mask bit represents bottom half of screen.
       Sprite attribute offset 6: [C---zdvh]

    e. Sprite in top-left area of screen: [-C-Czdvh]

    f. Sprite in top-right area of screen: [-CC-zdvh]

    g. Sprite in bottom-left area of screen: [C--Czdvh]

    h. Sprite in bottom-right area of screen: [C-C-zdvh]

    i. It is possible that a sprite straddles center line between left & right,
       but not top & bottom: [-CCCzdvh] or [C-CCzdvh]

    j. It is possible that a sprite straddles center line between top & bottom,
       but not left & right: [CC-Czdvh] or [CCC-zdvh]

    k. It is possible that a sprite straddles the center point of the screen,
       meaning both left & right, and top & bottom: [CCCCzdvh]

    l. Upon detecting a collision, VERA will OR the collision bit settings of
       the sprites in question, placing the result into its ISR register,
       yielding a 4-bit indication of which sprites to check in more detail.
       When looping through sprites, for each one, this formula determines
       whether to check the sprite any further:

          (([sprite byte 6] AND [ISR value] AND [$C0]) != 0) AND
          (([sprite byte 6] AND [ISR value] AND [$30]) != 0)

       If the formula equals zero (or false), the sprite in question is not
       within any quadrant of the collision. Check it no further.

 2. Create lists of projectiles and targets within detected quadrants.
    For sprites passing the check in paragraph #1.l, above, keep 4 lists,
    or however many are appropriate for the game or demo program.

    a. player projectiles: things the player shoots/throws/etc.
    b. enemy targets: things the player destroys/acquires/etc.
    c. enemy projectiles: things the enemy shoots/throws/etc.
    d. player targets: usually the player ship/avatar/etc.

    For optimization, if there is only ever 1 item, such as for a player
    avatar target, no list is needed; target use can be hard-coded. If a target
    can be destroyed both by an enemy and by a player, that scenario must be
    handled. Essentially, we are trying to handle all cases where sprite A
    affects sprite B in some way, when A collides with B, or vice versa.
    We must always avoid comparing sprites that are never to considered
    as having a collision, since that would waste CPU cycles.

 3. For sprites making it into the lists in section 2, above, check their
    bounding boxes to see whether they could possibly collide. There are
    various ways to accomplish this, such as:

    a. Compute and compare the ranges of X and Y, based on sprite sizes,
       to see if there is any possible overlap in both directions. This
       is an easy check, but because X and Y are potentially larger than
       8 bits, it becomes computationally expensive to perform all of
       the required additions, subtractions, and comparisons.

    b. Keep spatial maps for sprites, as they move, then AND the map
       values to see whether they overlap. The final check may be easy,
       but maintaining the maps is expensive, particularly when sprites
       spend most of their time not colliding.

    c. Precompute the possible spatial maps, based on X or Y positions
       and sprite sizes, and use lookup tables to obtain the map bits
       for each sprite. Then, AND the values together to check for any
       possibility of collision. The precision of the check depends on
       the number of bits per spatial map (i.e., it determines the
       granularity of the horizontal or vertical grid cells). More bits
       is more accurate, but also takes up more memory.

    d. Precompute the detection results, and use lookup tables to determine
       whether sprites could collide. Tables can be used for either
       dimension (X or Y), but there must be a unique table for each
       combination of sprite sizes where a collision may occur (e.g.,
       a table for 8x8, for 8x16, for 16x16, etc). The amount of memory
       required depends on the precision of each index (i.e., the values
       by which X and/or Y are divided, in order to form table indexes).

    e. In this program, we use method 3.c, above, with the following
       related considerations:

       (1) Each spatial map is 16 bits long, so the screen is horizontally
           divided into 16 spatial grid columns, each 640/16=40 pixels wide.

       (2) Each spatial map is 16 bits long, so the screen is vertically
           divided into 16 spatial grid rows, each 480/16=30 pixels high.

       (3) An 8-, 16-, or 32-bit wide sprite may need 2 consecutive X map bits. 
       (4) A 64-bit wide sprite may need 3 consecutive X map bits.
       (5) An 8-, or 16-bit tall sprite may need 2 consecutive Y map bits. 
       (6) A 32-bit tall sprite may need 3 consecutive Y map bits. 
       (7) A 64-bit tall sprite may need 4 consecutive Y map bits.

       (8) Do a double loop comparing <i>player</i> projectiles to <i>enemy</i> targets.
           For each pair, AND their 16-bit spatial masks. If the result is zero,
           check that sprite pair no further. Otherwise, check in more detail
           (see paragraph 4, below).

       (9) Do a double loop comparing <i>enemy</i> projectiles to <i>player</i> targets.
           For each pair, AND their 16-bit masks. If the result is zero,
           check that pair no further. Otherwise, check in more detail
           (see paragraph 4, below).

 4. Perform detailed checks for a minimal number of sprites, meaning
    only those sprite pairs that passed the checks in paragraphs 3.e.(8)
    or 3.e.(9), above. Detailed checks may not be needed in a particular
    game, but if required, the code may need to go so far as to check the
    pixels of the projectile sprite against the pixels of the target sprite.
    If such checks are needed, then using monochrome maps in place of the
    colored sprite pixels makes testing easier and faster. 

    Note that if a collision is detected and handled, the state of
    one or both sprites may change, effectively removing it/them from
    being considered in subsequent loops.
```
