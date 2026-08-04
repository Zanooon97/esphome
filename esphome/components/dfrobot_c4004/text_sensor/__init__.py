import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv

from .. import CONF_C4004_ID, C4004Component, disable_entity_mqtt_schema, without_entity_mqtt

CONF_DETECTION_RANGE_MODE = "detection_range_mode"
CONF_ZONE_1_BOUNDARY_STATE = "zone_1_boundary_state"
CONF_ZONE_2_BOUNDARY_STATE = "zone_2_boundary_state"
CONF_ZONE_3_BOUNDARY_STATE = "zone_3_boundary_state"
CONF_ZONE_4_BOUNDARY_STATE = "zone_4_boundary_state"
CONF_ZONE_5_BOUNDARY_STATE = "zone_5_boundary_state"
CONF_ZONE_1_APPROACH_AWAY_STATE = "zone_1_approach_away_state"
CONF_ZONE_2_APPROACH_AWAY_STATE = "zone_2_approach_away_state"
CONF_ZONE_3_APPROACH_AWAY_STATE = "zone_3_approach_away_state"
CONF_ZONE_4_APPROACH_AWAY_STATE = "zone_4_approach_away_state"
CONF_ZONE_5_APPROACH_AWAY_STATE = "zone_5_approach_away_state"

ZONE_BOUNDARY_STATE_KEYS = (
    CONF_ZONE_1_BOUNDARY_STATE,
    CONF_ZONE_2_BOUNDARY_STATE,
    CONF_ZONE_3_BOUNDARY_STATE,
    CONF_ZONE_4_BOUNDARY_STATE,
    CONF_ZONE_5_BOUNDARY_STATE,
)

ZONE_APPROACH_AWAY_STATE_KEYS = (
    CONF_ZONE_1_APPROACH_AWAY_STATE,
    CONF_ZONE_2_APPROACH_AWAY_STATE,
    CONF_ZONE_3_APPROACH_AWAY_STATE,
    CONF_ZONE_4_APPROACH_AWAY_STATE,
    CONF_ZONE_5_APPROACH_AWAY_STATE,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_C4004_ID): cv.use_id(C4004Component),
        cv.Optional(CONF_DETECTION_RANGE_MODE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:map-marker-radius",
            )
        ),
        cv.Optional(CONF_ZONE_1_BOUNDARY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:location-enter",
            )
        ),
        cv.Optional(CONF_ZONE_2_BOUNDARY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:location-enter",
            )
        ),
        cv.Optional(CONF_ZONE_3_BOUNDARY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:location-enter",
            )
        ),
        cv.Optional(CONF_ZONE_4_BOUNDARY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:location-enter",
            )
        ),
        cv.Optional(CONF_ZONE_5_BOUNDARY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:location-enter",
            )
        ),
        cv.Optional(CONF_ZONE_1_APPROACH_AWAY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:swap-horizontal",
            )
        ),
        cv.Optional(CONF_ZONE_2_APPROACH_AWAY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:swap-horizontal",
            )
        ),
        cv.Optional(CONF_ZONE_3_APPROACH_AWAY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:swap-horizontal",
            )
        ),
        cv.Optional(CONF_ZONE_4_APPROACH_AWAY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:swap-horizontal",
            )
        ),
        cv.Optional(CONF_ZONE_5_APPROACH_AWAY_STATE): disable_entity_mqtt_schema(
            text_sensor.text_sensor_schema(
            icon="mdi:swap-horizontal",
            )
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_C4004_ID])

    if mode_config := config.get(CONF_DETECTION_RANGE_MODE):
        sens = await text_sensor.new_text_sensor(without_entity_mqtt(mode_config))
        cg.add(parent.set_detection_range_mode_text_sensor(sens))

    for index, key in enumerate(ZONE_BOUNDARY_STATE_KEYS):
        if zone_config := config.get(key):
            sens = await text_sensor.new_text_sensor(without_entity_mqtt(zone_config))
            cg.add(parent.set_zone_boundary_state_text_sensor(index, sens))

    for index, key in enumerate(ZONE_APPROACH_AWAY_STATE_KEYS):
        if zone_config := config.get(key):
            sens = await text_sensor.new_text_sensor(without_entity_mqtt(zone_config))
            cg.add(parent.set_zone_approach_away_text_sensor(index, sens))
