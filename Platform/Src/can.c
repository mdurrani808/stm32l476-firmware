#include "can.h"

CAN_HandleTypeDef hcan1;

void MX_CAN1_Init(void)
{
  /* Clock enable is also done in MSP init; leaving it here is harmless */
  __HAL_RCC_CAN1_CLK_ENABLE();

  hcan1.Instance = CAN1;

  /*
   * CAN timing (TEST):
   * Target: ~1 Mbps assuming CAN kernel clock = 8 MHz
   *
   * tq = (Prescaler / fCANCLK)
   * bit_time = (1 + BS1 + BS2) * tq
   *
   * Prescaler=1, BS1=5, BS2=2  => total tq = 8
   * If fCANCLK=8 MHz => bit_time = 8/8e6 = 1 us => 1 Mbps
   *
   * If your HSE / CAN clock is not 8 MHz, change these values.
   */
  hcan1.Init.Prescaler = 1;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_5TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;

  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;

  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }

  /* NOTE:
   * - Do NOT start CAN here.
   * - Filters + HAL_CAN_Start() belong in the App CAN system.
   */
}
