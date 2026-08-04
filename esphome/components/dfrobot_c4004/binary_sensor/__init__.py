import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv

from .. import CONF_C4004_ID, C4004Component, disable_entity_mqtt_schema, without_entity_mqtt

CONF_ONLINE = "online"
CONF_PRESENCE = "presence"
CONF_ZONE_1_PRESENCE = "zone_1_presence"
CONF_ZONE_2_PRESENCE = "zone_2_presence"
CONF_ZONE_3_PRESENCE = "zone_3_presence"
CONF_ZONE_4_PRESENCE = "zone_4_presence"
CONF_ZONE_5_PRESENCE = "zone_5_presence"
CONF_ZONE_6_PRESENCE = "zone_6_presence"

ZONE_PRESENCE_KEYS = (
    CONF_ZONE_1_PRESENCE,
    CONF_ZONE_2_PRESENCE,
    CONF_ZONE_3_PRESENCE,
    CONF_ZONE_4_PRESENCE,
    CONF_ZONE_5_PRESENCE,
    CONF_ZONE_6_PRESENCE,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_C4004_ID): cv.use_id(C4004Component),
        cv.Optional(CONF_ONLINE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="connectivity",
                icon="mdi:radar",
            )
        ),
        cv.Optional(CONF_PRESENCE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="occupancy",
                icon="mdi:human-handsup",
            )
        ),
        cv.Optional(CONF_ZONE_1_PRESENCE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="occupancy",
                icon="mdi:home-account",
            )
        ),
        cv.Optional(CONF_ZONE_2_PRESENCE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="occupancy",
                icon="mdi:numeric-2-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_3_PRESENCE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="occupancy",
                icon="mdi:numeric-3-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_4_PRESENCE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="occupancy",
                icon="mdi:numeric-4-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_5_PRESENCE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="occupancy",
                icon="mdi:numeric-5-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_6_PRESENCE): disable_entity_mqtt_schema(
            binary_sensor.binary_sensor_schema(
                device_class="occupancy",
                icon="mdi:numeric-6-circle-outline",
            )
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_C4004_ID])

    if online_config := config.get(CONF_ONLINE):
        sens = await binary_sensor.new_binary_sensor(without_entity_mqtt(online_config))
        cg.add(parent.set_online_binary_sensor(sens))

    if presence_config := config.get(CONF_PRESENCE):
        sens = await binary_sensor.new_binary_sensor(without_entity_mqtt(presence_config))
        cg.add(parent.set_presence_binary_sensor(sens))

    for index, key in enumerate(ZONE_PRESENCE_KEYS):
        if zone_config := config.get(key):
            sens = await binary_sensor.new_binary_sensor(without_entity_mqtt(zone_config))
            cg.add(parent.set_zone_presence_binary_sensor(index, sens))
