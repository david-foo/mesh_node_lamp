#include <stdlib.h>
#include <sys/printk.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include <display/mb_display.h>

#include "board.h"
#include "main.h"
#include "level_srv.h"
#include "generic_level.h"



// record the last package msgs
s64_t last_message_timestamp;
u16_t last_message_src;
u16_t last_message_dst;
u8_t last_message_tid;


static u8_t time2tt(u32_t timems)
{
	u8_t steps = 0;
	u8_t resolution = 0;
	if (timems > 620000)
	{
		// > 620 seconds -> resolution=0b11 [10 minutes]
		resolution = 0x03;
		steps = timems / 600000;
	}
	else if (timems > 62000)
	{
		// > 62 seconds -> resolution=0b10 [10 seconds]
		resolution = 0x02;
		steps = timems / 10000;
	}
	else if (timems > 6200)
	{
		// > 6.2 seconds -> resolution=0b01 [1 seconds]
		resolution = 0x01;
		steps = timems / 1000;
	}
	else
	{
		// <= 6.2 seconds -> resolution=0b00 [100 ms]
		resolution = 0x00;
		steps = timems / 100;
	}
	printk("calculated steps=%d,resolution=%d\n", steps, resolution);
	return ((resolution << 6) | steps);
}

static u32_t tt2time(u8_t tt)
{
	uint8_t step = (tt & 0xc0) >> 6;
	uint8_t count = tt & 0x3f;
	int secs = 0, msecs = 0, minutes = 0, hours = 0;

	switch (step) {
	case 0:
		msecs = 100 * count;
		secs = msecs / 1000;
		msecs -= (secs * 1000);
		break;
	case 1:
		secs = 1 * count;
		minutes = secs / 60;
		secs -= (minutes * 60);
		break;

	case 2:
		secs = 10 * count;
		minutes = secs / 60;
		secs -= (minutes * 60);
		break;
	case 3:
		minutes = 10 * count;
		hours = minutes / 60;
		minutes -= (hours * 60);
		break;

	default:
		break;
	}

	return hours * 3600000 + minutes * 60000 + secs * 1000 + msecs;
}

static void generic_level_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	s16_t current, target;
	u8_t remaining_time;

	current = transform_get_current();
	target = transform_get_target();
	remaining_time = time2tt(transform_get_remain());

	NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 2+ 1 + 4);
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GENERIC_LEVEL_STATUS);
	net_buf_simple_add_le16(&msg, current);
	net_buf_simple_add_le16(&msg, target);
	net_buf_simple_add_u8(&msg, remaining_time);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		printk("Unable to send level Status response\n");
	}
}

static void generic_level_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("generic_level_set_unack\n");
	u8_t buflen = buf->len;
	// Level(2), TID(1), Transition Time(optional, 1), Delay (conditional, 1)
	s16_t target_level_state = (s16_t)net_buf_simple_pull_le16(buf);

	// The TID field is a transaction identifier indicating whether the message is a new message or a retransmission of a previously sent message
	u8_t tid = net_buf_simple_pull_u8(buf);

	// set the Generic Level state to the Level field of the message, unless the message has the same values for the SRC, DST, and TID fields as the
	// previous message received within the last 6 seconds.

	s64_t now = k_uptime_get(); // elapsed time since the system booted, in milliseconds.
	if (ctx->addr == last_message_src && ctx->recv_dst == last_message_dst && tid == last_message_tid && (now - last_message_timestamp <= 6000))
	{
		printk("Ignoring message - same transaction during 6 second window\n");
		return;
	}

	last_message_timestamp = now;
	last_message_src = ctx->addr;
	last_message_dst = ctx->recv_dst;
	last_message_tid = tid;

	printk("target_level_state=%d  buflen=%d\n", target_level_state, buflen);

	if (buflen > 3) {// with delay and transition time
		u8_t tt = net_buf_simple_pull_u8(buf);
		u8_t delay = net_buf_simple_pull_u8(buf);
		transform_set_level(target_level_state, delay*5, tt2time(tt));
		printk("tt=%x,delay=%d\n", tt, delay);
	} else {// without delay and transition time
		transform_set_level(target_level_state, 0, 1);
	}
}


static void generic_level_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("generic_level_set\n");
	generic_level_set_unack(model, ctx, buf);
	generic_level_get(model, ctx, buf);
}

static void generic_delta_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("generic_delta_set_unack\n");
	u8_t buflen = buf->len;
	//  Delta(4), TID(1), Transition Time(optional, 1), Delay (conditional, 1)
	s32_t target_level_state = (s32_t)net_buf_simple_pull_le32(buf);

	// The TID field is a transaction identifier indicating whether the message is a new message or a retransmission of a previously sent message
	u8_t tid = net_buf_simple_pull_u8(buf);

	// set the Generic Level state to the Level field of the message, unless the message has the same values for the SRC, DST, and TID fields as the
	// previous message received within the last 6 seconds.

	s64_t now = k_uptime_get(); // elapsed time since the system booted, in milliseconds.
	if (ctx->addr == last_message_src && ctx->recv_dst == last_message_dst && tid == last_message_tid && (now - last_message_timestamp <= 6000))
	{
		printk("Ignoring message - same transaction during 6 second window\n");
		return;
	}

	last_message_timestamp = now;
	last_message_src = ctx->addr;
	last_message_dst = ctx->recv_dst;
	last_message_tid = tid;

	printk("target_level_state=%d  buflen=%d\n", target_level_state, buflen);
	if (buflen > 5) {// with delay and transition time
		u8_t tt = net_buf_simple_pull_u8(buf);
		u8_t delay = net_buf_simple_pull_u8(buf);
		transform_set_delta((s16_t)(target_level_state), delay*5, tt2time(tt));
	} else {// without delay and transition time
		transform_set_delta((s16_t)(target_level_state), 0, 1);
	}
}

static void generic_delta_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	generic_delta_set_unack(model, ctx, buf);
	generic_level_get(model, ctx, buf);
}

static void generic_move_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{

}

static void generic_move_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{

}

void generic_level_status(struct bt_mesh_model *model)
{

}


const struct bt_mesh_model_op gen_level_srv_op[] = {
	{BT_MESH_MODEL_OP_2(0x82, 0x05), 0, generic_level_get},
	{BT_MESH_MODEL_OP_2(0x82, 0x06), 3, generic_level_set},
	{BT_MESH_MODEL_OP_2(0x82, 0x07), 3, generic_level_set_unack},
	{BT_MESH_MODEL_OP_2(0x82, 0x09), 5, generic_delta_set},
	{BT_MESH_MODEL_OP_2(0x82, 0x0A), 5, generic_delta_set_unack},
	{BT_MESH_MODEL_OP_2(0x82, 0x0B), 3, generic_move_set},
	{BT_MESH_MODEL_OP_2(0x82, 0x0C), 3, generic_move_set_unack},
	BT_MESH_MODEL_OP_END,
};


void level_srv_init()
{

}
