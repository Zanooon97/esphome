import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import CONF_C4004_ID, C4004Component, dfrobot_c4004_ns, disable_entity_mqtt_schema, without_entity_mqtt

CONF_INSTALL_HEIGHT = "install_height"
CONF_INSTALL_Z_ANGLE = "install_z_angle"
CONF_RANGE_X_MAX = "range_x_max"
CONF_RANGE_X_MIN = "range_x_min"
CONF_RANGE_Y_MAX = "range_y_max"
CONF_RANGE_Y_MIN = "range_y_min"
CONF_REAL_TIME_REPORT_INTERVAL = "real_time_report_interval"
CONF_TRAJECTORY_GENERATION_DISTANCE = "trajectory_generation_distance"
CONF_TRAJECTORY_LIFETIME = "trajectory_lifetime"
CONF_UNOCCUPIED_TIME = "unoccupied_time"
CONF_FRAME_GENERATE_COUNT = "frame_generate_count"
CONF_ZONE_1_MCU_IO = "zone_1_mcu_io"
CONF_ZONE_2_MCU_IO = "zone_2_mcu_io"
CONF_ZONE_3_MCU_IO = "zone_3_mcu_io"
CONF_ZONE_4_MCU_IO = "zone_4_mcu_io"
CONF_ZONE_5_MCU_IO = "zone_5_mcu_io"
CONF_ZONE_6_MCU_IO = "zone_6_mcu_io"

ZONE_MCU_IO_KEYS = (
    CONF_ZONE_1_MCU_IO,
    CONF_ZONE_2_MCU_IO,
    CONF_ZONE_3_MCU_IO,
    CONF_ZONE_4_MCU_IO,
    CONF_ZONE_5_MCU_IO,
    CONF_ZONE_6_MCU_IO,
)

C4004InstallHeightNumber = dfrobot_c4004_ns.class_(
    "C4004InstallHeightNumber", number.Number
)
C4004InstallZAngleNumber = dfrobot_c4004_ns.class_(
    "C4004InstallZAngleNumber", number.Number
)
C4004RangeXMaxNumber = dfrobot_c4004_ns.class_("C4004RangeXMaxNumber", number.Number)
C4004RangeXMinNumber = dfrobot_c4004_ns.class_("C4004RangeXMinNumber", number.Number)
C4004RangeYMaxNumber = dfrobot_c4004_ns.class_("C4004RangeYMaxNumber", number.Number)
C4004RangeYMinNumber = dfrobot_c4004_ns.class_("C4004RangeYMinNumber", number.Number)
C4004RealTimeReportIntervalNumber = dfrobot_c4004_ns.class_(
    "C4004RealTimeReportIntervalNumber", number.Number
)
C4004TrajectoryGenerationDistanceNumber = dfrobot_c4004_ns.class_(
    "C4004TrajectoryGenerationDistanceNumber", number.Number
)
C4004TrajectoryLifetimeNumber = dfrobot_c4004_ns.class_(
    "C4004TrajectoryLifetimeNumber", number.Number
)
C4004UnoccupiedTimeNumber = dfrobot_c4004_ns.class_(
    "C4004UnoccupiedTimeNumber", number.Number
)
C4004FrameGenerateCountNumber = dfrobot_c4004_ns.class_(
    "C4004FrameGenerateCountNumber", number.Number
)
C4004ZoneMcuIoNumber = dfrobot_c4004_ns.class_("C4004ZoneMcuIoNumber", number.Number)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_C4004_ID): cv.use_id(C4004Component),
        cv.Optional(CONF_INSTALL_HEIGHT): disable_entity_mqtt_schema(
            number.number_schema(
            C4004InstallHeightNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:human-male-height",
            unit_of_measurement="cm",
            )
        ),
        cv.Optional(CONF_INSTALL_Z_ANGLE): disable_entity_mqtt_schema(
            number.number_schema(
            C4004InstallZAngleNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:angle-acute",
            unit_of_measurement="deg",
            )
        ),
        cv.Optional(CONF_RANGE_X_MAX): disable_entity_mqtt_schema(
            number.number_schema(
            C4004RangeXMaxNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:axis-x-arrow",
            unit_of_measurement="cm",
            )
        ),
        cv.Optional(CONF_RANGE_X_MIN): disable_entity_mqtt_schema(
            number.number_schema(
            C4004RangeXMinNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:axis-x-arrow",
            unit_of_measurement="cm",
            )
        ),
        cv.Optional(CONF_RANGE_Y_MAX): disable_entity_mqtt_schema(
            number.number_schema(
            C4004RangeYMaxNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:axis-y-arrow",
            unit_of_measurement="cm",
            )
        ),
        cv.Optional(CONF_RANGE_Y_MIN): disable_entity_mqtt_schema(
            number.number_schema(
            C4004RangeYMinNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:axis-y-arrow",
            unit_of_measurement="cm",
            )
        ),
        cv.Optional(CONF_REAL_TIME_REPORT_INTERVAL): disable_entity_mqtt_schema(
            number.number_schema(
            C4004RealTimeReportIntervalNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:timer-sync",
            unit_of_measurement="s",
            )
        ),
        cv.Optional(CONF_TRAJECTORY_GENERATION_DISTANCE): disable_entity_mqtt_schema(
            number.number_schema(
            C4004TrajectoryGenerationDistanceNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:map-marker-distance",
            unit_of_measurement="cm",
            )
        ),
        cv.Optional(CONF_TRAJECTORY_LIFETIME): disable_entity_mqtt_schema(
            number.number_schema(
            C4004TrajectoryLifetimeNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:timer-outline",
            unit_of_measurement="s",
            )
        ),
        cv.Optional(CONF_UNOCCUPIED_TIME): disable_entity_mqtt_schema(
            number.number_schema(
            C4004UnoccupiedTimeNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:timer-off-outline",
            unit_of_measurement="s",
            )
        ),
        cv.Optional(CONF_FRAME_GENERATE_COUNT): disable_entity_mqtt_schema(
            number.number_schema(
            C4004FrameGenerateCountNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:counter",
            )
        ),
        cv.Optional(CONF_ZONE_1_MCU_IO): disable_entity_mqtt_schema(
            number.number_schema(
            C4004ZoneMcuIoNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:home-lightning-bolt-outline",
            )
        ),
        cv.Optional(CONF_ZONE_2_MCU_IO): disable_entity_mqtt_schema(
            number.number_schema(
            C4004ZoneMcuIoNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:numeric-2-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_3_MCU_IO): disable_entity_mqtt_schema(
            number.number_schema(
            C4004ZoneMcuIoNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:numeric-3-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_4_MCU_IO): disable_entity_mqtt_schema(
            number.number_schema(
            C4004ZoneMcuIoNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:numeric-4-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_5_MCU_IO): disable_entity_mqtt_schema(
            number.number_schema(
            C4004ZoneMcuIoNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:numeric-5-circle-outline",
            )
        ),
        cv.Optional(CONF_ZONE_6_MCU_IO): disable_entity_mqtt_schema(
            number.number_schema(
            C4004ZoneMcuIoNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:numeric-6-circle-outline",
            )
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_C4004_ID])

    if conf := config.get(CONF_INSTALL_HEIGHT):
        n = await number.new_number(without_entity_mqtt(conf), min_value=0, max_value=500, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_install_height_number(n))

    if conf := config.get(CONF_INSTALL_Z_ANGLE):
        n = await number.new_number(without_entity_mqtt(conf), min_value=0, max_value=90, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_install_z_angle_number(n))

    if conf := config.get(CONF_RANGE_X_MAX):
        n = await number.new_number(without_entity_mqtt(conf), min_value=-32767, max_value=32767, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_range_x_max_number(n))

    if conf := config.get(CONF_RANGE_X_MIN):
        n = await number.new_number(without_entity_mqtt(conf), min_value=-32767, max_value=32767, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_range_x_min_number(n))

    if conf := config.get(CONF_RANGE_Y_MAX):
        n = await number.new_number(without_entity_mqtt(conf), min_value=-32767, max_value=32767, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_range_y_max_number(n))

    if conf := config.get(CONF_RANGE_Y_MIN):
        n = await number.new_number(without_entity_mqtt(conf), min_value=-32767, max_value=32767, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_range_y_min_number(n))

    if conf := config.get(CONF_REAL_TIME_REPORT_INTERVAL):
        n = await number.new_number(without_entity_mqtt(conf), min_value=0, max_value=3600, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_real_time_report_interval_number(n))

    if conf := config.get(CONF_TRAJECTORY_GENERATION_DISTANCE):
        n = await number.new_number(without_entity_mqtt(conf), min_value=0, max_value=1000, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_trajectory_generation_distance_number(n))

    if conf := config.get(CONF_TRAJECTORY_LIFETIME):
        n = await number.new_number(without_entity_mqtt(conf), min_value=0, max_value=3600, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_trajectory_lifetime_number(n))

    if conf := config.get(CONF_UNOCCUPIED_TIME):
        n = await number.new_number(without_entity_mqtt(conf), min_value=0, max_value=3600, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_unoccupied_time_number(n))

    if conf := config.get(CONF_FRAME_GENERATE_COUNT):
        n = await number.new_number(without_entity_mqtt(conf), min_value=1, max_value=7, step=1)
        await cg.register_parented(n, config[CONF_C4004_ID])
        cg.add(parent.set_frame_generate_count_number(n))

    for index, key in enumerate(ZONE_MCU_IO_KEYS):
        if conf := config.get(key):
            n = await number.new_number(without_entity_mqtt(conf), min_value=-1, max_value=255, step=1)
            await cg.register_parented(n, config[CONF_C4004_ID])
            cg.add(n.set_zone_index(index))
            cg.add(parent.set_zone_mcu_io_number(index, n))
