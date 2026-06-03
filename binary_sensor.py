import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import CONF_T6615_ID, T6615Component

DEPENDENCIES = ["t6615"]

CONF_ERROR_FLAG        = "error_flag"
CONF_WARMUP_FLAG       = "warmup_flag"
CONF_CALIBRATING_FLAG  = "calibrating_flag"
CONF_SELFTEST_RUNNING  = "selftest_running"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_T6615_ID): cv.use_id(T6615Component),
        cv.Optional(CONF_ERROR_FLAG): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_WARMUP_FLAG): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:timer-sand",
        ),
        cv.Optional(CONF_CALIBRATING_FLAG): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:tune",
        ),
        cv.Optional(CONF_SELFTEST_RUNNING): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:microscope",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_T6615_ID])

    if error_config := config.get(CONF_ERROR_FLAG):
        sens = await binary_sensor.new_binary_sensor(error_config)
        cg.add(parent.set_error_binary_sensor(sens))

    if warmup_config := config.get(CONF_WARMUP_FLAG):
        sens = await binary_sensor.new_binary_sensor(warmup_config)
        cg.add(parent.set_warmup_binary_sensor(sens))

    if cal_config := config.get(CONF_CALIBRATING_FLAG):
        sens = await binary_sensor.new_binary_sensor(cal_config)
        cg.add(parent.set_calibrating_binary_sensor(sens))

    if selftest_config := config.get(CONF_SELFTEST_RUNNING):
        sens = await binary_sensor.new_binary_sensor(selftest_config)
        cg.add(parent.set_selftest_running_binary_sensor(sens))
