import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_SWITCH,
    ENTITY_CATEGORY_CONFIG,
)

from .. import CONF_T6615_ID, T6615Component, t6615_ns

IdleModeSwitch = t6615_ns.class_("IdleModeSwitch", switch.Switch)

CONF_IDLE_MODE = "idle_mode"

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
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_T6615_ID])

    if idle_config := config.get(CONF_IDLE_MODE):
        s = await switch.new_switch(idle_config)
        await cg.register_parented(s, config[CONF_T6615_ID])
