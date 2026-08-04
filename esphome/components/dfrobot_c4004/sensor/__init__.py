import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv

from .. import CONF_C4004_ID, C4004Component, disable_entity_mqtt_schema, without_entity_mqtt

CONF_LIVE_COUNT = "live_count"
CONF_TARGET_COUNT = "target_count"
CONF_MOTION_STATE = "motion_state"
CONF_ZONE_1_MOVING_COUNT = "zone_1_moving_count"
CONF_ZONE_2_MOVING_COUNT = "zone_2_moving_count"
CONF_ZONE_3_MOVING_COUNT = "zone_3_moving_count"
CONF_ZONE_4_MOVING_COUNT = "zone_4_moving_count"
CONF_ZONE_5_MOVING_COUNT = "zone_5_moving_count"
CONF_ZONE_1_STATIC_COUNT = "zone_1_static_count"
CONF_ZONE_2_STATIC_COUNT = "zone_2_static_count"
CONF_ZONE_3_STATIC_COUNT = "zone_3_static_count"
CONF_ZONE_4_STATIC_COUNT = "zone_4_static_count"
CONF_ZONE_5_STATIC_COUNT = "zone_5_static_count"

ZONE_MOVING_COUNT_KEYS = (
    CONF_ZONE_1_MOVING_COUNT,
    CONF_ZONE_2_MOVING_COUNT,
    CONF_ZONE_3_MOVING_COUNT,
    CONF_ZONE_4_MOVING_COUNT,
    CONF_ZONE_5_MOVING_COUNT,
)

ZONE_STATIC_COUNT_KEYS = (
    CONF_ZONE_1_STATIC_COUNT,
    CONF_ZONE_2_STATIC_COUNT,
    CONF_ZONE_3_STATIC_COUNT,
    CONF_ZONE_4_STATIC_COUNT,
    CONF_ZONE_5_STATIC_COUNT,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_C4004_ID): cv.use_id(C4004Component),
        cv.Optional(CONF_LIVE_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:account-group",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_TARGET_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:target-account",
            accuracy_decimals=0,
            unit_of_measurement="targets",
            )
        ),
        cv.Optional(CONF_MOTION_STATE): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:run",
            accuracy_decimals=0,
            )
        ),
        cv.Optional(CONF_ZONE_1_MOVING_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:run-fast",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_2_MOVING_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:run-fast",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_3_MOVING_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:run-fast",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_4_MOVING_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:run-fast",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_5_MOVING_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:run-fast",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_1_STATIC_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:account",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_2_STATIC_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:account",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_3_STATIC_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:account",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_4_STATIC_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:account",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
        cv.Optional(CONF_ZONE_5_STATIC_COUNT): disable_entity_mqtt_schema(
            sensor.sensor_schema(
            icon="mdi:account",
            accuracy_decimals=0,
            unit_of_measurement="people",
            )
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_C4004_ID])

    if live_count_config := config.get(CONF_LIVE_COUNT):
        sens = await sensor.new_sensor(without_entity_mqtt(live_count_config))
        cg.add(parent.set_live_count_sensor(sens))

    if target_count_config := config.get(CONF_TARGET_COUNT):
        sens = await sensor.new_sensor(without_entity_mqtt(target_count_config))
        cg.add(parent.set_target_count_sensor(sens))

    if motion_state_config := config.get(CONF_MOTION_STATE):
        sens = await sensor.new_sensor(without_entity_mqtt(motion_state_config))
        cg.add(parent.set_motion_state_sensor(sens))

    for index, key in enumerate(ZONE_MOVING_COUNT_KEYS):
        if zone_config := config.get(key):
            sens = await sensor.new_sensor(without_entity_mqtt(zone_config))
            cg.add(parent.set_zone_moving_count_sensor(index, sens))

    for index, key in enumerate(ZONE_STATIC_COUNT_KEYS):
        if zone_config := config.get(key):
            sens = await sensor.new_sensor(without_entity_mqtt(zone_config))
            cg.add(parent.set_zone_static_count_sensor(index, sens))
