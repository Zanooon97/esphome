import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_DISCOVERY, CONF_ID, CONF_LOG_TOPIC, CONF_MQTT, CONF_MQTT_ID, CONF_QOS, CONF_TOPIC_PREFIX
import esphome.final_validate as fv

DEPENDENCIES = ["uart"]
MULTI_CONF = True
CODEOWNERS = ["@jiaziui"]


def disable_entity_mqtt_schema(schema):
    """保留 schema 包装；实体 MQTT 镜像在 to_code 中通过 without_entity_mqtt 剥离。"""
    return schema


def without_entity_mqtt(config):
    """移除 mqtt 组件加载时 OnlyWith 自动注入的 mqtt_id，避免生成 mqtt.* 包装。"""
    if CONF_MQTT_ID not in config:
        return config
    stripped = dict(config)
    stripped.pop(CONF_MQTT_ID, None)
    return stripped

dfrobot_c4004_ns = cg.esphome_ns.namespace("dfrobot_c4004")
C4004Component = dfrobot_c4004_ns.class_(
    "C4004Component", cg.Component, uart.UARTDevice
)

CONF_C4004_ID = "c4004_id"
CONF_ZONE_GPIO_PINS = "zone_gpio_pins"
CONF_MQTT_BRIDGE = "mqtt_bridge"
CONF_MQTT_KEY = "mqtt_key"
CONF_RETAIN_STATE = "retain_state"
CONF_STREAM_QOS = "stream_qos"
CONF_DISCOVER_IP = "discover_ip"

C4004_ZONE_GPIO_COUNT = 6


def _validate_mqtt_bridge(config):
    if CONF_MQTT_BRIDGE in config:
        cv.requires_component("mqtt")(config)
    return config


def _final_validate_mqtt_transport_only(config):
    if CONF_MQTT_BRIDGE not in config:
        return config

    full_config = fv.full_config.get()
    mqtt_config = full_config.get(CONF_MQTT)
    if mqtt_config is None:
        raise cv.Invalid("dfrobot_c4004 mqtt_bridge requires the root mqtt component to be configured")

    topic_prefix = mqtt_config.get(CONF_TOPIC_PREFIX)
    if topic_prefix not in (None, ""):
        raise cv.Invalid(
            "dfrobot_c4004 uses MQTT as an interface-only bridge; set mqtt.topic_prefix: null to disable MQTT entity topics"
        )

    if mqtt_config.get(CONF_DISCOVERY, False):
        raise cv.Invalid(
            "dfrobot_c4004 uses MQTT as an interface-only bridge; set mqtt.discovery: false to disable Home Assistant MQTT entity discovery"
        )

    if mqtt_config.get(CONF_DISCOVER_IP, False):
        raise cv.Invalid(
            "dfrobot_c4004 uses MQTT as an interface-only bridge; set mqtt.discover_ip: false"
        )

    if mqtt_config.get(CONF_LOG_TOPIC) not in (None, {}):
        raise cv.Invalid(
            "dfrobot_c4004 reserves MQTT for bridge traffic; set mqtt.log_topic: ~ to avoid log-topic side effects"
        )

    return config

CONFIG_SCHEMA = (
    cv.All(
        cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(C4004Component),
                cv.Optional(CONF_ZONE_GPIO_PINS, default=[-1] * C4004_ZONE_GPIO_COUNT): cv.All(
                    cv.ensure_list(cv.int_range(min=-1, max=255)),
                    cv.Length(min=C4004_ZONE_GPIO_COUNT, max=C4004_ZONE_GPIO_COUNT),
                ),
                cv.Optional(CONF_MQTT_BRIDGE): cv.Schema(
                    {
                        cv.Required(CONF_TOPIC_PREFIX): cv.string_strict,
                        cv.Required(CONF_MQTT_KEY): cv.string_strict,
                        cv.Optional(CONF_QOS, default=1): cv.int_range(min=0, max=2),
                        cv.Optional(CONF_STREAM_QOS, default=0): cv.int_range(min=0, max=2),
                        cv.Optional(CONF_RETAIN_STATE, default=True): cv.boolean,
                    }
                ),
            }
        )
        .extend(uart.UART_DEVICE_SCHEMA)
        .extend(cv.COMPONENT_SCHEMA),
        _validate_mqtt_bridge,
    )
)

FINAL_VALIDATE_SCHEMA = _final_validate_mqtt_transport_only


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    for index, pin in enumerate(config[CONF_ZONE_GPIO_PINS]):
        cg.add(var.set_zone_gpio_pin(index, pin))

    if mqtt_bridge := config.get(CONF_MQTT_BRIDGE):
        cg.add(
            var.set_mqtt_bridge(
                mqtt_bridge[CONF_MQTT_KEY],
                mqtt_bridge[CONF_TOPIC_PREFIX],
                mqtt_bridge[CONF_QOS],
                mqtt_bridge[CONF_STREAM_QOS],
                mqtt_bridge[CONF_RETAIN_STATE],
            )
        )
