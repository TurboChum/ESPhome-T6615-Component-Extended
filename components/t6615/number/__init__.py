import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
    UNIT_PARTS_PER_MILLION,
)

from .. import CONF_T6615_ID, T6615Component, t6615_ns

DEPENDENCIES = ["t6615"]

ElevationNumber = t6615_ns.class_("ElevationNumber", number.Number)
CalibrationPpmNumber = t6615_ns.class_("CalibrationPpmNumber", number.Number)

CONF_ELEVATION = "elevation"
CONF_CAL_PPM_TARGET = "calibration_ppm_target"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_T6615_ID): cv.use_id(T6615Component),
        cv.Optional(CONF_ELEVATION): number.number_schema(
            ElevationNumber,
            unit_of_measurement="ft",
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:elevation-rise",
        ),
        cv.Optional(CONF_CAL_PPM_TARGET): number.number_schema(
            CalibrationPpmNumber,
            unit_of_measurement=UNIT_PARTS_PER_MILLION,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:molecule-co2",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_T6615_ID])

    if elev_config := config.get(CONF_ELEVATION):
        n = await number.new_number(
            elev_config, min_value=0.0, max_value=5000.0, step=1.0
        )
        await cg.register_parented(n, config[CONF_T6615_ID])
        cg.add(parent.set_elevation_number(n))

    if cal_config := config.get(CONF_CAL_PPM_TARGET):
        n = await number.new_number(
            cal_config, min_value=400.0, max_value=2000.0, step=1.0
        )
        await cg.register_parented(n, config[CONF_T6615_ID])
        cg.add(parent.set_cal_ppm_number(n))
