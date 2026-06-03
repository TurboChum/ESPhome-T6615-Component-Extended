import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_SWITCH,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from .. import CONF_T6615_ID, T6615Component, t6615_ns

IdleModeSwitch        = t6615_ns.class_("IdleModeSwitch",        switch.Switch)
CalibrationArmedSwitch = t6615_ns.class_("CalibrationArmedSwitch", switch.Switch)
AbcSwitch             = t6615_ns.class_("AbcSwitch",             switch.Switch)

CONF_IDLE_MODE          = "idle_mode"
CONF_CALIBRATION_ARMED  = "calibration_armed"
CONF_ABC_LOGIC          = "abc_logic"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_T6615_ID): cv.use_id(T6615Component),
        cv.Optional(CONF_IDLE_MODE): switch.switch_schema(
            IdleModeSwitch,
            device_class=DEVICE_CLASS_SWITCH,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:sleep",
        ),
        cv.Optional(CONF_CALIBRATION_ARMED): switch.switch_schema(
            CalibrationArmedSwitch,
            device_class=DEVICE_CLASS_SWITCH,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:shield-key",
        ),
        cv.Optional(CONF_ABC_LOGIC): switch.switch_schema(
            AbcSwitch,
            device_class=DEVICE_CLASS_SWITCH,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:refresh-auto",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_T6615_ID])

    if idle_config := config.get(CONF_IDLE_MODE):
        s = await switch.new_switch(idle_config)
        await cg.register_parented(s, config[CONF_T6615_ID])

    if cal_armed_config := config.get(CONF_CALIBRATION_ARMED):
        s = await switch.new_switch(cal_armed_config)
        await cg.register_parented(s, config[CONF_T6615_ID])
        cg.add(parent.set_cal_armed_switch(s))

    if abc_config := config.get(CONF_ABC_LOGIC):
        s = await switch.new_switch(abc_config)
        await cg.register_parented(s, config[CONF_T6615_ID])
        cg.add(parent.set_abc_switch(s))
