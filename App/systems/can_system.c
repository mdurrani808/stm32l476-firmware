#include "can_system.h"

#include "app_config.h"
#include "can.h"
#include "main.h"

/* Parameter storage interface */
#include "can_params.h"

/* Internal setter (not exposed in header on purpose) */
extern void CanParams__Set_SetLED(uint8_t v);

static void CAN_ConfigAcceptAllFilter(void)
{
  CAN_FilterTypeDef filter = {0};

  /* Bank 0, accept everything into FIFO0 */
  filter.FilterBank = 0;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;

  /* Accept all IDs: ID=0, MASK=0 */
  filter.FilterIdHigh = 0x0000;
  filter.FilterIdLow  = 0x0000;
  filter.FilterMaskIdHigh = 0x0000;
  filter.FilterMaskIdLow  = 0x0000;

  filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filter.FilterActivation = ENABLE;

  /* Single CAN instance safety default */
  filter.SlaveStartFilterBank = 14;

  if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK)
  {
    Error_Handler();
  }
}

/* ---- DBC Decode: BO_ 128 STEPPER_COMMAND ----
 *
 * BO_ 128 STEPPER_COMMAND: 8
 *   SG_ Command_Byte M : 0|8@1+ ...
 *   SG_ Set_LED m144M : 8|1@1+ ...
 *
 * Interpretation:
 * - Standard ID = 128 (0x80)
 * - Command_Byte is multiplexor at byte0
 * - Set_LED is only valid when Command_Byte == 144
 * - Set_LED startbit 8 len 1 Intel => bit0 of byte1
 */
static void DBC_Handle_StepperCommand(const CAN_RxHeaderTypeDef* rxHeader, const uint8_t* d)
{
  (void)rxHeader;

  const uint8_t command_byte = d[0];

  /* Only care about the multiplexed group m144M for now */
  if (command_byte == 144U)
  {
    const uint8_t set_led = (uint8_t)(d[1] & 0x01U);
    CanParams__Set_SetLED(set_led);
  }
}

static void CAN_HandleReceivedFrame(const CAN_RxHeaderTypeDef* rxHeader, const uint8_t* d)
{
  if (rxHeader->IDE != CAN_ID_STD)
  {
    return;
  }

  switch (rxHeader->StdId)
  {
    case 128U: /* 0x80 STEPPER_COMMAND */
      DBC_Handle_StepperCommand(rxHeader, d);
      break;

    default:
      /* Ignore unknown frames for now */
      break;
  }
}

static void CAN_PollReceive(void)
{
  while (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0)
  {
    CAN_RxHeaderTypeDef rxHeader = {0};
    uint8_t rxData[8] = {0};

    if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
    {
      CAN_HandleReceivedFrame(&rxHeader, rxData);
    }
  }
}

void CanSystem_Init(void)
{
  CAN_ConfigAcceptAllFilter();

  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
}

void CanSystem_Tick(void)
{
  /* RX polling + DBC decode only */
  CAN_PollReceive();
}
