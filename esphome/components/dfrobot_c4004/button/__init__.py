import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import CONF_C4004_ID, C4004Component, dfrobot_c4004_ns, disable_entity_mqtt_schema, without_entity_mqtt

CONF_FACTORY_RESET = "factory_reset"
CONF_RESET = "reset"
CONF_SET_INSTALL_INFO = "set_install_info"
CONF_SET_FOUR_SIDED_RANGE_MODE = "set_four_sided_range_mode"
CONF_TRAJECTORY_RANGE_MODE = "trajectory_range_mode"
CONF_CLEAR_LIVE_COUNT = "clear_live_count"

C4004FactoryResetButton = dfrobot_c4004_ns.class_(
    "C4004FactoryResetButton", button.Button
)
C4004ResetButton = dfrobot_c4004_ns.class_("C4004ResetButton", button.Button)
C4004SetInstallInfoButton = dfrobot_c4004_ns.class_(
    "C4004SetInstallInfoButton", button.Button
)
C4004SetFourSidedRangeModeButton = dfrobot_c4004_ns.class_(
    "C4004SetFourSidedRangeModeButton", button.Button
)
C4004TrajectoryRangeModeButton = dfrobot_c4004_ns.class_(
    "C4004TrajectoryRangeModeButton", button.Button
)
C4004ClearLiveCountButton = dfrobot_c4004_ns.class_(
    "C4004ClearLiveCountButton", button.Button
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_C4004_ID): cv.use_id(C4004Component),
        cv.Optional(CONF_FACTORY_RESET): disable_entity_mqtt_schema(
            button.button_schema(
            C4004FactoryResetButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:restore",
            )
        ),
        cv.Optional(CONF_RESET): disable_entity_mqtt_schema(
            button.button_schema(
            C4004ResetButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:restart",
            )
        ),
        cv.Optional(CONF_SET_INSTALL_INFO): disable_entity_mqtt_schema(
            button.button_schema(
            C4004SetInstallInfoButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:content-save-cog",
            )
        ),
        cv.Optional(CONF_SET_FOUR_SIDED_RANGE_MODE): disable_entity_mqtt_schema(
            button.button_schema(
            C4004SetFourSidedRangeModeButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:map-marker-radius",
            )
        ),
        cv.Optional(CONF_TRAJECTORY_RANGE_MODE): disable_entity_mqtt_schema(
            button.button_schema(
            C4004TrajectoryRangeModeButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:map-marker-distance",
            )
        ),
        cv.Optional(CONF_CLEAR_LIVE_COUNT): disable_entity_mqtt_schema(
            button.button_schema(
            C4004ClearLiveCountButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:countertop-outline",
            )
        ),
    }
)


async def to_code(config):
    for key in (
        CONF_FACTORY_RESET,
        CONF_RESET,
        CONF_SET_INSTALL_INFO,
        CONF_SET_FOUR_SIDED_RANGE_MODE,
        CONF_TRAJECTORY_RANGE_MODE,
        CONF_CLEAR_LIVE_COUNT,
    ):
        if button_config := config.get(key):
            btn = await button.new_button(without_entity_mqtt(button_config))
            await cg.register_parented(btn, config[CONF_C4004_ID])
