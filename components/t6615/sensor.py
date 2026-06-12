import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CO2,
    DEVICE_CLASS_CARBON_DIOXIDE,
    STATE_CLASS_MEASUREMENT,
    UNIT_PARTS_PER_MILLION,
)

from . import CONF_T6615_ID, T6615Component

DEPENDENCIES = ["t6615"]

CONF_ELEVATION_READING = "elevation_reading"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_T6615_ID): cv.use_id(T6615Component),
        cv.Optional(CONF_CO2): sensor.sensor_schema(
            unit_of_measurement=UNIT_PARTS_PER_MILLION,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_CARBON_DIOXIDE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_ELEVATION_READING): sensor.sensor_schema(
            unit_of_measurement="ft",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:elevation-rise",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_T6615_ID])

    if co2_config := config.get(CONF_CO2):
        sens = await sensor.new_sensor(co2_config)
        cg.add(parent.set_co2_sensor(sens))

    if elev_config := config.get(CONF_ELEVATION_READING):
        sens = await sensor.new_sensor(elev_config)
        cg.add(parent.set_elevation_sensor(sens))
