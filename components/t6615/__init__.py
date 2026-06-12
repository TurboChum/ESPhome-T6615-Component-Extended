import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@TurboChum"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

t6615_ns = cg.esphome_ns.namespace("t6615")
T6615Component = t6615_ns.class_("T6615Component", cg.PollingComponent, uart.UARTDevice)

CONF_T6615_ID = "t6615_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(T6615Component),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "t6615", baud_rate=19200, require_rx=True, require_tx=True
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
