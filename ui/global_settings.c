/*
 * File: global_settings.c
 * Project: ui
 * File Created: Monday, 12th July 2021 10:33:21 am
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "global_settings.h"
#include <dc/vmu_pkg.h>
#include <dc/vmufs.h>
#include <dc/fs_vmu.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <string.h>
#include <stdlib.h>
#include <kos/thread.h>

/* Images and such */
#if __has_include("openmenu_lcd.h") && __has_include("openmenu_pal.h") && __has_include("openmenu_vmu.h")
#include "openmenu_lcd.h"
#include "openmenu_pal.h"
#include "openmenu_vmu.h"

#define OPENMENU_ICON (openmenu_icon)
#define OPENMENU_LCD (openmenu_lcd)
#define OPENMENU_PAL (openmenu_pal)
#define OPENMENU_ICONS (1)
#else
#define OPENMENU_ICON (NULL)
#define OPENMENU_LCD (NULL)
#define OPENMENU_PAL (NULL)
#define OPENMENU_ICONS (0)
#endif

static openmenu_settings savedata;

static void settings_defaults(void) 
{
	savedata.identifier[0] = 'O';
	savedata.identifier[1] = 'M';
	savedata.version = SETTINGS_VERSION;
	savedata.ui = UI_LINE_DESC;
	savedata.region = REGION_NTSC_U;
	savedata.aspect = ASPECT_NORMAL;
	savedata.sort = SORT_DEFAULT;
	savedata.filter = FILTER_ALL;
	savedata.beep = BEEP_ON;
	savedata.multidisc = MULTIDISC_SHOW;
	savedata.custom_theme = THEME_OFF;
	savedata.custom_theme_num = THEME_0;
}

static void vmu_alldisplay_icon(uint8_t *bitmap)
{
    int            i = 0;
    maple_device_t *dev;

    while((dev = maple_enum_type(i++, MAPLE_FUNC_LCD))) 
    {
        vmu_draw_lcd(dev, bitmap);
    }
}

static int settings_validate(openmenu_settings *s)
{
	if (s->version != SETTINGS_VERSION) 
	{
		return -1;
	}
	
	if (s->identifier[0] != 'O' || s->identifier[1] != 'M')
	{
		return -2;
	}
	
	if ((s->ui < UI_START) || (s->ui > UI_END)) 
	{
		savedata.ui = UI_LINE_DESC;
	}
	
	if ((s->region < REGION_START) || (s->region > REGION_END)) 
	{
		savedata.region = REGION_NTSC_U;
	}
	
	if ((s->aspect < ASPECT_START) || (s->aspect > ASPECT_END)) 
	{
		savedata.aspect = ASPECT_NORMAL;
	}
	
	if ((s->sort < SORT_START) || (s->sort > SORT_END)) 
	{
		savedata.sort = SORT_DEFAULT;
	}
	
	if ((s->filter < FILTER_START) || (s->filter > FILTER_END)) 
	{
		savedata.filter = FILTER_ALL;
	}
	
	if ((s->beep < BEEP_START) || (s->beep > BEEP_END)) 
	{
		savedata.beep = BEEP_ON;
	}
	
	if ((s->multidisc < MULTIDISC_START) || (s->multidisc > MULTIDISC_END)) 
	{
		savedata.multidisc = MULTIDISC_SHOW;
	}
	
	if ((s->custom_theme < THEME_START) || (s->custom_theme > THEME_END)) 
	{
		savedata.custom_theme_num = (CFG_CUSTOM_THEME_NUM) THEME_OFF;
	}
	
	if (s->custom_theme_num > THEME_NUM_END) 
	{
		savedata.custom_theme_num = THEME_NUM_START;
	}
	
	if (s->custom_theme) 
	{
		savedata.region = REGION_END + 1 + s->custom_theme_num;
	}
	
	return 0;
}

void settings_init(void)
{
	settings_defaults();
	
#if OPENMENU_ICONS
	vmu_alldisplay_icon(OPENMENU_LCD);
#endif
	thd_sleep(200);
	
	settings_load();
}

void settings_load(void) 
{
	char save_name[24];
	maple_device_t *vmu;
	file_t f;
	openmenu_settings tmp;
	
	for (int i = 0; i < 8; i++)
	{
		if (!(vmu = maple_enum_type(i, MAPLE_FUNC_MEMCARD)))
		{
			// no more connected VMU's
			//printf("vmu %d not connected\n", i);
			return;
		}
		
		// Try and load savefile
		
		sprintf(save_name, "/vmu/%c%c/OPENMENU.CFG", vmu->port+'A', vmu->unit+'0');
		
		if ((f = fs_open(save_name, O_RDONLY)) == FILEHND_INVALID)
		{
			//printf("save %s not found\n", save_name);
			continue;
		}
		
		fs_read(f, &tmp, sizeof(openmenu_settings));
		
		if (settings_validate(&tmp) < 0)
		{
			//printf("settings not valid\n");
			fs_close(f);
			fs_unlink(save_name);
			continue;
		}
		
		memcpy(&savedata, &tmp, sizeof(openmenu_settings));
		
		fs_close(f);
		
		if (savedata.beep == BEEP_ON)
		{
			vmu_beep_raw(vmu, 0x000065f0); // Turn on Beep
			thd_sleep(500);
			vmu_beep_raw(vmu, 0x00000000); // Turn off Beep
		}
		
		return;
	}
}

/* Beeps while saving if enabled */
void settings_save(void) 
{
	char save_name[24];
	maple_device_t *vmu;
	file_t f;
	int pkg_size;
	vmu_pkg_t pkg;
	uint8_t *pkg_out;
	
	strcpy(pkg.desc_short, "openMenu Config");
    strcpy(pkg.desc_long, "openMenu Preferences");
    strcpy(pkg.app_id, "openMenuPref");
    
    pkg.icon_cnt = OPENMENU_ICONS;
    pkg.icon_data = OPENMENU_ICON;
    pkg.icon_anim_speed = 0;
    pkg.eyecatch_type = VMUPKG_EC_NONE;
	memcpy(pkg.icon_pal, OPENMENU_PAL, 32);
	pkg.data = (uint8_t *) &savedata;
	pkg.data_len = sizeof(openmenu_settings);
	
	if(vmu_pkg_build(&pkg, &pkg_out, &pkg_size) < 0)
	{
		//printf("ERROR can't build vmu_pkg\n");
		return;
	}
	
	for (int i = 0; i < 8; i++)
	{
		if (!(vmu = maple_enum_type(i, MAPLE_FUNC_MEMCARD)))
		{
			// no more connected VMU's
			break;
		}
		
		sprintf(save_name, "/vmu/%c%c/OPENMENU.CFG", vmu->port+'A', vmu->unit+'0');
		
		if ((f = fs_open(save_name, O_RDONLY)) != FILEHND_INVALID)
		{
			// save file found
			fs_close(f);
			fs_unlink(save_name);
		}
		
		if((vmufs_free_blocks(vmu)*512) < pkg_size)
		{
			// no free memory, try next vmu
			//printf("ERROR vmu don't have free memory\n");
			continue;
		}
		
		if ((f = fs_open(save_name, O_WRONLY | O_META)) == FILEHND_INVALID)
		{
			// can't create save file, try next vmu
			//printf("ERROR cant create %s\n", save_name);
			fs_close(f);
			fs_unlink(save_name);
		}
		
		fs_write(f, pkg_out, pkg_size);
		fs_close(f);
		
		if (savedata.beep == BEEP_ON)
		{
			vmu_beep_raw(vmu, 0x000065f0); // Turn on Beep
			thd_sleep(500);
			vmu_beep_raw(vmu, 0x00000000); // Turn off Beep
		}
		
		break;
	}
	
	free(pkg_out);
}

openmenu_settings *settings_get(void) 
{
	return &savedata;
}

