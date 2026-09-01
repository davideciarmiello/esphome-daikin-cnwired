import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import climate, switch, sensor, binary_sensor
from esphome.components.binary_sensor import binary_sensor_schema
from esphome.components.sensor import sensor_schema
from esphome.components.switch import switch_schema
from esphome.components.esp32 import include_builtin_idf_component
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_RADIATOR,
    ICON_THERMOMETER,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

# confs viste su https://github.com/lumixen/esphome-hlink-ac/blob/2026.7.1/components/hlink_ac/sensor/__init__.py

CONF_RX_PIN = "rx_pin"
CONF_TX_PIN = "tx_pin"
CONF_RX_INVERT = "rx_invert"
CONF_TX_INVERT = "tx_invert"
#CONF_POWERFUL = "powerful_preset"

CONF_AUTO_MODE = "auto_mode"
CONF_HEAT_COOL_AS_POWER_ON = "heat_cool_as_power_on"

CONF_TURBO_AS_PRESET = "turbo_as_preset"


CONF_POWER = "power_switch"
CONF_LED = "led_switch"
CONF_LISTEN_ONLY = "listen_only_switch"

CONF_ROOM_TEMPERATURE_SENSOR = "room_temperature_sensor"
CONF_ROOM_HUMIDITY_SENSOR = "room_humidity_sensor"

CONF_AC_TEMPERATURE_SENSOR = "ac_temperature_sensor"
CONF_ONLINE_SENSOR = "online_sensor"

cnwired_ns = cg.esphome_ns.namespace("daikin_cnwired")
DaikinCNWired = cnwired_ns.class_("DaikinCNWired", climate.Climate, cg.Component)
DaikinSwitch = cnwired_ns.class_("DaikinSwitch", switch.Switch, cg.Component)

SWITCH_SCHEMA = switch_schema(DaikinSwitch)
SWITCH_SCHEMA_DEBUG = switch_schema(DaikinSwitch, entity_category=ENTITY_CATEGORY_DIAGNOSTIC).extend(
        {
    cv.Optional("disabled_by_default", default=True): cv.boolean,
        })

CONFIG_SCHEMA = climate.climate_schema(DaikinCNWired).extend({
    cv.GenerateID(): cv.declare_id(DaikinCNWired),

    cv.Required(CONF_RX_PIN): pins.internal_gpio_input_pin_schema,
    cv.Required(CONF_TX_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_RX_INVERT, default=False): cv.boolean,
    cv.Optional(CONF_TX_INVERT, default=False): cv.boolean,
    cv.Optional(CONF_AUTO_MODE, default=True): cv.boolean,
    cv.Optional(CONF_HEAT_COOL_AS_POWER_ON, default=True): cv.boolean,
    cv.Optional(CONF_TURBO_AS_PRESET, default=False): cv.boolean,

    cv.Optional(CONF_POWER,): SWITCH_SCHEMA,
    cv.Optional(CONF_LED,): SWITCH_SCHEMA,
    cv.Optional(CONF_LISTEN_ONLY,): SWITCH_SCHEMA_DEBUG,
    
    cv.Optional(CONF_ROOM_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_ROOM_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),


    cv.Optional(CONF_AC_TEMPERATURE_SENSOR): sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT),

    cv.Optional(CONF_ONLINE_SENSOR): binary_sensor.binary_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),

}).extend(cv.COMPONENT_SCHEMA)

SWITCH_LED = 0
SWITCH_LISTEN_ONLY = 1
SWITCH_POWER = 2

async def to_code(config):
    # RMT viene utilizzato dal driver CN_WIRED
    include_builtin_idf_component("esp_driver_rmt")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    tx_pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_pins(rx_pin, tx_pin))
    cg.add(var.set_rx_invert(config[CONF_RX_INVERT]))
    cg.add(var.set_tx_invert(config[CONF_TX_INVERT]))
    cg.add(var.set_auto_mode(config[CONF_AUTO_MODE]))
    cg.add(var.set_heat_cool_as_power_on(config[CONF_HEAT_COOL_AS_POWER_ON]))
    cg.add(var.set_turbo_as_preset(config[CONF_TURBO_AS_PRESET]))
    #cg.add(var.set_powerful_preset(config[CONF_POWERFUL]))

    if CONF_POWER in config:
        sw_config = config[CONF_POWER]
        sw = cg.new_Pvariable(sw_config[CONF_ID],var,SWITCH_POWER)
        await switch.register_switch(sw, sw_config)
        cg.add(var.set_switch_power(sw))
        
    if CONF_LED in config:
        sw_config = config[CONF_LED]
        sw = cg.new_Pvariable(sw_config[CONF_ID],var,SWITCH_LED)
        await switch.register_switch(sw, sw_config)
        cg.add(var.set_switch_led(sw))
        
    if CONF_LISTEN_ONLY in config:
        sw_config = config[CONF_LISTEN_ONLY]
        sw = cg.new_Pvariable(sw_config[CONF_ID],var,SWITCH_LISTEN_ONLY)
        await switch.register_switch(sw, sw_config)
        cg.add(var.set_switch_listen_only(sw))

    if CONF_ROOM_TEMPERATURE_SENSOR in config:
        sens = await cg.get_variable(config[CONF_ROOM_TEMPERATURE_SENSOR])
        cg.add(var.set_room_temperature_sensor(sens))

    if CONF_ROOM_HUMIDITY_SENSOR in config:
        sens = await cg.get_variable(config[CONF_ROOM_HUMIDITY_SENSOR])
        cg.add(var.set_room_humidity_sensor(sens))

    if CONF_AC_TEMPERATURE_SENSOR in config:
        sw_config = config[CONF_AC_TEMPERATURE_SENSOR]
        sens = cg.new_Pvariable(sw_config[CONF_ID])
        await sensor.register_sensor(sens, sw_config)
        cg.add(var.set_ac_temperature_sensor(sens))

    if CONF_ONLINE_SENSOR in config:
        sensor_config = config[CONF_ONLINE_SENSOR]
        sens = await binary_sensor.new_binary_sensor(sensor_config)
        cg.add(var.set_online_sensor(sens))


