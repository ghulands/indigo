// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>


/** INDIGO ZWO ASI CCD driver
 \file indigo_ccd_asi.c
 */

#define DRIVER_VERSION 0x0001

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "indigo_ccd_asi.h"
#include "indigo_driver_xml.h"

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((asi_private_data *)DEVICE_CONTEXT->private_data)

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c

// -------------------------------------------------------------------------------- ZWO ASI USB interface implementation



typedef struct {
	libusb_device *dev;
	libusb_device_handle *handle;
	int device_count;
	indigo_timer *exposure_timer, *temperture_timer, *guider_timer;
	double target_temperature, current_temperature;
	unsigned short relay_mask;
	unsigned char *buffer;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
	double exposure;
	// ...etc, local variables for camera instance
} asi_private_data;

static bool asi_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->device_count++ == 0) {
		libusb_device *dev = PRIVATE_DATA->dev;

		// open the camera, hot-plug should be handled somehow with ASI

	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	// start exposure

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_read_pixels(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

		// download image to PRIVATE_DATA->buffer + FITS_HEADER_SIZE

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	// abort exposure

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

static bool asi_set_cooler(indigo_device *device, bool status, double target, double *current) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	// set cooler

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}

//static bool asi_guide(indigo_device *device, ...) {
// //...
//}

static void asi_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (--PRIVATE_DATA->device_count == 0) {

		// close

		free(PRIVATE_DATA->buffer);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

// callback for image download
static void exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (asi_read_pixels(device)) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure done");
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), PRIVATE_DATA->exposure, true);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}

// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		// anything else to do...
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (PRIVATE_DATA->can_check_temperature) {
		if (asi_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	PRIVATE_DATA->temperture_timer = indigo_set_timer(device, 5, ccd_temperature_callback);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	asi_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;

		// adjust info know before opening camera...
		// make hidden properties visible if needed
		// adjust read-write/read-only permission for properties if needed
		// adjust min/max values for properties if needed

		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (asi_open(device)) {

				// adjust info known after opening camera...
				// make hidden properties visible if needed
				// adjust read-write/read-only permission for properties if needed
				// adjust min/max values for properties if needed

//       if (has cooler) {
//         CCD_COOLER_PROPERTY->hidden = false;
//         CCD_TEMPERATURE_PROPERTY->hidden = false;
//         PRIVATE_DATA->target_temperature = 0;
//         ccd_temperature_callback(device);
//       }
				PRIVATE_DATA->can_check_temperature = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (PRIVATE_DATA->temperture_timer != NULL)
				indigo_cancel_timer(device, PRIVATE_DATA->temperture_timer);
			asi_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		PRIVATE_DATA->exposure = CCD_EXPOSURE_ITEM->number.value;
		asi_start_exposure(device, PRIVATE_DATA->exposure, CCD_FRAME_TYPE_DARK_ITEM->sw.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure initiated");
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->exposure > 4)
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, PRIVATE_DATA->exposure - 4, clear_reg_timer_callback);
		else {
			PRIVATE_DATA->can_check_temperature = false;
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, PRIVATE_DATA->exposure, exposure_timer_callback);
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			asi_abort_exposure(device);
			indigo_cancel_timer(device, PRIVATE_DATA->exposure_timer);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			if (CCD_COOLER_OFF_ITEM->sw.value) {
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
				CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	asi_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (asi_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			asi_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			// guide north
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				// guide north
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			// guide east
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				// guide west
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define ASI_VENDOR_ID                  0xXXXX

#define MAX_DEVICES                   10

static struct {
	int product;
	const char *name;
	indigo_device_interface iface;
} ASI_PRODUCTS[] = {
	{ 0, NULL }
};

static indigo_device *devices[MAX_DEVICES];

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device ccd_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	static indigo_device guider_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};
	struct libusb_device_descriptor descriptor;
	switch (event) {
	case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
		int rc = libusb_get_device_descriptor(dev, &descriptor);
		INDIGO_DEBUG_DRIVER(indigo_debug("asi_hotplug_callback: libusb_get_device_descriptor [%d] ->  %s", __LINE__, libusb_error_name(rc)));
		for (int i = 0; ASI_PRODUCTS[i].name; i++) {
			if (descriptor.idVendor == 0x1278 && ASI_PRODUCTS[i].product == descriptor.idProduct) {
				asi_private_data *private_data = malloc(sizeof(asi_private_data));
				assert(private_data != NULL);
				memset(private_data, 0, sizeof(asi_private_data));
				libusb_ref_device(dev);
				private_data->dev = dev;
				indigo_device *device = malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &ccd_template, sizeof(indigo_device));
				strcpy(device->name, ASI_PRODUCTS[i].name);
				device->device_context = private_data;
				for (int j = 0; j < MAX_DEVICES; j++) {
					if (devices[j] == NULL) {
						indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
						break;
					}
				}
				device = malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &guider_template, sizeof(indigo_device));
				strcpy(device->name, ASI_PRODUCTS[i].name);
				strcat(device->name, " guider");
				device->device_context = private_data;
				for (int j = 0; j < MAX_DEVICES; j++) {
					if (devices[j] == NULL) {
						indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
						break;
					}
				}
				break;
			}
		}
		break;
	}
	case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
		asi_private_data *private_data = NULL;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				if (PRIVATE_DATA->dev == dev) {
					private_data = PRIVATE_DATA;
					indigo_detach_device(device);
					free(device);
					devices[j] = NULL;
				}
			}
		}
		if (private_data != NULL) {
			libusb_unref_device(dev);
			free(private_data);
		}
		break;
	}
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_asi(bool state) {
	static bool current_state = false;
	if (state == current_state)
		return INDIGO_OK;
	if ((current_state = state)) {
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, 0x1278, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, NULL);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_asi: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, libusb_error_name(rc)));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
	} else {
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_asi: libusb_hotplug_deregister_callback [%d]", __LINE__));
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		return INDIGO_OK;
	}
}


