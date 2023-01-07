/*
 * Copyright 2022 wtcat
 */
#ifndef UI_RC_ACTIONS_IMPL_H_
#define UI_RC_ACTIONS_IMPL_H_

#include <lvgl/lvgl_res_loader.h>
#include <lvgl/lvgl_bitmap_font.h>

#include "rc_access.h"

#ifdef __cplusplus
extern "C"{
#endif

struct subrc_load_ops;

struct rc_context {
	lvgl_res_scene_t scene;
	struct list_head grp_list; /* group list */
	struct list_head prg_list; /* picregion list */
	struct list_head res_list; /* resource list */
	const struct subrc_load_ops *ld_ops; 
};

struct subrc_load_ops {
	void *(*load_group_from_scene)(uint32_t id, struct rc_context *rc);
	void *(*load_group_from_group)(void *h, uint32_t id, struct rc_context *rc);
	void *(*load_picreg_from_scene)(uint32_t id, struct rc_context *rc);
	void *(*load_picreg_from_group)(void *h, uint32_t id, struct rc_context *rc);
};

struct rm_actions_class {
    struct rm_class base;
    const char *sty_file;
    const char *pic_file;
    const char *str_file;
};

struct subrc_node {
	struct subrc_node *next;
	uint32_t scene;
	bool (*subrc_load)(uint32_t scene, struct rc_context *rc);
};

int rc_actions_init(const char *pic_file, const char *str_file, 
    const char *sty_file);
int rc_actions_subrc_register(struct subrc_node *subrc_nd);

#ifdef __cplusplus
}
#endif
#endif /* UI_RC_ACTIONS_IMPL_H_ */
