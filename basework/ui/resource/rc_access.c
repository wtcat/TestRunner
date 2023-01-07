/*
 * Copyright 2022 wtcat
 * 
 * UI resource access 
 */
#include <errno.h>
#include "basework/ui/resource/rc_access.h"


static struct rm_class *rm_class_ops;

int ui_resource_open(const void *oid, void **handle) {
	RC_ASSERT(rm_class_ops != NULL);
	if (rm_class_ops->open)
		return rm_class_ops->open(rm_class_ops, oid, handle);
	return -EINVAL;
}

void ui_resource_close(void *handle) {
	RC_ASSERT(rm_class_ops != NULL);
	if (rm_class_ops->close)
		rm_class_ops->close(rm_class_ops, handle);
}

int ui_resource_get(void *handle, struct resource *r) {
	RC_ASSERT(rm_class_ops != NULL);
	RC_ASSERT(rm_class_ops->load != NULL);
	return rm_class_ops->load(rm_class_ops, handle, r);
	return -EINVAL;
}

int ui_resource_get_ext(void *handle, enum resource_type type, uintptr_t oid, 
	size_t num, void *res) {
	struct resource r = {
		.type = type,
		.oid = oid,
		.num = num,
		.pres = res,
		.pext = NULL
	};
	return ui_resource_get(handle, &r);
}

void ui_resource_put(void *handle, struct resource *r) {
	RC_ASSERT(rm_class_ops != NULL);
	if (rm_class_ops->unload)
		rm_class_ops->unload(rm_class_ops, handle, r);
}

int ui_resource_manager_register(struct rm_class *rm) {
	if (rm == NULL)
		return -EINVAL;
	if (rm->load == NULL)
		return -EINVAL;
	if (rm_class_ops != NULL)
		return -EBUSY;
	
	rm_class_ops = rm;
	return 0;
}

void *_resource_malloc(size_t size) {
	RC_ASSERT(rm_class_ops != NULL);
	RC_ASSERT(rm_class_ops->alloc != NULL);
	return rm_class_ops->alloc(size);
}

void _resource_mfree(void *ptr) {
	RC_ASSERT(rm_class_ops != NULL);
	RC_ASSERT(rm_class_ops->free != NULL);
	if (ptr)
		rm_class_ops->free(ptr);
}

void *_resource_node_alloc(size_t size, void (*release)(void *), 
	struct list_head *head) {
	size_t alloc_size = sizeof(struct resource_node) + size;
	struct resource_node *rn;
	rn = _resource_malloc(alloc_size);
	if (rn) {
		rn->release = release;
		list_add_tail(&rn->node, head);
		return rn->data;
	}
	return NULL;
}

void _resource_node_free(void *r) {
	struct resource_node *p = container_of(r, struct resource_node, data);
	list_del(&p->node);
	_resource_mfree(p);
}

void _resources_release(struct list_head *head) {
	struct resource_node *r, *n;
	list_for_each_entry_safe_reverse(r, n, head, node){
		if (r->release)
			r->release(r->data);	
	}
}
