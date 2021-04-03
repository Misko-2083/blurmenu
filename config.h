#ifndef __CONFIG_H
#define __CONFIG_H

struct config {
	int item_padding;
	char font[];
};

static struct config config = {
	.item_padding = 7,
	.font = "Sans 16",
};

#endif /* __CONFIG_H */
