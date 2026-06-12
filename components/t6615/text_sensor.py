import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_CHIP,
)

from . import CONF_T6615_ID, T6615Component

DEPENDENCIES = ["t6615"]

CONF_SERIAL_NUMBER = "serial_number"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_FIRMWARE_DATE = "firmware_date"
CONF_SELFTEST_RESULT = "selftest_result"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_T6615_ID): cv.use_id(T6615Component),
        cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_CHIP,
        ),
        cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_CHIP,
        ),
        cv.Optional(CONF_FIRMWARE_DATE): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_CHIP,
        ),
        cv.Optional(CONF_SELFTEST_RESULT): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:microscope",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_T6615_ID])

    if serial_config := config.get(CONF_SERIAL_NUMBER):
        sens = await text_sensor.new_text_sensor(serial_config)
        cg.add(parent.set_serial_number_text_sensor(sens))

    if fw_ver_config := config.get(CONF_FIRMWARE_VERSION):
        sens = await text_sensor.new_text_sensor(fw_ver_config)
        cg.add(parent.set_firmware_version_text_sensor(sens))

    if fw_date_config := config.get(CONF_FIRMWARE_DATE):
        sens = await text_sensor.new_text_sensor(fw_date_config)
        cg.add(parent.set_firmware_date_text_sensor(sens))

    if selftest_config := config.get(CONF_SELFTEST_RESULT):
        sens = await text_sensor.new_text_sensor(selftest_config)
        cg.add(parent.set_selftest_result_text_sensor(sens))
