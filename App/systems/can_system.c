#include "can_system.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> /* strtoul/strtol/strtof */

#include "can.h"
#include "main.h"

#include "can_params.h"   /* public param API */
#include "can_config.h"   /* optional RX filter allowlist */

/* DBC text blob (generated from App/dbc/file.dbc) */
extern const char* g_can_dbc_text;

/* =========================
 *  DBC parsing
 * ========================= */

typedef struct
{
  uint32_t msg_id;
  uint8_t dlc;
  char name[64];
} dbc_msg_t;

typedef struct
{
  char full_name[64]; /* "MESSAGE.SIGNAL" */
  uint32_t msg_id;
  uint8_t start_bit;
  uint8_t length;
  bool is_signed;
  float factor;
  float offset;
  canp_type_t type;
} dbc_sig_t;

#define MAX_DBC_MSGS  (64U)
#define MAX_DBC_SIGS  (256U)

static dbc_msg_t s_msgs[MAX_DBC_MSGS];
static dbc_sig_t s_sigs[MAX_DBC_SIGS];
static uint32_t s_msg_count = 0;
static uint32_t s_sig_count = 0;

/* Dirty flags for TX */
static uint8_t s_msg_dirty[MAX_DBC_MSGS];

/* Inbox/outbox state flags (backed by parameters "pending_inbox"/"pending_outbox") */
static uint8_t s_inbox_updated_since_tick = 0U;
static uint8_t s_outbox_pending = 0U;

/* =========================
 *  RX filter allowlist
 * ========================= */

static bool rx_id_allowed(uint32_t std_id)
{
  if (g_can_rx_id_filter_count == 0U)
  {
    return true;
  }

  for (size_t i = 0; i < g_can_rx_id_filter_count; i++)
  {
    if ((uint32_t)g_can_rx_id_filter[i] == std_id)
    {
      return true;
    }
  }

  return false;
}

/* =========================
 *  Helpers
 * ========================= */

static int find_msg_index_by_id(uint32_t msg_id)
{
  for (uint32_t i = 0; i < s_msg_count; i++)
  {
    if (s_msgs[i].msg_id == msg_id)
    {
      return (int)i;
    }
  }
  return -1;
}

static int find_sig_index_by_full_name(const char* full_name)
{
  for (uint32_t i = 0; i < s_sig_count; i++)
  {
    if (strcmp(s_sigs[i].full_name, full_name) == 0)
    {
      return (int)i;
    }
  }
  return -1;
}

static bool mark_dirty_for_full_name(const char* full_name)
{
  if (full_name == NULL)
  {
    return false;
  }

  int si = find_sig_index_by_full_name(full_name);
  if (si < 0)
  {
    return false;
  }

  uint32_t msg_id = s_sigs[si].msg_id;
  int mi = find_msg_index_by_id(msg_id);
  if (mi < 0)
  {
    return false;
  }

  s_msg_dirty[mi] = 1U;
  return true;
}

/* Build "MSG.SIG" into dst (size must be >= 2).
 * Returns true if it fits fully, false if truncated.
 * Always NUL-terminates.
 */
static bool build_full_name(char* dst, size_t dst_size, const char* msg_name, const char* sig_name)
{
  if (dst == NULL || dst_size == 0U || msg_name == NULL || sig_name == NULL)
  {
    return false;
  }

  dst[0] = '\0';

  size_t used = 0U;
  size_t i = 0U;

  /* copy msg_name */
  for (i = 0U; msg_name[i] != '\0'; i++)
  {
    if (used + 1U >= dst_size)
    {
      dst[dst_size - 1U] = '\0';
      return false;
    }
    dst[used++] = msg_name[i];
  }

  /* add '.' */
  if (used + 1U >= dst_size)
  {
    dst[dst_size - 1U] = '\0';
    return false;
  }
  dst[used++] = '.';

  /* copy sig_name */
  for (i = 0U; sig_name[i] != '\0'; i++)
  {
    if (used + 1U >= dst_size)
    {
      dst[dst_size - 1U] = '\0';
      return false;
    }
    dst[used++] = sig_name[i];
  }

  dst[used] = '\0';
  return true;
}

/* =========================
 *  DBC parsing implementation
 * ========================= */

static void dbc_parse_reset(void)
{
  s_msg_count = 0;
  s_sig_count = 0;

  for (uint32_t i = 0; i < MAX_DBC_MSGS; i++)
  {
    s_msgs[i].msg_id = 0;
    s_msgs[i].dlc = 0;
    s_msgs[i].name[0] = '\0';
    s_msg_dirty[i] = 0U;
  }

  for (uint32_t i = 0; i < MAX_DBC_SIGS; i++)
  {
    s_sigs[i].full_name[0] = '\0';
    s_sigs[i].msg_id = 0;
    s_sigs[i].start_bit = 0;
    s_sigs[i].length = 0;
    s_sigs[i].is_signed = false;
    s_sigs[i].factor = 1.0f;
    s_sigs[i].offset = 0.0f;
    s_sigs[i].type = CANP_TYPE_INT32;
  }
}

static bool parse_uint(const char* s, uint32_t* out)
{
  if (s == NULL || out == NULL)
  {
    return false;
  }
  char* end = NULL;
  unsigned long v = strtoul(s, &end, 10);
  if (end == s)
  {
    return false;
  }
  *out = (uint32_t)v;
  return true;
}

static bool parse_float(const char* s, float* out)
{
  if (s == NULL || out == NULL)
  {
    return false;
  }
  char* end = NULL;
  float v = strtof(s, &end);
  if (end == s)
  {
    return false;
  }
  *out = v;
  return true;
}

/* Extremely small DBC subset parser:
 * - BO_ <id> <name>: <dlc> ...
 * - SG_ <sig> : <start>|<len>@... (<factor>,<offset>) ...
 * Types are inferred from length/sign and scaling:
 *   - if factor/offset not identity -> FLOAT
 *   - else bool if len==1
 *   - else int32
 */
static void dbc_parse_all(void)
{
  dbc_parse_reset();
  CanParams__Reset();

  /* Always create the two global flags */
  (void)CanParams__Create("pending_inbox", CANP_TYPE_BOOL);
  (void)CanParams__Create("pending_outbox", CANP_TYPE_BOOL);
  (void)CanParams_SetBool("pending_inbox", false);
  (void)CanParams_SetBool("pending_outbox", false);

  if (g_can_dbc_text == NULL)
  {
    return;
  }

  uint32_t current_msg_id = 0;
  char current_msg_name[64] = {0};

  const char* p = g_can_dbc_text;
  while (*p != '\0')
  {
    /* Grab a line */
    char line[256];
    size_t li = 0;
    while (*p != '\0' && *p != '\n' && li < (sizeof(line) - 1U))
    {
      line[li++] = *p++;
    }
    if (*p == '\n')
      p++;
    line[li] = '\0';

    if (strncmp(line, "BO_ ", 4) == 0)
    {
      /* BO_ <id> <name>: <dlc> */
      char* tok = strtok(line + 4, " ");
      if (tok == NULL)
        continue;

      uint32_t msg_id = 0;
      if (!parse_uint(tok, &msg_id))
        continue;

      tok = strtok(NULL, " :");
      if (tok == NULL)
        continue;

      (void)strncpy(current_msg_name, tok, sizeof(current_msg_name) - 1U);
      current_msg_name[sizeof(current_msg_name) - 1U] = '\0';
      current_msg_id = msg_id;

      tok = strtok(NULL, " ");
      if (tok == NULL)
        continue;

      uint32_t dlc = 0;
      if (!parse_uint(tok, &dlc))
        dlc = 8;

      if (s_msg_count < MAX_DBC_MSGS)
      {
        s_msgs[s_msg_count].msg_id = current_msg_id;
        s_msgs[s_msg_count].dlc = (uint8_t)dlc;
        (void)strncpy(s_msgs[s_msg_count].name, current_msg_name, sizeof(s_msgs[s_msg_count].name) - 1U);
        s_msgs[s_msg_count].name[sizeof(s_msgs[s_msg_count].name) - 1U] = '\0';
        s_msg_dirty[s_msg_count] = 0U;
        s_msg_count++;
      }
    }
    else if (strncmp(line, " SG_ ", 5) == 0 || strncmp(line, "SG_ ", 4) == 0)
    {
      /* Allow " SG_" or "SG_" */
      const char* sg = (line[0] == ' ') ? (line + 5) : (line + 4);

      /* SG_ <sig> : <start>|<len>@... (<factor>,<offset>) */
      char sig_name[64] = {0};

      /* copy sig name */
      const char* colon = strstr(sg, ":");
      if (colon == NULL)
        continue;

      size_t name_len = 0;
      const char* ns = sg;
      while (ns < colon && *ns != ' ' && name_len < (sizeof(sig_name) - 1U))
      {
        sig_name[name_len++] = *ns++;
      }
      sig_name[name_len] = '\0';
      if (sig_name[0] == '\0')
        continue;

      const char* after_colon = colon + 1;
      while (*after_colon == ' ')
        after_colon++;

      uint32_t start_bit = 0;
      uint32_t sig_len = 0;

      /* parse start|len */
      const char* bar = strstr(after_colon, "|");
      const char* at = strstr(after_colon, "@");
      if (bar == NULL || at == NULL || bar > at)
        continue;

      char tmp[16] = {0};
      size_t n = (size_t)(bar - after_colon);
      if (n >= sizeof(tmp))
        continue;
      (void)memcpy(tmp, after_colon, n);
      tmp[n] = '\0';
      if (!parse_uint(tmp, &start_bit))
        continue;

      n = (size_t)(at - (bar + 1));
      if (n >= sizeof(tmp))
        continue;
      (void)memcpy(tmp, bar + 1, n);
      tmp[n] = '\0';
      if (!parse_uint(tmp, &sig_len))
        continue;

      bool is_signed = false;
      if (at[1] == '-') /* crude: treat '-' as signed */
        is_signed = true;

      /* factor,offset */
      float factor = 1.0f;
      float offset = 0.0f;
      const char* lp = strstr(after_colon, "(");
      const char* rp = strstr(after_colon, ")");
      if (lp && rp && rp > lp)
      {
        char pair[64] = {0};
        size_t pn = (size_t)(rp - (lp + 1));
        if (pn < sizeof(pair))
        {
          (void)memcpy(pair, lp + 1, pn);
          pair[pn] = '\0';

          char* comma = strchr(pair, ',');
          if (comma)
          {
            *comma = '\0';
            (void)parse_float(pair, &factor);
            (void)parse_float(comma + 1, &offset);
          }
        }
      }

      canp_type_t type = CANP_TYPE_INT32;
      if (factor != 1.0f || offset != 0.0f)
      {
        type = CANP_TYPE_FLOAT;
      }
      else if (sig_len == 1U)
      {
        type = CANP_TYPE_BOOL;
      }
      else
      {
        type = CANP_TYPE_INT32;
      }

      if (s_sig_count < MAX_DBC_SIGS)
      {
        (void)build_full_name(s_sigs[s_sig_count].full_name,
                              sizeof(s_sigs[s_sig_count].full_name),
                              current_msg_name,
                              sig_name);

        s_sigs[s_sig_count].msg_id = current_msg_id;
        s_sigs[s_sig_count].start_bit = (uint8_t)start_bit;
        s_sigs[s_sig_count].length = (uint8_t)sig_len;
        s_sigs[s_sig_count].is_signed = is_signed;
        s_sigs[s_sig_count].factor = factor;
        s_sigs[s_sig_count].offset = offset;
        s_sigs[s_sig_count].type = type;

        (void)CanParams__Create(s_sigs[s_sig_count].full_name, type);

        s_sig_count++;
      }
    }
  }
}

/* =========================
 *  Bit packing helpers
 * ========================= */

static uint32_t extract_bits_le(const uint8_t* data, uint8_t start_bit, uint8_t length)
{
  uint32_t out = 0;
  for (uint8_t i = 0; i < length; i++)
  {
    uint16_t bit_index = (uint16_t)start_bit + i;
    uint16_t byte_index = bit_index / 8U;
    uint8_t bit_in_byte = (uint8_t)(bit_index % 8U);
    uint8_t bit = (data[byte_index] >> bit_in_byte) & 0x1U;
    out |= ((uint32_t)bit << i);
  }
  return out;
}

static int32_t sign_extend(uint32_t raw, uint8_t bitlen)
{
  if (bitlen == 0U || bitlen >= 32U)
    return (int32_t)raw;

  uint32_t sign_bit = 1UL << (bitlen - 1U);
  if (raw & sign_bit)
  {
    uint32_t mask = ~((1UL << bitlen) - 1U);
    raw |= mask;
  }
  return (int32_t)raw;
}

/* =========================
 *  RX decode path
 * ========================= */

static void handle_rx_frame(uint32_t std_id, const uint8_t* data, uint8_t dlc)
{
  (void)dlc;

  if (!rx_id_allowed(std_id))
  {
    return;
  }

  /* Decode all signals whose msg_id matches std_id */
  for (uint32_t i = 0; i < s_sig_count; i++)
  {
    if (s_sigs[i].msg_id != std_id)
      continue;

    const dbc_sig_t* s = &s_sigs[i];

    uint32_t raw = extract_bits_le(data, s->start_bit, s->length);

    bool updated_any = false;

    if (s->type == CANP_TYPE_BOOL)
    {
      if (CanParams__UpdateBool(s->full_name, (uint8_t)(raw != 0U)))
      {
        updated_any = true;
      }
    }
    else if (s->type == CANP_TYPE_FLOAT)
    {
      int32_t signed_raw = s->is_signed ? sign_extend(raw, s->length) : (int32_t)raw;
      float phys = ((float)signed_raw) * s->factor + s->offset;
      if (CanParams__UpdateFloat(s->full_name, phys))
      {
        updated_any = true;
      }
    }
    else
    {
      int32_t signed_raw = s->is_signed ? sign_extend(raw, s->length) : (int32_t)raw;
      if (CanParams__UpdateInt32(s->full_name, signed_raw))
      {
        updated_any = true;
      }
    }

    if (updated_any)
    {
      s_inbox_updated_since_tick = 1U;
    }
  }
}

/* HAL callback (invoked in IRQ context) */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
  if (hcan == NULL)
    return;

  CAN_RxHeaderTypeDef rx_header;
  uint8_t data[8];

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, data) != HAL_OK)
  {
    return;
  }

  if (rx_header.IDE != CAN_ID_STD)
  {
    return;
  }

  handle_rx_frame(rx_header.StdId, data, rx_header.DLC);
}

/* =========================
 *  TX encode path
 * ========================= */

static void insert_bits_le(uint8_t* data, uint8_t start_bit, uint8_t length, uint32_t value)
{
  for (uint8_t i = 0; i < length; i++)
  {
    uint16_t bit_index = (uint16_t)start_bit + i;
    uint16_t byte_index = bit_index / 8U;
    uint8_t bit_in_byte = (uint8_t)(bit_index % 8U);

    uint8_t bit = (uint8_t)((value >> i) & 0x1U);

    data[byte_index] &= (uint8_t)~(1U << bit_in_byte);
    data[byte_index] |= (uint8_t)(bit << bit_in_byte);
  }
}

static bool encode_raw_from_param(const dbc_sig_t* s, uint32_t* out_raw)
{
  if (s == NULL || out_raw == NULL)
    return false;

  if (s->type == CANP_TYPE_BOOL)
  {
    bool bv = false;
    if (!CanParams_GetBool(s->full_name, &bv))
      return false;
    *out_raw = bv ? 1U : 0U;
    return true;
  }
  else if (s->type == CANP_TYPE_FLOAT)
  {
    float fv = 0.0f;
    if (!CanParams_GetFloat(s->full_name, &fv))
      return false;

    /* invert scaling: raw = (phys - offset)/factor */
    if (s->factor == 0.0f)
      return false;

    float fr = (fv - s->offset) / s->factor;
    int32_t ir = (int32_t)fr;
    *out_raw = (uint32_t)ir;
    return true;
  }
  else
  {
    int32_t iv = 0;
    if (!CanParams_GetInt32(s->full_name, &iv))
      return false;

    *out_raw = (uint32_t)iv;
    return true;
  }
}

static void transmit_dirty_messages(void)
{
  for (uint32_t mi = 0; mi < s_msg_count; mi++)
  {
    if (s_msg_dirty[mi] == 0U)
      continue;

    uint8_t data[8] = {0};

    /* Encode all signals for this message from current parameters */
    for (uint32_t si = 0; si < s_sig_count; si++)
    {
      const dbc_sig_t* s = &s_sigs[si];
      if (s->msg_id != s_msgs[mi].msg_id)
        continue;

      uint32_t raw = 0;
      if (!encode_raw_from_param(s, &raw))
      {
        /* If parameter isn't valid yet, leave signal as 0 */
        raw = 0;
      }

      insert_bits_le(data, s->start_bit, s->length, raw);
    }

    CAN_TxHeaderTypeDef txh;
    txh.StdId = s_msgs[mi].msg_id;
    txh.ExtId = 0;
    txh.IDE = CAN_ID_STD;
    txh.RTR = CAN_RTR_DATA;
    txh.DLC = s_msgs[mi].dlc;
    txh.TransmitGlobalTime = DISABLE;

    uint32_t mbx = 0;
    (void)HAL_CAN_AddTxMessage(&hcan1, &txh, data, &mbx);

    s_msg_dirty[mi] = 0U;
  }
}

/* =========================
 *  Round-robin controller
 * ========================= */

void can_system_controller(void)
{
  static uint8_t s_inited = 0U;

  if (!s_inited)
  {
    dbc_parse_all();

    /* Start CAN interrupts */
    (void)HAL_CAN_Start(&hcan1);
    (void)HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    s_inited = 1U;
  }

  /* Update pending_inbox based on whether any RX updates happened since last tick */
  if (s_inbox_updated_since_tick != 0U)
  {
    (void)CanParams_SetBool("pending_inbox", true);
  }
  else
  {
    (void)CanParams_SetBool("pending_inbox", false);
  }
  s_inbox_updated_since_tick = 0U;

  /* If outbox pending, transmit dirty messages */
  bool pending = false;
  if (CanParams_GetBool("pending_outbox", &pending) && pending)
  {
    transmit_dirty_messages();
    (void)CanParams_SetBool("pending_outbox", false);
    s_outbox_pending = 0U;
  }
}

/* =========================
 *  External TX request API
 * ========================= */

bool CanSystem_SetBool(const char* full_name, bool value)
{
  if (full_name == NULL)
    return false;

  if (strcmp(full_name, "pending_inbox") == 0 || strcmp(full_name, "pending_outbox") == 0)
    return false;

  if (!mark_dirty_for_full_name(full_name))
    return false;

  if (!CanParams_SetBool(full_name, value))
    return false;

  s_outbox_pending = 1U;
  (void)CanParams_SetBool("pending_outbox", true);
  return true;
}

bool CanSystem_SetInt32(const char* full_name, int32_t value)
{
  if (full_name == NULL)
    return false;

  if (strcmp(full_name, "pending_inbox") == 0 || strcmp(full_name, "pending_outbox") == 0)
    return false;

  if (!mark_dirty_for_full_name(full_name))
    return false;

  if (!CanParams_SetInt32(full_name, value))
    return false;

  s_outbox_pending = 1U;
  (void)CanParams_SetBool("pending_outbox", true);
  return true;
}

bool CanSystem_SetFloat(const char* full_name, float value)
{
  if (full_name == NULL)
    return false;

  if (strcmp(full_name, "pending_inbox") == 0 || strcmp(full_name, "pending_outbox") == 0)
    return false;

  if (!mark_dirty_for_full_name(full_name))
    return false;

  if (!CanParams_SetFloat(full_name, value))
    return false;

  s_outbox_pending = 1U;
  (void)CanParams_SetBool("pending_outbox", true);
  return true;
}
