#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>

#include "usbd_configurator.h"

LOG_MODULE_REGISTER(usbd_config);

/* By default, do not register the USB DFU class DFU mode instance. */
static const char *const blocklist[] = {
	"dfu_dfu",
	NULL,
};

/* doc device instantiation start */
/*
 * Instantiate a context named usbd_ctx using the default USB device
 * controller, the Zephyr project vendor ID, and the sample product ID.
 * Zephyr project vendor ID must not be used outside of Zephyr samples.
 */
USBD_DEVICE_DEFINE(usbd_ctx,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_USBD_VID, CONFIG_USBD_PID);
/* doc device instantiation end */

/* doc string instantiation start */
USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_MANUFACTURER_DEFINE(manufecturer, CONFIG_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(product_d, CONFIG_USBD_PRODUCT);
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(sample_sn)));

/* doc string instantiation end */

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

/* doc configuration instantiation start */
static const uint8_t attributes = (IS_ENABLED(CONFIG_USBD_SELF_POWERED) ?
				   USB_SCD_SELF_POWERED : 0) |
				  (IS_ENABLED(CONFIG_USBD_REMOTE_WAKEUP) ?
				   USB_SCD_REMOTE_WAKEUP : 0);

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(sample_fs_config,
			  attributes,
			  CONFIG_USBD_MAX_POWER, &fs_cfg_desc);

/* High speed configuration */
USBD_CONFIGURATION_DEFINE(sample_hs_config,
			  attributes,
			  CONFIG_USBD_MAX_POWER, &hs_cfg_desc);
/* doc configuration instantiation end */

static void test_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		uint32_t dtr = 0U;

		uart_line_ctrl_get(msg->dev, UART_LINE_CTRL_DTR, &dtr);
		LOG_DBG("Control line state: dtr=%u", dtr);
	}
}

static void sample_fix_code_triple(struct usbd_context *usbd_ctx,
				   const enum usbd_speed speed)
{
	/* Always use class code information from Interface Descriptors */
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_VIDEO_CLASS)) {
		/*
		 * Class with multiple interfaces have an Interface
		 * Association Descriptor available, use an appropriate triple
		 * to indicate it.
		 */
		usbd_device_set_code_triple(usbd_ctx, speed,
					    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	} else {
		usbd_device_set_code_triple(usbd_ctx, speed, 0, 0, 0);
	}
}

static struct usbd_context *usbd_setup_device(usbd_msg_cb_t msg_cb)
{
	int err;

	/* doc add string descriptor start */
	err = usbd_add_descriptor(&usbd_ctx, &sample_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (errno=%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&usbd_ctx, &manufecturer);
	if (err) {
		LOG_ERR("Failed to initialize manufacturer descriptor (errno=%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&usbd_ctx, &product_d);
	if (err) {
		LOG_ERR("Failed to initialize product descriptor (errno=%d)", err);
		return NULL;
	}

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(&usbd_ctx, &sample_sn);
	))
	if (err) {
		LOG_ERR("Failed to initialize SN descriptor (errno=%d)", err);
		return NULL;
	}
	/* doc add string descriptor end */

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&usbd_ctx) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&usbd_ctx, USBD_SPEED_HS,
					     &sample_hs_config);
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return NULL;
		}

		err = usbd_register_all_classes(&usbd_ctx, USBD_SPEED_HS, 1,
						blocklist);
		if (err) {
			LOG_ERR("Failed to add register classes");
			return NULL;
		}

		sample_fix_code_triple(&usbd_ctx, USBD_SPEED_HS);
	}

	/* doc configuration register start */
	err = usbd_add_configuration(&usbd_ctx, USBD_SPEED_FS,
				     &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}
	/* doc configuration register end */

	/* doc functions register start */
	err = usbd_register_all_classes(&usbd_ctx, USBD_SPEED_FS, 1, blocklist);
	if (err) {
		LOG_ERR("Failed to add register classes");
		return NULL;
	}
	/* doc functions register end */

	sample_fix_code_triple(&usbd_ctx, USBD_SPEED_FS);
	usbd_self_powered(&usbd_ctx, attributes & USB_SCD_SELF_POWERED);

	if (msg_cb != NULL) {
		/* doc device init-and-msg start */
		err = usbd_msg_register_cb(&usbd_ctx, msg_cb);
		if (err) {
			LOG_ERR("Failed to register message callback");
			return NULL;
		}
		/* doc device init-and-msg end */
	}

#if CONFIG_usbd_ctx_20_EXTENSION_DESC
	(void)usbd_device_set_bcd_usb(&usbd_ctx, USBD_SPEED_FS, 0x0201);
	(void)usbd_device_set_bcd_usb(&usbd_ctx, USBD_SPEED_HS, 0x0201);

	err = usbd_add_descriptor(&usbd_ctx, &sample_usbext);
	if (err) {
		LOG_ERR("Failed to add USB 2.0 Extension Descriptor");
		return NULL;
	}
#endif

	return &usbd_ctx;
}

static struct usbd_context *usbd_init_device(usbd_msg_cb_t msg_cb)
{
	int err;

	if (usbd_setup_device(msg_cb) == NULL) {
		return NULL;
	}

	/* doc device init start */
	err = usbd_init(&usbd_ctx);
	if (err) {
		LOG_ERR("Failed to initialize device support (errno=%d)", err);
		return NULL;
	}

	return &usbd_ctx;
}

int init_cdc_acm(struct usbd_context *usbd_ctx) {
	int err;

	usbd_ctx = usbd_init_device(test_msg_cb);
	if (usbd_ctx == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(usbd_ctx)) {
		err = usbd_enable(usbd_ctx);
		if (err) {
			LOG_ERR("Failed to enable device support (errno=%d)", err);
			return err;
		}
	}

	LOG_INF("USB device support enabled");

	return 0;
}