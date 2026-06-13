#include "DShot.h"

DMAMEM volatile uint32_t dshot_dma_buffer[33] __attribute__((aligned(32)));

DShot::DShot(){}

void DShot::reset()
{
  for(int i=0; i<4; ++i)
  {
    throttle[i] = 0.0f;
    dshot_cmd[i] = DshotCMD();
    output[i] = 0;
  }
  for(int i=0; i<33; ++i)
  {
    dshot_dma_buffer[i] = 0;
  }
}

void DShot::begin()
{
  // expand table
  for(int i=0; i<16; ++i)
  {
    DSHOT_WAVEFORM_L[i] =
      ((uint32_t)(C_DSHOT_WAVEFORM_L[(i >> 0) & 1u]) << 0) |
      ((uint32_t)(C_DSHOT_WAVEFORM_L[(i >> 1) & 1u]) << 1) |
      ((uint32_t)(C_DSHOT_WAVEFORM_L[(i >> 2) & 1u]) << 2) |
      ((uint32_t)(C_DSHOT_WAVEFORM_L[(i >> 3) & 1u]) << 4);
    DSHOT_WAVEFORM_H[i] =
      ((uint32_t)(C_DSHOT_WAVEFORM_H[(i >> 0) & 1u]) << 0) |
      ((uint32_t)(C_DSHOT_WAVEFORM_H[(i >> 1) & 1u]) << 1) |
      ((uint32_t)(C_DSHOT_WAVEFORM_H[(i >> 2) & 1u]) << 2) |
      ((uint32_t)(C_DSHOT_WAVEFORM_H[(i >> 3) & 1u]) << 4);
  }

  // https://github.com/easone/teensy4_FlexIO_parallel_example/blob/main/teensy4_FlexIO_parallel_example.ino
  
  pFlex = FlexIOHandler::flexIOHandler_list[0]; // Get a FlexIO channel, FlexIO1
  pFlex_p = &pFlex->port(); // Pointer to the port structure in the FlexIO channel
  pFlex_hw = &pFlex->hardware(); // Pointer to the hardware structure in the FlexIO channel

  pinMode(2, OUTPUT); // FlexIO1_D04
  pinMode(3, OUTPUT); // FlexIO1_D05
  pinMode(4, OUTPUT); // FlexIO1_D06
  pinMode(5, OUTPUT); // FlexIO1_D08

  *(portControlRegister(2)) = 0xFF; // High speed and drive strength configuration
  *(portControlRegister(3)) = 0xFF;
  *(portControlRegister(4)) = 0xFF;
  *(portControlRegister(5)) = 0xFF;

  pFlex->setClockSettings(3, 0, 0); // Set clock 480 MHz from pll3_sw_clk

  pFlex->setIOPinToFlexMode(2); // Set up pin mux
  pFlex->setIOPinToFlexMode(3);
  pFlex->setIOPinToFlexMode(4);
  pFlex->setIOPinToFlexMode(5);

  pFlex_hw->clock_gate_register |= pFlex_hw->clock_gate_mask; // Enable the clock
  
  pFlex_p->CTRL = FLEXIO_CTRL_FLEXEN | FLEXIO_CTRL_FASTACC; // Enable the FlexIO with fast access

  pFlex_p->SHIFTCTL[0] = (S0_TIMSEL<<24) | (S0_TIMPOL<<23) | (S0_PINCFG<<16) | (S0_PINSEL<<8) | (S0_PINPOL<<7) | (S0_SMOD<<0);
  pFlex_p->SHIFTCFG[0] = (S0_PWIDTH<<16) | (S0_SSTOP<<4) | (S0_SSTART<<0);

  pFlex_p->TIMCTL[0] = (T0_TRGSEL<<24) | (T0_TRGPOL<<23) | (T0_TRGSRC<<22) | (T0_PINCFG<<16) | (T0_PINSEL<<8) | (T0_PINPOL<<7) | (T0_TIMOD<<0);
  pFlex_p->TIMCFG[0] = (T0_TIMOUT<<24) | (T0_TIMDEC<<20) | (T0_TIMRST<<16) | (T0_TIMDIS<<12) | (T0_TIMENA<<8) | (T0_TSTOP<<4) | (T0_TSTART<<1);
  pFlex_p->TIMCMP[0] = ((SHIFTS_PER_TRANSFER*2-1)<<8) | ((CLOCK_DIVIDER/2-1)<<0);

  // Enable DMA trigger on Shifter 0
  pFlex_p->SHIFTSDEN |= (1<<0); // DMA request is generated when data is transferred from buffer0 to shifter0
  
  // Disable DMA channel so it doesn't start transferring yet
  dma.disable();
  unsigned int transferBytes = sizeof(dshot_dma_buffer);
  dma.sourceBuffer(dshot_dma_buffer, transferBytes); // transfer entire dataBuffer
  dma.destination(pFlex_p->SHIFTBUF[0]);
  // Set DMA channel to automatically disable on completion (manually enable it every loop)
  dma.disableOnCompletion();
  // Set up DMA channel to use Shifter 0 trigger
  dma.triggerAtHardwareEvent(pFlex_hw->shifters_dma_channel[0]);
  
  // initialize data and buffer
  reset();

  // flush DMAMEM cache
  arm_dcache_flush((void*)dshot_dma_buffer, transferBytes);
}

void DShot::write()
{
  for(int i=0; i<4; ++i)
  {
    output[i] = encodeDShot(i);
  }
  
  for (int i=0; i<16; ++i)
  {
    uint32_t c =
      (((output[0] >> (15-i)) & 1u) << 0) |
      (((output[1] >> (15-i)) & 1u) << 1) |
      (((output[2] >> (15-i)) & 1u) << 2) |
      (((output[3] >> (15-i)) & 1u) << 3);

    dshot_dma_buffer[i*2 + 0] = DSHOT_WAVEFORM_L[c];
    dshot_dma_buffer[i*2 + 1] = DSHOT_WAVEFORM_H[c];
  }

  arm_dcache_flush((void*)dshot_dma_buffer, sizeof(dshot_dma_buffer));

  int32_t step = micros() - now;
  now += step;
  if(wait)
  {
    wait = max(wait-step, 0);
  }
  else
  {
    dma.enable();
  }
}

// 2000 + 48
uint16_t DShot::encodeDShot(const uint32_t& c)
{
  uint16_t x = 0;
  if(dshot_cmd[c].cmd) // if cmd
  {
    if(dshot_cmd[c].again) // cmd again
    {
      x = (dshot_cmd[c].cmd << 1) | 1; // telemetry bit is needed
      --dshot_cmd[c].again;
    }
    else if(dshot_cmd[c].wait > 0) // if wait
    {
      wait = max(wait, dshot_cmd[c].wait); // set wait time
      dshot_cmd[c] = {0, 0, 0}; // cmd done, reset
    }
  }
  else
  {
    // clamp
    if(throttle[c] < 0.010f) return 0;
    x = static_cast<uint16_t>(throttle[c] * 2000.0f);
    // 11 bits throttle, 1 bit telemetry (0 off)
    x = (constrain(x, 20, 2000) + 47) << 1;
  }
  // 4-bit CRC
  uint16_t crc = ((x ^ (x >> 4)) ^ (x >> 8)) & 0x0F;
  // 16-bit frame
  return ((x << 4) | crc);
}