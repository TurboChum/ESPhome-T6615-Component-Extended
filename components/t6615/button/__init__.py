import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_RESTART,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_RESTART,
)

from .. import CONF_T6615_ID, T6615Component, t6615_ns

DEPENDENCIES = ["t6615"]

WarmResetButton = t6615_ns.class_("WarmResetButton", button.Button)
CalibrateButton = t6615_ns.class_("CalibrateButton", button.Button)
SelfTestButton = t6615_ns.class_("SelfTestButton", button.Button)

CONF_WARM_RESET = "warm_reset"
CONF_TRIGGER_CALIBRATION = "trigger_calibration"
CONF_SELF_TEST = "self_test"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_T6615_ID): cv.use_id(T6615Component),
        cv.Optional(CONF_WARM_RESET): button.button_schema(
            WarmResetButton,
            device_class=DEVICE_CLASS_RESTART,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_RESTART,
        ),
        cv.Optional(CONF_TRIGGER_CALIBRATION): button.button_schema(
            CalibrateButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:tune",
        ),
        cv.Optional(CONF_SELF_TEST): button.button_schema(
            SelfTestButton,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:microscope",
        ),
    }
)


async def to_code(config):
    if reset_config := config.get(CONF_WARM_RESET):
        b = await button.new_button(reset_config)
        await cg.register_parented(b, config[CONF_T6615_ID])

    if cal_config := config.get(CONF_TRIGGER_CALIBRATION):
        b = await button.new_button(cal_config)
        await cg.register_parented(b, config[CONF_T6615_ID])

    if selftest_config := config.get(CONF_SELF_TEST):
        b = await button.new_button(selftest_config)
        await cg.register_parented(b, config[CONF_T6615_ID])
