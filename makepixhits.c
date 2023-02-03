// File: makepixhits.c
// Purpose: Make table of hit decisions based on sprite coordinates
// Copyright (c) 2023 by Curtis Whitley

#include <stdio.h>

/*
    For sprites to collide, their bounding rectangles must overlap. Having a
    collision at the pixel level implies the overlap, but the opposite is not
    true; the overlap does not gurantee a pixel level collision.

    Sprites are 16x16 pixels, so the potential area within which they could overlap
    is nearly 32x32 pixels (31x31 to be precise), taking into account that the
    worst case is the trailing edge of sprite 1 overlapping the leading edge of
    sprite 2. If this happens in both X and Y, the sprites overlap in corners.
    The best case is that the 2 sprites overlap completely, meaning that their
    leading (and trailing) edges are aligned in both X and Y.

    If the upper-left corner of sprite 1 is at least 16 pixel columns to the left
    or right of the upper-left corner of sprite 2 then the sprites do not overlap,
    regardless of their pixel contents or Y coordinates.

    If the upper-left corner of sprite 1 is at least 16 pixel rows above or below
    the upper-left corner of sprite 2 then the sprites do not overlap, regardless
    of their pixel contents or Y coordinates.

    If the sprites do overlap in their bounding (16x16) rectangles, then their
    pixel contents must be compared, to see whether they truly collide at the
    individual pixel resolution. Looking at their coordinates as differences
    from each other (i.e., DX=X1-X2 and DY=Y1-Y2), the following must be true
    for any collision to occur (this checks bounding rectangles):

    -15 <= DX <= 15, or ABS(DX) <= 15

    -15 <= DY <= 16, or ABS(DY) <= 15

    Even if the above statements are true, it does not imply a collision at the
    pixel level, because that depends on the shape of each sprite.

    This program determines whether the sprites overlap for each combination of
    coordinates in X and Y. The output is a table of decision results (simple flags)
    that may be indexed using adjusted (relative, not absolute) X and Y coordinates.
    Specifically, the indexes into the table will be DX and DY, both adjusted to be
    non-negative by adding 16.

    As mentioned above, overlapping sprites must live within a 31x31 pixel area,
    relative to each other. If we assume a 32x32 pixel square, place sprite 1
    in the lower-right quadrant (at position (16,16))with, and virtually overlay sprite 2,
    we can determine whether sprite 2 collides  sprite 1, by looking for
    non-transparent pixels in both sprites, at the same pixel location, for
    every pixel location across sprite 1. This pixel test can be done for each
    possible position of sprite 2 (meaning for all applicable DX and DY values).
    Pixels of sprite 2 that lie outside of the 32x32 square are ignored, because
    they cannot possibly overlap sprite 1.

    For any given (DX,DY) pair, if the sprites overlap with those relative
    postions, then the output array will have a nonzero value in its [DY][DX]th bit
    position. If they do not overlap, then the [DY][DX]th bit position will contain zero.

    At runtime, the X16 only needs to index the output table, based on the coordinates
    of the two sprites, to know whether they collide at the pixel level. The
    X16 application does not need to test individual pixels, because that is
    done here in this program.

    The output table will only be used to test sprites at the pixel level that were
    already proven to overlap at both the quadrant level (with X and Y each
    divided into 2 parts, meaning 4 screen quadrants of 320x240 pixels per area),
    and at the spatial grid level (with X and Y each divided into 16 parts,
    meaning 256 screen cells of 40x30 pixels each).

    At present, the output table contains only 1 flag per byte. It could be optimized
    to have 8 flags per byte, reducing its memory requirement by a factor of 8;
    however, that would require slightly more runtime computation to check the
    proper bit within each byte.
*/

const unsigned char sprite_bitmap[16][16] =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x06,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x06,0x06,0x06,0x01,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0x06,0x06,0x06,0x06,0x06,0x06,0x01,0x00,0x00,0x00,0x00,

    0x00,0x00,0x00,0x00,0x01,0x06,0x06,0x06,0x06,0x06,0x06,0x01,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x06,0x06,0x06,0x01,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x06,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

void main() {
    printf("sprite_hit_decision:\n");
    for (int dy = -16; dy < 16; dy++) {
        for (int dx = -16; dx < 16; dx++) {
            unsigned char flag = 0;
            for (int y1 = 0; y1 < 16; y1++) {
                int y2 = y1 - dy;
                if (y2 >= 0 && y2 < 16) {
                    for (int x1 = 0; x1 < 16; x1++) {
                        int x2 = x1 - dx;
                        if (x2 >= 0 && x2 < 16) {
                            if (sprite_bitmap[y1][x1] && sprite_bitmap[y2][x2]) {
                                flag = 1;
                                break;
                            }
                        }
                    }
                }
                if (flag) {
                    break;
                }
            }
            printf("    .byte    %i    ; dx: %i, dy: %i\n", flag, dx, dy);
        }
        printf("\n");
    }
}
