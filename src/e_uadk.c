/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <openssl/engine.h>
#include <uadk/wd.h>
#include "uadk.h"
#include "uadk_async.h"

/* Constants used when creating the ENGINE */
const char *engine_uadk_id = "uadk";
static const char *engine_uadk_name = "uadk hardware engine support";

static int uadk_cipher;
static int uadk_digest;
static int uadk_rsa;
static int uadk_pkey;

__attribute__((constructor))
static void uadk_constructor(void)
{
}

__attribute__((destructor))
static void uadk_destructor(void)
{
}

static int uadk_destroy(ENGINE *e)
{
	if (uadk_cipher)
		uadk_destroy_cipher();
	if (uadk_digest)
		uadk_destroy_digest();
	if (uadk_rsa)
		uadk_destroy_rsa();
	if (uadk_pkey)
		uadk_destroy_ecc();
	return 1;
}


static int uadk_init(ENGINE *e)
{
	uadk_init_ecc();
	uadk_init_cipher();
	return 1;
}

static int uadk_finish(ENGINE *e)
{
	return 1;
}

static void engine_init_child_at_fork_handler(void)
{
	async_module_init();
}

/*
 * This stuff is needed if this ENGINE is being
 * compiled into a self-contained shared-library.
 */
static int bind_fn(ENGINE *e, const char *id)
{
	struct uacce_dev_list *list;

	if (id && (strcmp(id, engine_uadk_id) != 0)) {
		fprintf(stderr, "wrong engine id\n");
		return 0;
	}

	if (!ENGINE_set_id(e, engine_uadk_id) ||
	    !ENGINE_set_destroy_function(e, uadk_destroy) ||
	    !ENGINE_set_init_function(e, uadk_init) ||
	    !ENGINE_set_finish_function(e, uadk_finish) ||
	    !ENGINE_set_name(e, engine_uadk_name)) {
		fprintf(stderr, "bind failed\n");
		return 0;
	}

	async_module_init();
	pthread_atfork(NULL, NULL, engine_init_child_at_fork_handler);

	list = wd_get_accel_list("cipher");
	if (list) {
		if (!uadk_bind_cipher(e, list))
			fprintf(stderr, "uadk bind cipher failed\n");
		else
			uadk_cipher = 1;

		wd_free_list_accels(list);
	}

	list = wd_get_accel_list("digest");
	if (list) {
		if (!uadk_bind_digest(e, list))
			fprintf(stderr, "uadk bind digest failed\n");
		else
			uadk_digest = 1;

		wd_free_list_accels(list);
	}

	list = wd_get_accel_list("rsa");
	if (list) {
		if (!uadk_bind_rsa(e))
			fprintf(stderr, "uadk bind rsa failed\n");
		else
			uadk_rsa = 1;

		wd_free_list_accels(list);
	}

	list = wd_get_accel_list("sm2");
	if (list) {
		if (!uadk_bind_pkey(e))
			fprintf(stderr, "uadk bind pkey failed\n");
		else
			uadk_pkey = 1;

		wd_free_list_accels(list);
	}

	return 1;
}

IMPLEMENT_DYNAMIC_CHECK_FN()
IMPLEMENT_DYNAMIC_BIND_FN(bind_fn)
