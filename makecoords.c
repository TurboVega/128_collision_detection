// File: makecoords.c
// Purpose: Make table of X,Y coordinates and related sprite path data
// Copyright (c) 2023 by Curtis Whitley

#include <stdio.h>
#include <math.h>
#include <string.h>

#define SPRITE_COLL_MASK_00_NONE    ((unsigned char)0x00)
#define SPRITE_COLL_MASK_10_LEFT    ((unsigned char)0x10)
#define SPRITE_COLL_MASK_20_RIGHT   ((unsigned char)0x20)
#define SPRITE_COLL_MASK_40_TOP     ((unsigned char)0x40)
#define SPRITE_COLL_MASK_80_BOTTOM  ((unsigned char)0x80)

#define SPRITE_ZDEPTH_ABOVE_L1      ((unsigned char)0x0C)

#define SCREEN_WIDTH                640
#define SCREEN_HEIGHT               480
#define CENTER_X                    ((short)(SCREEN_WIDTH/2))
#define CENTER_Y                    ((short)(SCREEN_HEIGHT/2))

#define SPRITE_WIDTH                16
#define SPRITE_HEIGHT               16

typedef int bool;

/*
Dec  Hex 
400 $0190
208 $00D0
256 $0100
304 $0130
352 $0160
400 $0190
464 $01D0
512 $0200
*/

static const int ring_radius[8] = { 50, 70, 90, 110, 130, 150, 170, 190 };
static const int num_points[8] = { 400, 208, 256, 304, 352, 400, 464, 512 };
static unsigned short spatial_map_x[640];
static unsigned short spatial_map_y[480];

bool point_in_top_left(short x, short y) {
    return x >= 0 && x < CENTER_X && y >= 0 && y < CENTER_Y;
}

bool sprite_in_top_left(short x, short y) {
    short lastx = x + SPRITE_WIDTH - 1;
    short lasty = y + SPRITE_HEIGHT - 1;
    return point_in_top_left(x, y) ||
            point_in_top_left(x, lasty) ||
            point_in_top_left(lastx, y) ||
            point_in_top_left(lastx, lasty);
}

bool point_in_top_right(short x, short y) {
    return x >= CENTER_X && x < SCREEN_WIDTH && y >= 0 && y < CENTER_Y;
}

bool sprite_in_top_right(short x, short y) {
    short lastx = x + SPRITE_WIDTH - 1;
    short lasty = y + SPRITE_HEIGHT - 1;
    return point_in_top_right(x, y) ||
            point_in_top_right(x, lasty) ||
            point_in_top_right(lastx, y) ||
            point_in_top_right(lastx, lasty);
}

bool point_in_bottom_left(short x, short y) {
    return x >= 0 && x < CENTER_X && y >= CENTER_Y && y < SCREEN_HEIGHT;
}

bool sprite_in_bottom_left(short x, short y) {
    short lastx = x + SPRITE_WIDTH - 1;
    short lasty = y + SPRITE_HEIGHT - 1;
    return point_in_bottom_left(x, y) ||
            point_in_bottom_left(x, lasty) ||
            point_in_bottom_left(lastx, y) ||
            point_in_bottom_left(lastx, lasty);
}

bool point_in_bottom_right(short x, short y) {
    return x >= CENTER_X && x < SCREEN_WIDTH && y >= CENTER_Y && y < SCREEN_HEIGHT;
}

bool sprite_in_bottom_right(short x, short y) {
    short lastx = x + SPRITE_WIDTH - 1;
    short lasty = y + SPRITE_HEIGHT - 1;
    return point_in_bottom_right(x, y) ||
            point_in_bottom_right(x, lasty) ||
            point_in_bottom_right(lastx, y) ||
            point_in_bottom_right(lastx, lasty);
}

void create_spatial_map(unsigned short* spatial_map, int sprite_size,
                        int screen_size, int cell_size) {
    unsigned char bits[640+64];
    for (int pos = 0; pos < screen_size; pos++) {
        memset(bits, 0, sizeof(bits));
        for (int b=0; b<sprite_size; b++) {
            bits[pos+b] = 1;
        }
        unsigned short map = 0;
        int pos2 = 0;
        for (int cell = 0; cell < screen_size/cell_size; cell++) {
            unsigned char bit = 0;
            for (int b=0; b<cell_size; b++) {
                bit |= bits[pos2++];
            }
            spatial_map[pos] = (spatial_map[pos] << 1) | (unsigned short)bit;
        }
    }
}

int main()
{
    create_spatial_map(spatial_map_x, 16, 640, 40);
    create_spatial_map(spatial_map_y, 16, 480, 30);

    for (int n = 0; n < 8; n++) {
        int hradius, vradius;
        if (n == 0) {
            // swap x and y (turn the ring 90 degrees)
            hradius = ring_radius[n];
            vradius = (hradius*12)/3;
        } else {
            vradius = ring_radius[n];
            hradius = (vradius*4)/3;
        }
        printf("sprite_path_%i: ; %ix%i, %i points\n", n, hradius, vradius, num_points[n]);
        float inc = 360.0/num_points[n];
        int pt = 0;
        for (float angle = 0.0; angle < 360.0; angle+=inc) {
            float rads = 2.0f*3.1415926f*(float)angle/360.0f;

            short x=(short)(int)(cos(rads)*(float)hradius+320.0)-8;
            short y=(short)(int)(sin(rads)*(float)vradius+240.0)-8;

            unsigned char collision_mask = SPRITE_ZDEPTH_ABOVE_L1;

            if (sprite_in_top_left(x, y)) {
                collision_mask |= (SPRITE_COLL_MASK_40_TOP|SPRITE_COLL_MASK_10_LEFT);
            }

            if (sprite_in_top_right(x, y)) {
                collision_mask |= (SPRITE_COLL_MASK_40_TOP|SPRITE_COLL_MASK_20_RIGHT);
            }

            if (sprite_in_bottom_left(x, y)) {
                collision_mask |= (SPRITE_COLL_MASK_80_BOTTOM|SPRITE_COLL_MASK_10_LEFT);
            }

            if (sprite_in_bottom_right(x, y)) {
                collision_mask |= (SPRITE_COLL_MASK_80_BOTTOM|SPRITE_COLL_MASK_20_RIGHT);
            }

            printf("    .word    %i,%i         ;   %i, angle: %f, rads: %f\n", x, y, pt++, angle, rads);
            printf("    .byte    $%02hX             ;   quadrant mask: %c%c%c%c\n", collision_mask,
                    (collision_mask & SPRITE_COLL_MASK_80_BOTTOM) ? 'B' : '-',
                    (collision_mask & SPRITE_COLL_MASK_40_TOP) ? 'T' : '-',
                    (collision_mask & SPRITE_COLL_MASK_20_RIGHT) ? 'R' : '-',
                    (collision_mask & SPRITE_COLL_MASK_10_LEFT) ? 'L' : '-');
            printf("    .word    $%04hX,$%04hX     ;   grid masks X & Y\n",
                    spatial_map_x[x], spatial_map_y[y]);

            if (pt >= num_points[n]) {
                break;
            }
        }
        printf("end_sprite_path_%i: ; %ix%i\n\n", n, hradius, vradius);
    }

    return 0;
}
