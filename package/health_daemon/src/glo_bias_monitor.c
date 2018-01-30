/*
 * Copyright (C) 2018 Swift Navigation Inc.
 * Contact: Swift Navigation <dev@swiftnav.com>
 *
 * This source is subject to the license found in the file 'LICENSE' which must
 * be be distributed together with this source. All other rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdlib.h>
#include <string.h>

#include <libpiksi/logging.h>
#include <libpiksi/sbp_zmq_pubsub.h>

#include <libsbp/sbp.h>
#include <libsbp/observation.h>

#include "health_monitor.h"
#include "utils.h"

#include "glo_health_context.h"
#include "glo_bias_monitor.h"

/* these are from fw private, consider moving to libpiski */
#define MSG_FORWARD_SENDER_ID (0u)

#define GLO_BIAS_ALERT_RATE_LIMIT (240000) /* ms */

static health_monitor_t *glo_bias_monitor;

static int glo_bias_timer_callback(health_monitor_t *monitor, void *context)
{
  (void)monitor;
  (void)context;
  if (glo_context_is_glonass_enabled() && glo_context_is_connected_to_base()) {
    sbp_log(
      LOG_WARNING,
      "Reference Glonass Biases Msg Timeout - no biases received from base station within %d sec window",
      GLO_BIAS_ALERT_RATE_LIMIT / 1000);
  }

  return 0;
}

static int sbp_msg_glo_biases_callback(health_monitor_t *monitor,
                                       u16 sender_id,
                                       u8 len,
                                       u8 msg_[],
                                       void *ctx)
{
  (void)monitor;
  (void)len;
  (void)msg_;
  (void)ctx;

  if (sender_id == MSG_FORWARD_SENDER_ID) {
    return 0;
  }
  return 1; // only reset if glo biases message is from base
}


int glo_bias_timeout_health_monitor_init(health_ctx_t *health_ctx)
{
  glo_bias_monitor = health_monitor_create();
  if (glo_bias_monitor == NULL) {
    return -1;
  }

  if (health_monitor_init(glo_bias_monitor,
                          health_ctx,
                          SBP_MSG_GLO_BIASES,
                          sbp_msg_glo_biases_callback,
                          GLO_BIAS_ALERT_RATE_LIMIT,
                          glo_bias_timer_callback,
                          NULL)
      != 0) {
    return -1;
  }

  return 0;
}

void glo_bias_timeout_health_monitor_deinit(void)
{
  if (glo_bias_monitor != NULL) {
    health_monitor_destroy(&glo_bias_monitor);
  }
}
