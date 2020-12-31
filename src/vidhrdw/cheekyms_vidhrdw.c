/*************************************************************************
 Universal Cheeky Mouse Driver
 (c)Lee Taylor May 1998, All rights reserved.

 For use only in offical Mame releases.
 Not to be distrabuted as part of any commerical work.
***************************************************************************
Functions to emulate the video hardware of the machine.
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int redraw_man = 0;
static int man_scroll = -1;
static data8_t sprites[0x20];
static int char_palette = 0;


PALETTE_INIT( cheekyms )
{
	int i, j, bit, r, g, b;

	for (i = 0; i < 6; i++)
	{
		for (j = 0; j < 0x20; j++)
		{
			/* red component */
			bit = (color_prom[0x20 * (i / 2) + j] >> ((4 * (i & 1)) + 0)) & 0x01;
			r = 0xff * bit;
			/* green component */
			bit = (color_prom[0x20 * (i / 2) + j] >> ((4 * (i & 1)) + 1)) & 0x01;
			g = 0xff * bit;
			/* blue component */
			bit = (color_prom[0x20 * (i / 2) + j] >> ((4 * (i & 1)) + 2)) & 0x01;
			b = 0xff * bit;

			palette_set_color((i * 0x20) + j, r,g,b);
		}
	}
}


WRITE_HANDLER( cheekyms_sprite_w )
{
	sprites[offset] = data;
}


WRITE_HANDLER( cheekyms_port_40_w )
{
	/* The lower bits probably trigger sound samples */
	DAC_data_w(0, data ? 0x80 : 0);
}


WRITE_HANDLER( cheekyms_port_80_w )
{
	int new_man_scroll;

	/* Bits 0-1 Sound enables, not sure which bit is which */

	/* Bit 2 is interrupt enable */
	interrupt_enable_w(offset, data & 0x04);

	/* Bit 3-5 Man scroll amount */
    new_man_scroll = (data >> 3) & 0x07;
	if (man_scroll != new_man_scroll)
	{
		man_scroll = new_man_scroll;
		redraw_man = 1;
	}

	/* Bit 6 is palette select (Selects either 0 = PROM M8, 1 = PROM M9) */
	set_vh_global_attribute(&char_palette, (data >> 2) & 0x10);

	/* Bit 7 is screen flip */
	flip_screen_set(data & 0x80);
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( cheekyms )
{
	int offs;


	if (get_vh_global_attribute_changed())
	{
		memset(dirtybuffer, 1, videoram_size);
	}


	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	/* Draw the sprites first, because they're supposed to appear below
	   the characters */
	for (offs = 0; offs < 0x20; offs += 4)
	{
		int v1, sx, sy, col, code;

		if ((sprites[offs + 3] & 0x08) == 0x00) continue;

		v1  = sprites[offs + 0];
		sy  = sprites[offs + 1];
		sx  = 256 - sprites[offs + 2];
		code =  (~sprites[offs + 0] & 0x0f) << 1;
		col = (~sprites[offs + 3] & 0x07);

		if (v1 & 0x80)
		{
			if (!flip_screen)
			{
				code++;
			}

			drawgfx(bitmap,Machine->gfx[1],
					code,col,
					0,0,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
		else
		{
			if (v1 & 0x02)
			{
				drawgfx(bitmap,Machine->gfx[1],
					code + 0x20,
					col,
					0,0,
					sx, sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

				drawgfx(bitmap,Machine->gfx[1],
					code + 0x21,
					col,
					0,0,
					sx + 0x10, sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
			else
			{
				drawgfx(bitmap,Machine->gfx[1],
					code + 0x20,
					col,
					0,0,
					sx, sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

				drawgfx(bitmap,Machine->gfx[1],
					code + 0x21,
					col,
					0,0,
					sx, sy + 0x10,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy,man_area,color;

		sx = offs % 32;
		sy = offs / 32;


		man_area = ((sy >=  6) && (sy <= 26) && (sx >=  8) && (sx <= 12));

		if (sx >= 30)
		{
			if (sy < 12)
				color = 0x15;
			else if (sy < 20)
				color = 0x16;
			else
				color = 0x14;
		}
		else
		{
			color = ((sx >> 1) & 0x0f) + char_palette;
			if (sy == 4 || sy == 27)
				color = 0xc + char_palette;
		}

		if (flip_screen)
		{
			sx = 31 - sx;
			sy = 31 - sy;
		}

		drawgfx(tmpbitmap,machine->gfx[0],
				videoram[offs],
				color,
				flip_screen,flip_screen,
				8*sx, 8*sy - (man_area ? man_scroll : 0),
				cliprect,TRANSPARENCY_PEN,0);
	}

	redraw_man = 0;

	/* copy the temporary bitmap to the screen over the sprites */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN,Machine->pens[4*char_palette]);
}
