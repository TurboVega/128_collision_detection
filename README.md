# 128_collision_detection
<b>Demo of moving 128 sprites with collision detection, for Commander X16</b>

This is the second part of a 2-part demo, where the code handles
collision detection with 128 moving sprites. This particular repository
has code that shows 128 sprites moving in concentric elliptical
paths (that I called rings). There is now collision detection
in this code.

To make the movement of so many objects as smooth as possible, I have attempted
to reduce the computations required during each screen frame (i.e., during the
blanking time indicated by the VERA VSYNC interrupt). The method for reducing
the computations at runtime is simple: perform most of the computations ahead
of time, and keep the results in tables. This includes:

* The X and Y coordinates of every sprite position on every ring (elliptical path).
Keeping this data in a table eliminates the sin() and cos() computations at runtime.
* The decisions of whether a projectile collides with a target based on their
relative distance from each other. Keeping this data in a table eliminates the
pixel-level comparison operations at runtime.

<b>Please be aware that I have traded memory for CPU cycles, meaning that the
pre-computed table data requires a lot of memory!</b>

Here are some more details about the chosen algorithms in this program.

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
This involves a series (combination) of several methods:

 1. Let the VERA hardware detect the collision quadrants, rather than
    detecting which kinds of sprites collided. This implies dividing
    the screen into 4 logical parts, since there are 4 collision bits
    available in VERA.

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
       
       Bear in mind that multiple quadrants may be indicated simulatneously.

 2. Create a list of projectiles within detected quadrants (meaning
    quadrants where collisions are detected). Projectiles not found
    in detected quadrants may be ignored when evaluating targets.

 3. Loop through all target sprites in order to update their movement
    and appearances. In many applications, targets might be destroyed and
    removed from the screen. In this application, they continue to move,
    but with a color indicating their having been hit by projectiles.
    
    a. If any targets have already had collisions, do not make any
    further collision detection testing for them; just keep moving
    them (in this particular program, anyway).

    b. For targets not colliding with projectiles earlier, loop through
       the list of detected projectiles, and compare spatial maps, by
       AND-ing the map values. If the result is zero, there is no
       collision for that particular projectile and target.
       If the result of any AND operation is nonzero, then there is a
       spatial collision. 

       (1) Each X spatial map is 16 bits long, so the screen is horizontally
           divided into 16 spatial grid columns, each 640/16=40 pixels wide.

       (2) Each Y spatial map is 16 bits long, so the screen is vertically
           divided into 16 spatial grid rows, each 480/16=30 pixels high.

       (3) An 8-, 16-, or 32-bit wide sprite may need 2 consecutive X map bits. 
       (4) A 64-bit wide sprite may need 3 consecutive X map bits.
       (5) An 8-, or 16-bit tall sprite may need 2 consecutive Y map bits. 
       (6) A 32-bit tall sprite may need 3 consecutive Y map bits. 
       (7) A 64-bit tall sprite may need 4 consecutive Y map bits.

       Note: This program only uses 16x16 sprites.

    c. For targets involved in spatial collisions, lookup the proper item
       in the pixel-level decision table, based on the X and Y coordinates
       of the target and the projectile. If the table item value is
       nonzero, then there is a pixel-level collision. The lookup indexes
       are relative, not absolute, so DX and DY are used, not the actual
       X and Y coordinates. This is necessary to keep the size of the
       decision table within reasonable memory limits.

```
