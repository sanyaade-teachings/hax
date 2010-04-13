#include <hax.h>
#include <stdio.h>

#include "encoder.h"
#include "ports.h"
#include "util.h"

#include "robot.h"

void drive_raw(AnalogOut left, AnalogOut right) {
	motor_set(MTR_DRIVE_L, +left);
	motor_set(MTR_DRIVE_R, -right);
}

void drive_smart(AnalogOut forward, AnalogOut turn) {
	int16_t left  = (int16_t) forward - turn;
	int16_t right = (int16_t) forward + turn;
	int16_t max   = MAX(ABS(left), ABS(right));

	/* Scale the values to not exceed kMotorMax. */
	if (max > kMotorMax) {
		left  = left  * kMotorMax / max;
		right = right * kMotorMax / max;
	}

	motor_set(MTR_DRIVE_L, -left);
	motor_set(MTR_DRIVE_R, +right);
}

void arm_raw(AnalogOut vel) {
	motor_set(MTR_ARM_A1, +vel);
	motor_set(MTR_ARM_A2, +vel);
	motor_set(MTR_ARM_B1, -vel);
	motor_set(MTR_ARM_B2, -vel);
}

bool arm_smart(AnalogOut vel) {
	int16_t pos  = analog_adc_get(POT_ARM);
	bool    up   = vel > 0 && ARM_LT(pos, POT_ARM_HIGH);
	bool    down = vel < 0 && ARM_GT(pos, POT_ARM_LOW);
	bool    move = up || down;

	arm_raw(move * vel);

	return !move;
}

void ramp_raw(AnalogOut vel) {
	motor_set(MTR_LIFT_L, -vel);
	motor_set(MTR_LIFT_R, +vel);
}

bool ramp_smart(AnalogOut vel) {
	int16_t pos  = analog_adc_get(POT_LIFT);
	bool    up   = vel > 0 && LIFT_LT(pos, POT_LIFT_HIGH);
	bool    down = vel < 0 && LIFT_GT(pos, POT_LIFT_LOW);
	bool    move = up || down;

	ramp_raw(move * vel);

	return !move;
}

int32_t drive_straight(AnalogOut forward) {
	int32_t left  = encoder_get(ENC_L);
	int32_t right = encoder_get(ENC_R);

	int32_t diff  = left - right;
	int32_t turn  = SIGN(diff) * PROP(ABS(forward), DRIVE_STRAIGHT_ERRMAX, ABS(diff));

	printf((char *)"FORWARD %4d   TURN %4d\n\r", (int)forward, (int)turn), 

	drive_smart(forward, turn);

	return (left + right) / 2;
}

bool drive_turn(int16_t angle) {
	int32_t left  = encoder_get(ENC_L);
	int32_t right = encoder_get(ENC_R);
	int32_t cur   = ABS(left - right) / 2;
	int32_t tar   = (int32_t)ABS(angle) * ENC_PER_DEG;
	int32_t diff  = ABS(cur - tar);

	int32_t tmp1  = (int32_t)kMotorMax * diff / DRIVE_TURN_ERRMAX;

	int32_t turn  = PROP(kMotorMax, DRIVE_TURN_ERRMAX, diff);

	printf((char *)"LEFT %4d   RIGHT %4d   DIFF %4d  TMP %4d   SPEED %4d\n\r",
		(int)left, (int)right, (int)diff, (int)tmp1, (int)turn);

	drive_smart(0, SIGN(angle) * turn);

	return ABS(cur - tar) <= DRIVE_TURN_ERRMIN;
}