It uses FlexIO and DMA, based on https://github.com/easone/teensy4_FlexIO_parallel_example


// declare

DShot wingbeater;

// setup

wingbeater.begin();

// set throttle (float, unit is fulll throttle)

// 0 1 2 3 maps pin 2 3 4 5

wingbeater.throttle[0] = 0.1f;

// whem any command is set, it will do only the commands

wingbeater.dshot_cmd[0] = DSHOT_CMD_SAVE_SETTINGS

// beat the wings, call every loop

wingbeater.write();



it handles call repeats and waiting time, DSHOT_TEST_SPIN helps with testing motor direction

list of commands:

const DshotCMD DSHOT_CMD_NONE                    = {0, 0, 0};

const DshotCMD DSHOT_CMD_SPIN_DIRECTION_1        = {7, 10, 0}; // 6x

const DshotCMD DSHOT_CMD_SPIN_DIRECTION_2        = {8, 10, 0}; // 6x

const DshotCMD DSHOT_CMD_3D_MODE_OFF             = {9, 10, 0}; // 6x

const DshotCMD DSHOT_CMD_3D_MODE_ON              = {10, 10, 0};  // 6x

const DshotCMD DSHOT_CMD_SAVE_SETTINGS           = {12, 10, 35000}; // 6x, wait 35ms

const DshotCMD DSHOT_CMD_SPIN_DIRECTION_NORMAL   = {20, 10, 0}; // 6x

const DshotCMD DSHOT_CMD_SPIN_DIRECTION_REVERSED = {21, 10, 0}; // 6x
const DshotCMD DSHOT_TEST_SPIN                   = {248, 12000, 0}; // 3s
