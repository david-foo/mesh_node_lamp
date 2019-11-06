#ifndef _GENRIC_LEVEL_H
#define _GENRIC_LEVEL_H

s32_t transform_set_level(s16_t end, s32_t delay, s32_t trans_time);
s32_t transform_set_delta(s16_t delta, s32_t delay, s32_t trans_time);
s16_t transform_get_current(void);
s16_t transform_get_target(void);
s8_t transform_going(void);
s32_t transform_get_remain(void);
void transform_init(void);

#endif
