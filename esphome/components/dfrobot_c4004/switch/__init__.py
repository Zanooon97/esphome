import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import CONF_C4004_ID, C4004Component, dfrobot_c4004_ns, disable_entity_mqtt_schema, without_entity_mqtt

CONF_PRESENCE_ENABLE = "presence_enable"
CONF_TRAJECTORY_TRACK_ENABLE = "trajectory_track_enable"
CONF_TRK_LED = "trk_led"
CONF_OCC_LED = "occ_led"

C4004PresenceEnableSwitch = dfrobot_c4004_ns.class_(
    "C4004PresenceEnableSwitch", switch.Switch
)
C4004TrajectoryTrackEnableSwitch = dfrobot_c4004_ns.class_(
    "C4004TrajectoryTrackEnableSwitch", switch.Switch
)
C4004TrkLEDSwitch = dfrobot_c4004_ns.class_(
    "C4004TrkLEDSwitch", switch.Switch
)
C4004OccLEDSwitch = dfrobot_c4004_ns.class_(
    "C4004OccLEDSwitch", switch.Switch
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_C4004_ID): cv.use_id(C4004Component),
        cv.Optional(CONF_PRESENCE_ENABLE): disable_entity_mqtt_schema(
            switch.switch_schema(
            C4004PresenceEnableSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:human-handsup",
            )
        ),
        cv.Optional(CONF_TRAJECTORY_TRACK_ENABLE): disable_entity_mqtt_schema(
            switch.switch_schema(
            C4004TrajectoryTrackEnableSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:map-marker-path",
            )
        ),
        cv.Optional(CONF_TRK_LED): disable_entity_mqtt_schema(
            switch.switch_schema(
            C4004TrkLEDSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:led-on",
            )
        ),
        cv.Optional(CONF_OCC_LED): disable_entity_mqtt_schema(
            switch.switch_schema(
            C4004OccLEDSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:led-on",
            )
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_C4004_ID])

    if presence_enable_config := config.get(CONF_PRESENCE_ENABLE):
        sw = await switch.new_switch(without_entity_mqtt(presence_enable_config))
        await cg.register_parented(sw, config[CONF_C4004_ID])
        cg.add(parent.set_presence_enable_switch(sw))

    if trajectory_track_enable_config := config.get(CONF_TRAJECTORY_TRACK_ENABLE):
        sw = await switch.new_switch(without_entity_mqtt(trajectory_track_enable_config))
        await cg.register_parented(sw, config[CONF_C4004_ID])
        cg.add(parent.set_trajectory_track_enable_switch(sw))

    if trk_led_config := config.get(CONF_TRK_LED):
        sw = await switch.new_switch(without_entity_mqtt(trk_led_config))
        await cg.register_parented(sw, config[CONF_C4004_ID])
        cg.add(parent.set_trk_led_switch(sw))

    if occ_led_config := config.get(CONF_OCC_LED):
        sw = await switch.new_switch(without_entity_mqtt(occ_led_config))
        await cg.register_parented(sw, config[CONF_C4004_ID])
        cg.add(parent.set_occ_led_switch(sw))
