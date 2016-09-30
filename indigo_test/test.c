//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "indigo_bus.h"
#include "ccd_simulator.h"

static indigo_property *connection_property;
static indigo_property *exposure_property;
static indigo_property *ccd1_property;

static indigo_result test_attach(indigo_client *client) {
  connection_property = indigo_init_switch_property(NULL, "CCD Simulator", "CONNECTION", "", "", 0, 0, 0, 2);
  indigo_init_switch_item(&connection_property->items[0], "CONNECTED", "", false);
  indigo_init_switch_item(&connection_property->items[1], "DISCONNECTED", "", true);
  exposure_property = indigo_init_number_property(NULL, "CCD Simulator", "CCD_EXPOSURE", "", "", 0, 0, 1);
  indigo_init_number_item(&exposure_property->items[0], "CCD_EXPOSURE_VALUE", "", 0, 0, 0, 0);
  ccd1_property = indigo_init_blob_property(NULL, "CCD Simulator", "CCD_IMAGE", "", "", 0, 1);
  indigo_init_blob_item(&ccd1_property->items[0], "CCD_IMAGE", "");
  indigo_log("connected to INDI bus...");
  indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
  return INDIGO_OK;
}

static indigo_result test_define_property(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property, const char *message) {
  if (message)
    indigo_log("message: %s", message);
  if (indigo_property_match(connection_property, property)) {
    connection_property->items[0].switch_value = true;
    connection_property->items[1].switch_value = false;
    indigo_change_property(client, connection_property);
    return INDIGO_OK;
  }
  return INDIGO_OK;
}

static indigo_result test_update_property(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property, const char *message) {
  if (message)
    indigo_log("message: %s", message);
  if (indigo_property_match(connection_property, property)) {
    indigo_property_copy_values(connection_property, property, true);
    if (connection_property->items[0].switch_value) {
      indigo_log("connected...");
      exposure_property->items[0].number_value = 3.0;
      indigo_change_property(client, exposure_property);
    } else {
      indigo_log("disconnected...");
      indigo_stop();
    }
    return INDIGO_OK;
  }
  if (indigo_property_match(exposure_property, property)) {
    indigo_property_copy_values(exposure_property, property, true);
    if (exposure_property->state == INDIGO_BUSY_STATE) {
      indigo_log("exposure %gs...", exposure_property->items[0].number_value);
    } else if (exposure_property->state == INDIGO_OK_STATE) {
      indigo_log("exposure done...");
    }
    return INDIGO_OK;
  }
  if (indigo_property_match(ccd1_property, property)) {
    indigo_property_copy_values(ccd1_property, property, true);
    if (ccd1_property->state == INDIGO_OK_STATE) {
      indigo_log("image received (%d bytes)...", ccd1_property->items[0].blob_size);
      connection_property->items[0].switch_value = false;
      connection_property->items[1].switch_value = true;
      indigo_change_property(client, connection_property);
    }
    return INDIGO_OK;
  }
  return INDIGO_OK;
}

static indigo_result test_send_message(struct indigo_client *client, struct indigo_driver *driver, const char *message) {
  indigo_log("message: %s", message);
  return INDIGO_OK;
}

static indigo_result test_detach(indigo_client *client) {
  indigo_log("disconnected from INDI bus...");
  exit(0);
  return INDIGO_OK;
}

static indigo_client test = {
  NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
  test_attach,
  test_define_property,
  test_update_property,
  NULL,
  test_send_message,
  test_detach
};

int main(int argc, const char * argv[]) {
  indigo_main_argc = argc;
  indigo_main_argv = argv;
  indigo_start();
  indigo_connect_driver(ccd_simulator());
  indigo_connect_client(&test);
  sleep(10);
  indigo_stop();
  return 0;
}
