/*
 * Copyright 2022 wtcat
 *
 * The Resource API convert layer for the SDK-ZS308 of actions company
 */
#include <errno.h>

#include "basework/ui/resource/rc_actions_impl.h"

#define SCENE_MAX  10
#define GROUP_MAX  (SCENE_MAX + 50)
#define PICREG_MAX 5

/* Resource type */
enum rc_type {
	RC_SCENE,
	RC_GROUP,
	RC_PICREG,
	RC_FONT,
	RC_MAX
};

struct group_node {
	struct list_head node;
	lvgl_res_group_t group;
};

struct picreg_node {
	struct list_head node;
	lvgl_res_picregion_t picreg;
};

struct font_node {
	struct list_head node;
	lv_font_t font;
};

struct rc_pool {
	struct rc_context context[SCENE_MAX];
	struct group_node groups[GROUP_MAX];
	struct picreg_node picreg[PICREG_MAX];
	char *freelst[RC_MAX];
};

struct rc_img_obj {
	struct image_obj *obj;
	lv_img_dsc_t imgs[];
};

struct rc_str_obj {
	struct string_obj *obj;
	lvgl_res_string_t *rstr;
	struct c_string cstr[];
};

struct rc_font_obj {
	struct font_obj *obj;
	lv_font_t font;
};

static struct rc_pool rc_pool;
static struct subrc_node *subrc_root;

static void rc_pool_init(void) {
	char *p;
	int i;

	for (i = 0, p = (char *)&rc_pool.context[0]; i < SCENE_MAX; i++) {
		*(char **)p = rc_pool.freelst[RC_SCENE];
		rc_pool.freelst[RC_SCENE] = p;
		p += sizeof(rc_pool.context[0]);
	}
	
	for (i = 0, p = (char *)&rc_pool.groups[0]; i < GROUP_MAX; i++) {
		*(char **)p = rc_pool.freelst[RC_GROUP];
		rc_pool.freelst[RC_GROUP] = p;
		p += sizeof(rc_pool.groups[0]);
	}
	
	for (i = 0, p = (char *)&rc_pool.picreg[0]; i < PICREG_MAX; i++) {
		*(char **)p = rc_pool.freelst[RC_PICREG];
		rc_pool.freelst[RC_PICREG] = p;
		p += sizeof(rc_pool.picreg[0]);
	}
}

static void *rc_alloc(enum rc_type type) {
	char *p = rc_pool.freelst[type];
	if (p) 
		rc_pool.freelst[type] = *(char **)p;
    RC_ASSERT(p != NULL);
	return p;
}

static void rc_free(void *ptr, enum rc_type type) {
	RC_ASSERT(type < RC_MAX);
	if (ptr) {
		*(char **)ptr = rc_pool.freelst[type];
		rc_pool.freelst[type] = (char *)ptr;
	}
}

static void *rc_load_group_from_scene(uint32_t id, struct rc_context *rc) {
	struct group_node *grp;
	int err;

	if (rc == NULL)
		return NULL;

	grp = rc_alloc(RC_GROUP);
	err = lvgl_res_load_group_from_scene(&rc->scene, id, &grp->group);
	if (err) {
		rc_free(grp, RC_GROUP);
		return NULL;
	}

	list_add_tail(&grp->node, &rc->grp_list);
	return &grp->group;
}

static void *rc_load_group_from_group(void *h, uint32_t id, struct rc_context *rc) {
	struct group_node *grp;
	int err;

	if (h == NULL || rc == NULL)
		return NULL;

	grp = rc_alloc(RC_GROUP);
	err = lvgl_res_load_group_from_group(h, id, &grp->group);
	if (err) {
		rc_free(grp, RC_GROUP);
		return NULL;
	}

	list_add_tail(&grp->node, &rc->grp_list);
	return &grp->group;
}

static void *rc_load_picreg_from_scene(uint32_t id, struct rc_context *rc) {
	struct picreg_node *prg;
	int err;

	if (rc == NULL)
		return NULL;

	prg = rc_alloc(RC_PICREG);
	err = lvgl_res_load_picregion_from_scene(&rc->scene, id, &prg->picreg);
	if (err) {
		rc_free(prg, RC_PICREG);
		return NULL;
	}

	list_add_tail(&prg->node, &rc->prg_list);
	return &prg->picreg;
}

static void *rc_load_picreg_from_group(void *h, uint32_t id, struct rc_context *rc) {
	struct picreg_node *prg;
	int err;

	if (h == NULL || rc == NULL)
		return NULL;

	prg = rc_alloc(RC_PICREG);
	err = lvgl_res_load_picregion_from_group(h, id, &prg->picreg);
	if (err) {
		rc_free(prg, RC_PICREG);
		return NULL;
	}

	list_add_tail(&prg->node, &rc->prg_list);
	return &prg->picreg;
}

static int rc_load_subrc(uint32_t scene, struct rc_context *rc) {
	struct subrc_node *subrc = subrc_root;
	
	while (subrc) {
		if (subrc->subrc_load) {
			if (subrc->subrc_load(scene, rc))
				return 0;
		}
		subrc = subrc->next;
	}
	
	return 0;
}

static int rc_open(struct rm_class *cls, const void *oid, void **handle) {
	static const struct subrc_load_ops ops = {
		.load_group_from_scene = rc_load_group_from_scene,
		.load_group_from_group = rc_load_group_from_group,
		.load_picreg_from_scene = rc_load_picreg_from_scene,
		.load_picreg_from_group = rc_load_picreg_from_group
	};
    struct rm_actions_class *rac = (struct rm_actions_class *)cls;
	uint32_t scene = (uint32_t)oid;
	struct rc_context *rc;
	int err;

	if (handle == NULL)
		return -EINVAL;

	rc = rc_alloc(RC_SCENE);
	RC_ASSERT(rc != NULL);
	INIT_LIST_HEAD(&rc->grp_list);
	INIT_LIST_HEAD(&rc->prg_list);
	INIT_LIST_HEAD(&rc->res_list);
	err = lvgl_res_load_scene(scene, &rc->scene, 
        rac->str_file, rac->pic_file, rac->str_file);
	if (err)
		return err;

	rc->ld_ops = &ops;
	err = rc_load_subrc(scene, rc);
	if (err) 
		lvgl_res_unload_scene(&rc->scene);
	
	*handle = rc;
	
	return err;
}

static void rc_close(struct rm_class *cls, void *handle) {
	struct rc_context *rc = handle;
	struct group_node *grp, *next_grp;
	struct picreg_node *prg, *next_prg;

	if (rc == NULL)
		return;
	_resources_release(&rc->res_list);

	list_for_each_entry_safe_reverse(prg, next_prg, &rc->prg_list, node) {
		list_del(&prg->node);
		lvgl_res_unload_picregion(&prg->picreg);
		rc_free(prg, RC_PICREG);
	}
	
	list_for_each_entry_safe_reverse(grp, next_grp, &rc->grp_list, node) {
		list_del(&grp->node);
		lvgl_res_unload_group(&grp->group);
		rc_free(grp, RC_GROUP);
	}

	lvgl_res_unload_scene(&rc->scene);
}

static void rc_pictures_release(void *data) {
	struct rc_img_obj *robj = data;
	lvgl_res_unload_pictures(robj->obj->imgs, robj->obj->num);
	_resource_node_free(robj);
}

static int rc_load_pictures(struct rc_context *rc, struct resource *r) {
	struct image_obj *img = (struct image_obj *)r->pres;
	struct rc_img_obj *robj;
	struct group_node *grp;
	struct picreg_node *prg;
	int err;

	robj = _resource_node_alloc(sizeof(*robj) + sizeof(lv_img_dsc_t) * r->num, 
		rc_pictures_release, &rc->res_list);
	if (!robj)
		return -ENOMEM;
	
	err = lvgl_res_load_pictures_from_scene(&rc->scene, r->oid_ptr, robj->imgs, 
        r->pext, r->num);
	if (!err)
		goto _success;
	
	list_for_each_entry(grp, &rc->grp_list, node) {
		err = lvgl_res_load_pictures_from_group(&grp->group, r->oid_ptr, 
            robj->imgs, r->pext, r->num);
		if (!err)
			goto _success;
	}

	list_for_each_entry(prg, &rc->prg_list, node) {
		err = lvgl_res_load_pictures_from_picregion(&prg->picreg,
            r->oid_u16[0], r->oid_u16[1], robj->imgs);
		if (!err)
			goto _success;
	}

	_resource_node_free(robj);
	return err;

_success:
	img->imgs = robj->imgs;
	img->num = r->num;
	return 0;
}

static void rc_strings_release(void *data) {
	struct rc_str_obj *robj = data;
	lvgl_res_unload_strings(robj->rstr, robj->obj->num);
	_resource_node_free(robj);
}

static int rc_load_strings(struct rc_context *rc, struct resource *r) {
	struct string_obj *ss = (struct string_obj *)r->pres;
	struct rc_str_obj *robj;
	lvgl_res_string_t *rstr;
	struct group_node *grp;
	size_t alloc_size;
	int err;

	alloc_size = sizeof(*robj) + (sizeof(struct c_string) + sizeof(lvgl_res_string_t)) * r->num;
	robj = _resource_node_alloc(alloc_size, rc_strings_release, &rc->res_list);
	if (!robj)
		return -ENOMEM;

	rstr = (lvgl_res_string_t *)(robj->cstr + r->num);
	err = lvgl_res_load_strings_from_scene(&rc->scene, r->oid_ptr, 
        rstr, r->num);
	if (!err)
		goto _success;
	
	list_for_each_entry(grp, &rc->grp_list, node) {
		err = lvgl_res_load_strings_from_group(&grp->group, r->oid_ptr, 
            rstr, r->num);
		if (!err)
			goto _success;
	}

	_resource_node_free(robj);
	return err;

_success:
	ss->ss = robj->cstr;
	ss->num = r->num;
	robj->rstr = rstr;
	robj->obj = ss;
	for (size_t i = 0; i < r->num; i++) 
		robj->cstr[i].ctext = rstr[i].txt;
	return 0;
}

static void rc_font_release(void *data) {
	struct rc_font_obj *fobj = data;
	lvgl_bitmap_font_close(&fobj->font);
	_resource_node_free(data);
}

static int rc_load_font(struct rc_context *rc, struct resource *r) {
	struct rc_font_obj *fobj;
	int err;

	if (!r->pres)
		return -EINVAL;

	fobj = _resource_node_alloc(sizeof(*fobj), rc_font_release, &rc->res_list);
	if (!fobj)
		return -ENOMEM;

	err = lvgl_bitmap_font_open(&fobj->font, r->oid_str);
	if (err)
		_resource_node_free(fobj);

	fobj->obj = (struct font_obj *)r->pres;
	fobj->obj->font = &fobj->font;
	return 0;
}

static int rc_load(struct rm_class *cls, void *handle, struct resource *r) { 
	struct rc_context *rc = handle;
	(void) cls;

	if (rc == NULL || r == NULL)
		return -EINVAL;

	switch (r->type) {
	case UI_RES_PICTURE:
		return rc_load_pictures(rc, r);
	case UI_RES_STRING:
		return rc_load_strings(rc, r);
	case UI_RES_FONT:
		return rc_load_font(rc, r);
	default:
		return -EINVAL;
	}
	return -EINVAL;
}
	
static void rc_unload(struct rm_class *cls, void *handle, struct resource *r) {
	struct rc_context *rc = handle;
	if (rc == NULL || r == NULL)
		return;

	switch (r->type) {
	case UI_RES_PICTURE: {
		struct image_obj *img = (struct image_obj *)r->pres;
		struct rc_img_obj *robj;
		RC_ASSERT(img != NULL);
		robj = container_of(img->imgs, struct rc_img_obj, imgs);
		lvgl_res_unload_pictures(img->imgs, img->num);
		_resource_node_free(robj);
		}
		break;
	case UI_RES_STRING: {
		struct string_obj *obj = (struct string_obj *)r->pres;
		struct rc_str_obj *robj;
		RC_ASSERT(obj != NULL);
		robj = container_of(obj->ss, struct rc_str_obj, cstr);
		lvgl_res_unload_strings(robj->rstr, robj->obj->num);
		_resource_node_free(robj);
		}
		break;
	case UI_RES_FONT: {
		struct font_obj *obj = (struct font_obj *)r->pres;
		struct rc_font_obj *robj;
		RC_ASSERT(obj != NULL);
		robj = container_of(obj->font, struct rc_font_obj, font);
		lvgl_bitmap_font_close(obj->font);
		_resource_node_free(robj);
		}
		break;
	default:
		return;
	}
}

static struct rm_actions_class rc_actions = {
    .base = {
        .open   = rc_open,
        .close  = rc_close,
        .load   = rc_load,
        .unload = rc_unload,
		.alloc  = NULL, //TODO: malloc
		.free   = NULL  //TODO: free
    }
};

int rc_actions_init(const char *pic_file, const char *str_file, 
    const char *sty_file) {
    rc_pool_init();
    rc_actions.pic_file = pic_file;
    rc_actions.str_file = str_file;
    rc_actions.sty_file = sty_file;
    return ui_resource_manager_register(&rc_actions.base);
}

int rc_actions_subrc_register(struct subrc_node *subrc_nd) {
	struct subrc_node *subrc;

	if (subrc_nd == NULL)
		return -EINVAL;

	for (subrc = subrc_root; subrc; subrc = subrc->next) {
		if (subrc == subrc_nd)
			return -EEXIST;
	}

	subrc_nd->next = subrc_root;
	subrc_root = subrc_nd;
	return 0;
}
