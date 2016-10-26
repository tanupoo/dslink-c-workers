/*
 * Copyright (C) 2000 Shoichi Sakane <sakane@tanu.org>, All rights reserved.
 * See the file LICENSE in the top level directory for more details.
 */
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <err.h>

#include <dslink/dslink.h>
#define LOG_TAG "dsbench"	// dslink/log.h requires...
#include <dslink/log.h>
#include <dslink/ws.h>
#include <dslink/utils.h>
#include <jansson.h>
#include <uv.h>

#include "pub.h"
#include "pub_factory.h"

typedef struct {
	DSLink *link;
	DSNode *super_root;
	/* loop = &link->loop; */
} base_loop_ctx_t;

typedef struct {
	base_loop_ctx_t *base;
	DSNode *node;
	char *node_name;
	void (*work_main)(uv_work_t *);
	int working;
	int finish;
	char *string_value;
} work_ctx_t;

//=============
void
do_work_hard_each(uv_work_t *work)
{
	work_ctx_t *wc = (work_ctx_t *)work->data;

	if (f_debug) {
		printf("%s", wc->node_name);
		fflush(stdout);
	}
}

void
do_heavy_work_for20s(uv_work_t *work)
{
	static int counter = 1;
	work_ctx_t *wc = (work_ctx_t *)work->data;

	usleep(2000000);

	int buf_size = 10;
	char *buf = malloc(buf_size);
	snprintf(buf, buf_size, "%d", counter++);
	wc->string_value = buf;

	if (f_debug) {
		printf("%s", wc->node_name);
		fflush(stdout);
	}

	/* finish this work */
	if (counter > 10)
		wc->finish++;
}

void
do_heavy_work_for30s(uv_work_t *work)
{
	static int counter = 1;
	work_ctx_t *wc = (work_ctx_t *)work->data;

	usleep(3000000);

	int buf_size = 10;
	char *buf = malloc(buf_size);
	snprintf(buf, buf_size, "%d", counter++);
	wc->string_value = buf;
	printf("%s", wc->node_name);
	fflush(stdout);

	if (f_debug) {
		printf("%s", wc->node_name);
		fflush(stdout);
	}

	/* finish this work */
	if (counter > 10)
		wc->finish++;
}
//=============

void
work_ctx_destroy(work_ctx_t *wc)
{
	if (wc->string_value != NULL)
		free(wc->string_value);

	free(wc->node_name);
	free(wc);
}

work_ctx_t *
work_ctx_new(base_loop_ctx_t *base, DSNode *node, char *node_name,
		void (*work_main)(uv_work_t *))
{
	work_ctx_t *wc = malloc(sizeof(*wc));
	if (wc == NULL) {
		log_err("failed to malloc(value_holder)\n");
		return NULL;
	}
	wc->base = base;
	wc->node = node;
	wc->node_name = strdup(node_name);
	wc->work_main = work_main;
	wc->working = 0;
	wc->finish = 0;
	wc->string_value = NULL;

	return wc;
}

void
queue_work_finished(uv_work_t *work, int status)
{
	work_ctx_t *wc = (work_ctx_t *)work->data;

	/*
	 * set value to topic node
	 */
	if (wc->string_value) {
		json_t *j_val = json_string(wc->string_value);

		if (dslink_node_set_value(wc->base->link, wc->node, j_val) != 0) {
			log_warn("failed to set the value of %s\n", wc->node_name);
			json_decref(j_val);
			dslink_node_tree_free(wc->base->link, wc->node);
			return;
		}

		free(wc->string_value);
		wc->string_value = NULL;
	}

	/* clean this work */
	free(work);
	wc->working--;
}

void
queue_work_start(work_ctx_t *wc)
{
	uv_work_t *work = malloc(sizeof(*work));
	work->data = wc;
	uv_queue_work(&wc->base->link->loop, work, wc->work_main,
			queue_work_finished);
	wc->working++;

	if (f_debug) {
		printf("+");
		fflush(stdout);
	}
}

void
heavy_work_timer_close(uv_handle_t *handle)
{
	uv_timer_t *timer = (uv_timer_t *)handle;
	free(timer);
}

static void
on_heavy_work_timer(uv_timer_t *timer)
{
	work_ctx_t *wc = (work_ctx_t *)timer->data;

	/*
	 * here, should be checked whether this task should work more or not.
	 */
	if (wc->finish) {
		/* shutdown */
		log_info("%s done\n", wc->node_name);
		uv_timer_stop(timer);
		uv_close((uv_handle_t *)timer, heavy_work_timer_close);
		work_ctx_destroy(wc);
		return;
	}

	/*
	 * check whether this task has been working already.
	 */
#if 1
	if (wc->working)
		return;
#endif

	queue_work_start(wc);
}

int
heavy_work_start(base_loop_ctx_t *base, char *node_name, useconds_t interval,
		void (*work_main)(uv_work_t *))
{
	/*
	 * create a node
	 */
	DSNode *node = dslink_node_create(base->super_root, node_name, "node");
	if (!node) {
		log_warn("failed to create a node, %s.\n", node_name);
		return -1;
	}

	json_t *j = json_string("string");
	if (dslink_node_set_meta(base->link, node, "$type", j) != 0) {
		log_warn("failed to set the type.\n");
		dslink_node_tree_free(base->link, node);
		json_decref(j);
		return -1;
	}
	json_decref(j);

	if (dslink_node_add_child(base->link, node) != 0) {
		log_warn("failed to add a node, %s\n", node_name);
		dslink_node_tree_free(base->link, node);
		return -1;
	}

	/*
	 * set timer
	 */
	uv_timer_t *timer = malloc(sizeof(*timer));
	uv_timer_init(&base->link->loop, timer);
	timer->data = work_ctx_new(base, node, node_name, work_main);
	uv_timer_start(timer, on_heavy_work_timer, 0, interval);

	return 0;
}

void
base_loop_ctx_destroy(base_loop_ctx_t *base)
{
	free(base);
}

base_loop_ctx_t *
base_loop_ctx_new(DSLink *link, DSNode *super_root)
{
	base_loop_ctx_t *base = malloc(sizeof(*base));
	base->link = link;
	base->super_root = super_root;

	return base;
}

static int
factory_init(DSLink *link, DSNode *super_root)
{
	base_loop_ctx_t *base = base_loop_ctx_new(link, super_root);

	/*
	 * here create timers for the threads working heavy.
	 */
	heavy_work_start(base, "A", 100, do_heavy_work_for20s);
	heavy_work_start(base, "B", 200, do_heavy_work_for30s);
	heavy_work_start(base, "C", 300, do_work_hard_each);

	return 0;
}

static void
pub_on_init(DSLink *link)
{
	if (factory_init(link, link->responder->super_root) < 0)
		errx(1, "ERROR: factory_init() failed.");

	log_info("Initialized!\n");
}

static void
pub_on_connected(DSLink *link)
{
	log_info("Connected!\n");
}

static void
pub_on_disconnected(DSLink *link)
{
	log_info("Disconnected!\n");
}

int
responder_start(int argc, char **argv, char *my_name)
{
	DSLinkCallbacks cbs = {
	    pub_on_init,
	    pub_on_connected,
	    pub_on_disconnected,
	    NULL
	};

	return dslink_init(argc, argv, my_name, 0, 1, &cbs);
}
