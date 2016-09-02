//
// Created by Matej on 15. 06. 2016.
//

#include "Gestures.h"
#include "pebble.h"
#include "Buttons.h"
#include "NotificationsWindow.h"

static int16_t prevRoll = 0;
static uint8_t waitNextNSamplesForStabilisation = 0;
static uint16_t ignoreNextNumDetections = 0;
static uint16_t ignoreNextNumSamples = 0;
static bool resetPrevRoll = true;

static int16_t substract_angles(int16_t a, int16_t b)
{
    int16_t res = a - b;
    while (res < -180)
        res += 360;
    while (res > 180)
        res -= 360;

    return res;
}

static void scroll_gesture_detected(bool down)
{
    if (ignoreNextNumDetections > 0)
    {
        ignoreNextNumDetections--;
        return;
    }

    if (down)
        nw_simulate_button_down();
    else
        nw_simulate_button_up();

    ignoreNextNumSamples = 10;
    resetPrevRoll = true;
}

static void shake_gesture_detected()
{
    accelerometer_shake(ACCEL_AXIS_X, 0);
    ignoreNextNumSamples = 10;
    resetPrevRoll = true;
    waitNextNSamplesForStabilisation = 0;
}

static void accel_data_received(AccelData *data, uint32_t num_samples) {

    if (ignoreNextNumSamples > 0)
    {
        ignoreNextNumSamples--;
        return;
    }

    if (vibrating)
        return;

    AccelData firstEntry = data[0];
    uint16_t sum = (uint16_t) (abs(firstEntry.x) + abs(firstEntry.y) + abs(firstEntry.z));

    // If there is really a lot of acceleration going on, user must be shaking his hand
    if (sum > 5000)
    {
        shake_gesture_detected();
        return;
    }


    // Any movement can seriously mess up the detection
    // Ignore all samples where acceleration appears to deviate from standard gravity acceleration too much
    if (sum < 1000 || sum > 1700)
        return;

    // Calculate roll
    int16_t roll = (int16_t) atan2_lookup(firstEntry.y, firstEntry.z);
    roll = (int16_t) TRIGANGLE_TO_DEG(roll);

    // If watch is not in "looking at position", we should ignore all events
    if ((roll < 70 && roll > 0) || (roll > -70 && roll < 0))
        return;

    if (resetPrevRoll)
    {
        resetPrevRoll = false;
        prevRoll = roll;
        return;
    }

    int16_t diff = substract_angles(roll, prevRoll);

    if (waitNextNSamplesForStabilisation == 0)
    {
        if (abs(diff) > 70)
        {
            // Sometimes high differences can be caused by sudden movement that has slipped through earlier filters.
            // Wait for the next 2 measurements (3rd is the checking one) for watch to stabilize
            // And check again if difference is still there

            waitNextNSamplesForStabilisation = 3;
            return;
        }

    }
    else if (waitNextNSamplesForStabilisation == 1)
    {
        if (abs(diff) > 70)
        {
            bool down = diff > 0;
            scroll_gesture_detected(down);
            return;
        }
    }
    else
    {
        waitNextNSamplesForStabilisation--;

        // We should not be updating prevRoll when we are not sure about the
        // new measurements and we wait for the watch to stabilize
        return;
    }

    prevRoll = roll;
}

void nw_gestures_init() {
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
    accel_data_service_subscribe(1, accel_data_received);
}

void nw_gestures_deinit() {
    accel_data_service_unsubscribe();
}
