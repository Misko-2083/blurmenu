#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include "blurmenu.h"
#include "config.h"

static int item_width = 120;
static int item_height = 25;

static struct menuitem *add_item(struct menu *menu)
{
	struct menuitem *item;
	if (menu->nr_items == menu->alloc_items) {
		menu->alloc_items = (menu->alloc_items + 16) * 2;
		menu->items = realloc(menu->items,
			menu->alloc_items * sizeof(struct menuitem));
	}
	item = menu->items + menu->nr_items;
	memset(item, 0, sizeof(*item));
	menu->nr_items++;
	return item;
}

static struct menuitem *item_create(struct menu *menu, const char *name)
{
	static int x = 10;
	static int y = 10;

	struct menuitem *item = add_item(menu);
	item->name = strdup(name);
	item->box.x = x;
	item->box.y = y;
	item->box.w = item_width;
	item->box.h = item_height;
	y += item_height + 10;
	double blue[] = { 0.0, 0.0, 1.0, 1.0 };
	double white[] = { 1.0, 1.0, 1.0, 1.0 };
	item->texture = texture_create(&item->box, name, white, blue);
	return item;
}

void items_init(struct menu *menu)
{
	/*
	 * This will be the place to create menu items - through whatever
	 * route, for example .desktop files, predefined, config or from stdin
	 */
	item_create(menu, "aaa");
	item_create(menu, "bbb");
	item_create(menu, "ccc");
	
	/* select first item */
	menu->selected = menu->items;
}

void items_finish(struct menu *menu)
{
	struct menuitem *item;
	for (int i = 0; i < menu->nr_items; i++) {
		item = menu->items + i;
		if (item->name)
			free(item->name);
		if (item->texture)
			cairo_surface_destroy(item->texture);
	}
}

void items_handle_key(struct menu *menu, int step)
{
	int index = menu->selected - menu->items;
	if (step < 0 && index == 0)
		return;
	if (step > 0 && index == menu->nr_items - 1)
		return;
	menu->selected += step;
}
