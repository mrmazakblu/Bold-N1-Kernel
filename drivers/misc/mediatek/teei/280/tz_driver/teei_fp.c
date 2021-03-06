/*
 * Copyright (c) 2015-2017 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/slab.h>
#include "teei_fp.h"
#include "teei_client_transfer_data.h"
#define IMSG_TAG "[tz_driver]"
#include <imsg_log.h>
#include <linux/vmalloc.h>

unsigned long fp_buff_addr;

unsigned long create_fp_fdrv(int buff_size)
{
	unsigned long addr = 0;

	if (buff_size < 1) {
		IMSG_ERROR("Wrong buffer size %d:", buff_size);
		return 0;
	}
	addr = (unsigned long) vmalloc(buff_size);
	if (addr == 0) {
		IMSG_ERROR("kmalloc buffer failed");
		return 0;
	}
	memset((void *)addr, 0, buff_size);
	return addr;
}

static struct TEEC_Context context;
static int context_initialized;
struct TEEC_UUID uuid_fp = { 0x7778c03f, 0xc30c, 0x4dd0,
{ 0xa3, 0x19, 0xea, 0x29, 0x64, 0x3d, 0x4d, 0x4b } };
int send_fp_command(void *buffer, unsigned long size)
{
	int ret = 0;

	IMSG_INFO("TEEI start send_fp_command\n");

	if (buffer == NULL || size < 1)
		return -1;

	if (context_initialized == 0) {
		memset(&context, 0, sizeof(context));
		ret = ut_pf_gp_initialize_context(&context);
		if (ret) {
			IMSG_ERROR("Failed to initialize fp context ,err: %x",
			ret);
			goto release_1;
		}
		context_initialized = 1;
	}
	ret = ut_pf_gp_transfer_data(&context, &uuid_fp, 1, buffer, size);
	if (ret) {
		IMSG_ERROR("Failed to transfer data,err: %x", ret);
		goto release_2;
	}
release_2:
	if (ret) {
		ut_pf_gp_finalize_context(&context);
		context_initialized = 0;
	}
release_1:
	IMSG_INFO("TEEI end of send_fp_command\n");
	return ret;
}
