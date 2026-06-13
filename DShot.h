#ifndef DSHOT_h
#define DSHOT_h
#include <Arduino.h>
#include <FlexIO_t4.h>
#include <DMAChannel.h>

// Shifter 0 registers
#define S0_TIMSEL 0 // Use timer 0
#define S0_TIMPOL 1 // Shift on negedge of clock 
#define S0_PINCFG 3 // Shifter pin output
#define S0_PINSEL 4 // Select pins FXIO1_D4
#define S0_PINPOL 0 // Shifter pin active high polarity
#define S0_SMOD 2 // Shifter transmit mode
#define S0_PWIDTH 7 // 1-bit parallel shift width
#define S0_SSTOP 0 // 0 = Stop bit disabled
#define S0_SSTART 0 // Start bit disabled, transmitter loads data on enable
// Timer 0 registers
#define T0_TRGSEL 1 // Trigger select Shifter 0 status flag (SHIFTBUF is empty)
#define T0_TRGPOL 1 // Trigger active low
#define T0_TRGSRC 1 // Internal trigger selected
#define T0_PINCFG 0 // Timer pin output disabled
#define T0_PINSEL 0 // Select pin FXIO_D0
#define T0_PINPOL 0 // Timer pin polarity active high
#define T0_TIMOD 1 // Dual 8-bit counters baud mode
#define T0_TIMOUT 1 // 00b = high & !reset; 01b = low & !reset; 10b = high & reset; 11b low & reset
#define T0_TIMDEC 0 // FlexIO clock decrements Timer counter, shift clock speed = timer output speed
#define T0_TIMRST 0 // Timer never reset
#define T0_TIMDIS 2 // Timer disabled on Timer compare (010b - Timer disabled on Timer compare (upper 8-bits match and decrement))
#define T0_TIMENA 2 // Timer enabled on Trigger high
#define T0_TSTOP 0 // 0 = Stop bit disabled
#define T0_TSTART 0 // Start bit disabled
#define SHIFTS_PER_TRANSFER 4 // 4-bit per word
#define CLOCK_DIVIDER 100 // Shift frequency is 100 times slower than 480 MHz FlexIO clock  = 4.8MHz = 0.6kHz * 8-bit-waveform

struct DshotCMD{
  uint16_t cmd;
  uint16_t again; // times
  uint16_t wait; // us
};

const DshotCMD DSHOT_CMD_NONE                    = {0, 0, 0};
const DshotCMD DSHOT_CMD_SPIN_DIRECTION_1        = {7, 10, 0}; // 6x
const DshotCMD DSHOT_CMD_SPIN_DIRECTION_2        = {8, 10, 0}; // 6x
const DshotCMD DSHOT_CMD_3D_MODE_OFF             = {9, 10, 0}; // 6x
const DshotCMD DSHOT_CMD_3D_MODE_ON              = {10, 10, 0};  // 6x
const DshotCMD DSHOT_CMD_SAVE_SETTINGS           = {12, 10, 35000}; // 6x, wait 35ms
const DshotCMD DSHOT_CMD_SPIN_DIRECTION_NORMAL   = {20, 10, 0}; // 6x
const DshotCMD DSHOT_CMD_SPIN_DIRECTION_REVERSED = {21, 10, 0}; // 6x
const DshotCMD DSHOT_TEST_SPIN                   = {248, 12000, 0}; // 3s

// Dshot waveform 8-bit resolution, pulses lengthened or ESC won't read
// 0 = 0b00000111
// 1 = 0b00111111
const uint32_t C_DSHOT_WAVEFORM_L[2] = {0x00000101, 0x01010101};
const uint32_t C_DSHOT_WAVEFORM_H[2] = {0x00000000, 0x00000001};
// 8-bit waveform to 8 FlexIO pads -> 2 uint32_t per Dshot bit. 16 Dshot bits -> 32 uint32_t DMA data + 1 to pull low.
extern volatile uint32_t dshot_dma_buffer[33];

class DShot
{
public:
  // ---- channels ----
  float throttle[4];
  uint16_t output[4];
  DshotCMD dshot_cmd[4];
  // ---- interface ----
  DShot();
  void reset();
  void begin();
  void write();

private:
  // ---- FlexIO setup ----
  FlexIOHandler *pFlex;
  IMXRT_FLEXIO_t *pFlex_p;
  const FlexIOHandler::FLEXIO_Hardware_t *pFlex_hw;
  DMAChannel dma;
  // ---- channels ----
  //uint16_t output[4];
  int32_t wait = 0;
  uint32_t now = 0;
  uint16_t encodeDShot(const uint32_t& c);
  // ---- table ----
  uint32_t DSHOT_WAVEFORM_L[16];
  uint32_t DSHOT_WAVEFORM_H[16];
};


#endif