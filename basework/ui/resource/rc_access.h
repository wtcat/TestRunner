/*
 * Copyright 2022 wtcat
 */
#ifndef UI_RC_ACCESS_H_
#define UI_RC_ACCESS_H_

#include <assert.h>
#include <lvgl.h>

#include "basework/container/list.h"

#ifdef __cplusplus
extern "C"{
#endif

#define RC_ASSERT(c) assert(c)

enum resource_type {
	UI_RES_PICTURE,
	UI_RES_STRING,
	UI_RES_FONT,
	UI_RES_MAX
};

struct c_string {
	const char *ctext;
};

struct image_obj {
    lv_img_dsc_t *imgs;
    size_t num;
};

struct string_obj {
	struct c_string *ss;
	size_t num;
};

struct font_obj {
	lv_font_t *font;
};

struct resource_node {
	struct list_head node;
	void (*release)(void *data);
	char data[0];
};

struct resource {
    enum resource_type type;
	union {
		char *oid_str;
		void *oid_ptr;
		uint32_t oid_u32;
		uint16_t oid_u16[2];
        uintptr_t oid;
	};
	size_t num;
	void *pres; /* Point to resource */
	void *pext;
};

struct rm_class {
	int  (*open)(struct rm_class *cls, const void *oid, void **handle);
	void (*close)(struct rm_class *cls, void *handle);
	int  (*load)(struct rm_class *cls, void *handle, struct resource *r);
	void (*unload)(struct rm_class *cls, void *handle, struct resource *r);

	/* Memory allocate interface */
	void *(*alloc)(size_t);
	void (*free)(void *);
};

/*
 * _resource_malloc - allocate memory(private interface).
 *
 * @size: request size
 * return NULL if failed
 */
void *_resource_malloc(size_t size);

/*
 * __resource_alloc - free memory(private interface).
 *
 * @ptr: pointer to memory
 */
void _resource_mfree(void *ptr);

/*
 * _resource_alloc - allocate memory for resource (private interface).
 *
 * @size: request size
 * @release: resource release callback
 * @head: resource list
 * return NULL if failed
 */
void *_resource_node_alloc(size_t size, void (*release)(void *), 
	struct list_head *head);

/*
 * _resource_node_free - free resource node (private interface).
 *
 * @r: resource pointer
 */
void _resource_node_free(void *r);

/*
 * _resources_release - release all resource nodes (private interface).
 * @head: resource list
 */
void _resources_release(struct list_head *head);

/*
 * ui_resource_open - open resource table
 *
 * @oid: resource table id
 * @handle: resource table handle
 * return 0 if success
 */
int ui_resource_open(const void *oid, void **handle);

/*
 * ui_resource_close - close resource table
 *
 * @handle: resource table handle
 */
void ui_resource_close(void *handle);

/*
 * ui_resource_get - get the specified resource
 *
 * @handle: resource table handle
 * @r: resource object
 * return 0 if success
 */
int ui_resource_get(void *handle, struct resource *r);

// int ui_resource_get_ext(void *handle, enum resource_type type, uintptr_t oid, 
// 	size_t num, void *res);

/*
 * ui_resource_put - free the resource
 *
 * @handle: resource table handle
 * @r: resource object
 */
void ui_resource_put(void *handle, struct resource *r);

/*
 * ui_resource_manager_register - register the resource manager
 *
 * @rm: resource manager object
 * return 0 if success
 */
int ui_resource_manager_register(struct rm_class *rm);

#ifdef __cplusplus
}
#endif
#endif /* UI_RC_ACCESS_H_ */
