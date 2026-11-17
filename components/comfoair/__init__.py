import math
import esphome.codegen as cg
import esphome.cpp_generator as cppg
import esphome.config_validation as cv
from esphome import pins
from esphome.core import ID
from esphome.util import Registry
from esphome.const import CONF_ID, CONF_VERSION, CONF_NAME, UNIT_PERCENT, CONF_RX_PIN, CONF_TX_PIN, ENTITY_CATEGORY_CONFIG, ENTITY_CATEGORY_DIAGNOSTIC
from esphome.components import text_sensor, binary_sensor, sensor, climate, number, select
from enum import Enum

DEPENDENCIES = ['climate']
CONF_GLOBAL_FILTERS = "global_filters"

AUTO_LOAD = ["text_sensor", "binary_sensor", "sensor", "climate", "number", "select"]
MULTI_CONF = True

CONF_HUB_ID = 'comfoair'

empty_sensor_hub_ns = cg.esphome_ns.namespace('comfoair')

Comfoair = empty_sensor_hub_ns.class_('Comfoair', cg.Component)
ComfoairClimate = empty_sensor_hub_ns.class_('ComfoairClimate', cg.Component)
ComfoairNumber = empty_sensor_hub_ns.class_('ComfoairNumber', number.Number)
ComfoairSelect = empty_sensor_hub_ns.class_('ComfoairSelect', select.Select)
class ComfoNumConvs(Enum):
    UINT8 = 0,
    UINT16 = 1,
    UINT32 = 2,
    INT16 = 3

FILTER_REGISTRY = Registry()
validate_filters = cv.validate_registry("filter", FILTER_REGISTRY)
sensors = {
    "fan_speed":          {"unit": "", "PDO": 65, "CONV": ComfoNumConvs.UINT8},
    "next_fan_change":    {"unit": "s", "PDO": 81, "CONV": ComfoNumConvs.UINT32},
    "next_bypass_change": {"unit": "s", "PDO": 82, "CONV": ComfoNumConvs.UINT32},
    "next_supply_change": {"unit": "s", "PDO": 86, "CONV": ComfoNumConvs.UINT32},
    "next_exhaust_change": {"unit": "s", "PDO": 87, "CONV": ComfoNumConvs.UINT32},
    "next_profile_change": {"unit": "s", "PDO": 83, "CONV": ComfoNumConvs.UINT32},

    "exhaust_fan_duty":  {"unit":"%",    "PDO": 117, "CONV": ComfoNumConvs.UINT8 },
    "supply_fan_duty":   {"unit":"%",    "PDO": 118, "CONV": ComfoNumConvs.UINT8 },
    "exhaust_fan_flow":  {"unit":"m³/h", "PDO": 119, "CONV": ComfoNumConvs.UINT16},
    "supply_fan_flow":   {"unit":"m³/h", "PDO": 120, "CONV": ComfoNumConvs.UINT16},
    "exhaust_fan_speed": {"unit":"rpm",  "PDO": 121, "CONV": ComfoNumConvs.UINT16},
    "supply_fan_speed":  {"unit":"rpm",  "PDO": 122, "CONV": ComfoNumConvs.UINT16},

    "power_consumption_current":      {"unit": "W",   "PDO": 128, "CONV": ComfoNumConvs.UINT16},
    "energy_consumption_ytd":         {"unit": "kWh", "PDO": 129, "CONV": ComfoNumConvs.UINT16},
    "energy_consumption_since_start": {"unit": "kWh", "PDO": 130, "CONV": ComfoNumConvs.UINT16},

    "preheater_energy_ytd":           {"unit": "kWh", "PDO": 144, "CONV": ComfoNumConvs.UINT16},
    "preheater_energy_since_start":   {"unit": "kWh", "PDO": 145, "CONV": ComfoNumConvs.UINT16},
    "preheater_power_current":        {"unit": "W",   "PDO": 146, "CONV": ComfoNumConvs.UINT16},

    "days_remaining_filter": {"unit": "days", "PDO": 192, "CONV": ComfoNumConvs.UINT16},

    "avoided_heating_actual": {"unit": "W", "PDO": 213, "CONV": ComfoNumConvs.UINT16},
    "avoided_heating_ytd":    {"unit": "kWh", "PDO": 214, "CONV": ComfoNumConvs.UINT16},
    "avoided_heating_total":  {"unit": "kWh", "PDO": 215, "CONV": ComfoNumConvs.UINT16},

    "avoided_cooling_actual": {"unit": "W", "PDO": 216, "CONV": ComfoNumConvs.UINT16},
    "avoided_cooling_ytd":    {"unit": "kWh", "PDO": 217, "CONV": ComfoNumConvs.UINT16},
    "avoided_cooling_total":  {"unit": "kWh", "PDO": 218, "CONV": ComfoNumConvs.UINT16},

    "bypass_state":  {"unit": "%", "PDO": 227, "CONV": ComfoNumConvs.UINT8},

    # 0, 100, 200 or 300, anything between while sensor ventilation modulates the speed
    "fan_speed_modulated":        {"unit": "", "PDO": 226, "CONV": ComfoNumConvs.UINT16},
    "frost_protection_unbalance": {"unit": "%", "PDO": 228, "CONV": ComfoNumConvs.UINT8},

    # temps
    "rmot":                     {"unit": "°C", "PDO": 209, "CONV": ComfoNumConvs.INT16, "div": 10},
    "target_temp":              {"unit": "°C", "PDO": 212, "CONV": ComfoNumConvs.UINT16, "div": 10},
    "pre_heater_temp_before":   {"unit": "°C", "PDO": 220, "CONV": ComfoNumConvs.INT16, "div": 10},
    "post_heater_temp_before":  {"unit": "°C", "PDO": 221, "CONV": ComfoNumConvs.INT16, "div": 10},
    "extract_air_temp":         {"unit": "°C", "PDO": 274, "CONV": ComfoNumConvs.INT16, "div": 10},
    "exhaust_air_temp":         {"unit": "°C", "PDO": 275, "CONV": ComfoNumConvs.INT16, "div": 10},
    "outdoor_air_temp":         {"unit": "°C", "PDO": 276, "CONV": ComfoNumConvs.INT16, "div": 10},

    "pre_heater_temp_after":    {"unit": "°C", "PDO": 277, "CONV": ComfoNumConvs.INT16, "div": 10},
    "post_heater_temp_after":   {"unit": "°C", "PDO": 278, "CONV": ComfoNumConvs.INT16, "div": 10},

    # humidity
    "extract_air_humidity":   {"unit": "%", "PDO": 290, "CONV": ComfoNumConvs.UINT8},
    "exhaust_air_humidity":   {"unit": "%", "PDO": 291, "CONV": ComfoNumConvs.UINT8},
    "outdoor_air_humidity":   {"unit": "%", "PDO": 292, "CONV": ComfoNumConvs.UINT8},
    "pre_heater_hum_after":   {"unit": "%", "PDO": 293, "CONV": ComfoNumConvs.UINT8},
    "supply_air_humidity":    {"unit": "%", "PDO": 294, "CONV": ComfoNumConvs.UINT8},


    # analog ports
    "analog_input_0_10v_1":   {"unit": "V", "PDO": 369, "CONV": ComfoNumConvs.UINT8, "div": 10},
    "analog_input_0_10v_2":   {"unit": "V", "PDO": 370, "CONV": ComfoNumConvs.UINT8, "div": 10},
    "analog_input_0_10v_3":   {"unit": "V", "PDO": 371, "CONV": ComfoNumConvs.UINT8, "div": 10},
    "analog_input_0_10v_4":   {"unit": "V", "PDO": 372, "CONV": ComfoNumConvs.UINT8, "div": 10},

}

textSensors = {
    "operating_mode": {
        "PDO": 49,
        "code": '''
return vals[0] == 1 ? "limited_manual": (vals[0] == 0xff ? "auto": "unlimited_manual");
'''
    },
    "bypass_activation_mode": {
        "PDO": 66,
        "code": '''
return  vals[0] == 0 ? "auto": (vals[0] == 1 ? "activated": "deactivated");
'''
    },
    "temp_profile": {
        "PDO": 67,
        "code": '''
return vals[0] == 0 ? "auto": (vals[0] == 1 ? "cold": "warm");
'''
    },
    "application_mode": {
        "PDO": 18,
        "code": '''
static const char *const names[] = {"init", "normal", "filter_wizard", "commissioning", "supplier_factory",
                                    "zehnder_factory", "recovery", "service_print", "component_check"};
return vals[0] < 9 ? names[vals[0]] : std::to_string(vals[0]);
'''
    },
    "sensor_ventilation_mode": {
        "PDO": 225,
        "code": '''
static const char *const names[] = {"disabled", "active", "overruling"};
return vals[0] < 3 ? names[vals[0]] : std::to_string(vals[0]);
'''
    },
    # Why the unit doesn't run at the requested airflow, as in aiocomfoconnect calculate_airflow_constraints()
    "ventilation_constraints": {
        "PDO": 230,
        "code": '''
uint64_t bits = 0;
for (int i = 0; i < 8; i++) bits |= (uint64_t) vals[i] << (8 * i);
if (!(bits & (1ULL << 45))) return "none";
static const struct { uint8_t bit; const char *name; } names[] = {
    {2, "resistance"}, {3, "resistance"}, {4, "preheater_negative"}, {5, "noise_guard"}, {7, "noise_guard"},
    {6, "resistance_guard"}, {8, "resistance_guard"}, {9, "frost_protection"}, {10, "bypass"},
    {12, "analog_input_1"}, {13, "analog_input_2"}, {14, "analog_input_3"}, {15, "analog_input_4"},
    {16, "hood"}, {18, "analog_preset"}, {19, "comfocool"}, {22, "preheater_positive"},
    {23, "rf_sensor_flow_preset"}, {24, "rf_sensor_flow_proportional"}, {25, "temperature_comfort"},
    {26, "humidity_comfort"}, {27, "humidity_protection"}, {41, "frost_protection_unbalance"},
    {42, "frost_protection_unbalance"}, {47, "co2_zone_1"}, {48, "co2_zone_2"}, {49, "co2_zone_3"},
    {50, "co2_zone_4"}, {51, "co2_zone_5"}, {52, "co2_zone_6"}, {53, "co2_zone_7"}, {54, "co2_zone_8"}};
std::string text;
const char *last = nullptr;
for (const auto &n : names) {
  if (!(bits & (1ULL << n.bit)) || (last && strcmp(n.name, last) == 0)) continue;  // pairs sharing a name are next to each other
  if (!text.empty()) text += ", ";
  text += n.name;
  last = n.name;
}
return text.empty() ? "other" : text;
'''
    },

}

# Read with RMI "get property", and the numbers and selects written with "set property".
# Allowed only if also in RMI_PROPERTIES in rmi.h with the same kind.
DIAGNOSTIC = {"category": ENTITY_CATEGORY_DIAGNOSTIC}
rmiTextSensors = {
    "hardware_version": DIAGNOSTIC,
    "firmware_version": DIAGNOSTIC,
}
# Commissioning, read only
rmiSensors = {
    "airflow_away":               {"unit": "m³/h", "decimals": 0, **DIAGNOSTIC},
    "airflow_low":                {"unit": "m³/h", "decimals": 0, **DIAGNOSTIC},
    "airflow_medium":             {"unit": "m³/h", "decimals": 0, **DIAGNOSTIC},
    "airflow_high":               {"unit": "m³/h", "decimals": 0, **DIAGNOSTIC},
    "bathroom_switch_on_delay":   {"unit": "s", "decimals": 0, **DIAGNOSTIC},
    "bathroom_switch_off_delay":  {"unit": "min", "decimals": 0, **DIAGNOSTIC},
}
# min and max are only until the unit's own range is read, the step is the resolution
rmiNumbers = {
    "rmot_heating_threshold":     {"unit": "°C", "min": 0, "max": 20, "step": 0.1},
    "rmot_cooling_threshold":     {"unit": "°C", "min": 15, "max": 40, "step": 0.1},
    "comfort_temperature_warm":   {"unit": "°C", "min": 15, "max": 30, "step": 0.1},
    "comfort_temperature_normal": {"unit": "°C", "min": 15, "max": 30, "step": 0.1},
    "comfort_temperature_cool":   {"unit": "°C", "min": 15, "max": 30, "step": 0.1},
    "filter_warning_period":      {"unit": "days", "min": 0, "max": 365, "step": 1},
    "filter_lifetime":            {"unit": "days", "min": 0, "max": 365, "step": 1},
}
# Options in the order of their values (0, 1, 2)
CONTROL_OPTIONS = ["off", "auto only", "on"]
rmiSelects = {
    "passive_temperature_control":       CONTROL_OPTIONS,
    "passive_temperature_control_speed": ["slow", "medium", "fast"],
    "temperature_profile_mode":          ["adaptive", "fixed"],
    "humidity_comfort_control":          CONTROL_OPTIONS,
    "humidity_protection":               CONTROL_OPTIONS,
}

binarySensors = {
    "alarm": {
        "PDO": 17,
        "code": '''
return vals[0] == 1;
        '''
    },
    # summer/winter mode
    "heating_season":   {
        "unit": "",
        "PDO": 210,
        "code": '''
return vals[0] == 1;
        '''
    },
    "cooling_season":   {
        "unit": "",
        "PDO": 211,
        "code": '''
return vals[0] == 1;
        '''
    },
    "manual_mode": {
        "PDO": 56,
        "code": '''
return vals[0] == 1;
        '''
    },
    "away": {
        "PDO": 16,
        "code": '''
return vals[0] == 0x08;
        '''
    },

    "fan_supply_only": {
        "PDO": 70,
        "code": '''
return vals[0] != 0;
        '''
    },
    "fan_exhaust_only": {
        "PDO": 71,
        "code": '''
return vals[0] != 0;
        '''
    },
}
GEN_SENSORS_SCHEMA = {
    cv.Optional(key, default=key): cv.maybe_simple_value(
        sensor.sensor_schema(unit_of_measurement=value['unit'], accuracy_decimals=math.trunc(math.log10(value['div'])) if 'div' in value else 0), key=CONF_NAME, )
    for key, value in sensors.items()
}

GEN_TEXTSENSORS_SCHEMA = {
    cv.Optional(key, default=key): cv.maybe_simple_value(
        text_sensor.text_sensor_schema(), key=CONF_NAME, accuracy_decimals=2 )
    for key, value in textSensors.items()
}

GEN_RMITEXTSENSORS_SCHEMA = {
    cv.Optional(key, default=key): cv.maybe_simple_value(
        text_sensor.text_sensor_schema(entity_category=value.get("category", cv.UNDEFINED)), key=CONF_NAME)
    for key, value in rmiTextSensors.items()
}

GEN_RMISENSORS_SCHEMA = {
    cv.Optional(key, default=key): cv.maybe_simple_value(
        sensor.sensor_schema(unit_of_measurement=value["unit"], accuracy_decimals=value["decimals"],
                             entity_category=value.get("category", cv.UNDEFINED)), key=CONF_NAME)
    for key, value in rmiSensors.items()
}

GEN_RMINUMBERS_SCHEMA = {
    cv.Optional(key, default=key): cv.maybe_simple_value(
        number.number_schema(ComfoairNumber, unit_of_measurement=value["unit"], entity_category=ENTITY_CATEGORY_CONFIG),
        key=CONF_NAME)
    for key, value in rmiNumbers.items()
}

GEN_RMISELECTS_SCHEMA = {
    cv.Optional(key, default=key): cv.maybe_simple_value(
        select.select_schema(ComfoairSelect, entity_category=ENTITY_CATEGORY_CONFIG), key=CONF_NAME)
    for key in rmiSelects
}

GEN_BINARYSENSORS_SCHEMA = {
    cv.Optional(key, default=key): cv.maybe_simple_value(
        binary_sensor.binary_sensor_schema(), key=CONF_NAME)
    for key, value in binarySensors.items()
}

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(Comfoair).extend({
        cv.GenerateID(): cv.declare_id(Comfoair),
        cv.Optional(CONF_RX_PIN, default=21): pins.internal_gpio_input_pin_number,
        cv.Optional(CONF_TX_PIN, default=25): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_GLOBAL_FILTERS): sensor.validate_filters,
    })
    .extend(GEN_SENSORS_SCHEMA)
    .extend(GEN_TEXTSENSORS_SCHEMA)
    .extend(GEN_RMITEXTSENSORS_SCHEMA)
    .extend(GEN_RMISENSORS_SCHEMA)
    .extend(GEN_RMINUMBERS_SCHEMA)
    .extend(GEN_RMISELECTS_SCHEMA)
    .extend(GEN_BINARYSENSORS_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    )



async def to_code(config):
    cg.add_library("can_common", None, "https://github.com/collin80/can_common.git#8585f9dc807ebbeedeb509d74159f40f538d2d65")
    # TechOverflow fork supports ESP-IDF 5.5+ and declares its can_common dependency.
    cg.add_library("esp32_can", None, "https://github.com/TechOverflow/esp32_can.git#695ad3b38d6b9ef4b047d92d6cc7dbdcccea9f76")


    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_rx(config[CONF_RX_PIN]))
    cg.add(var.set_tx(config[CONF_TX_PIN]))
    for key, value in sensors.items():
        sens = await sensor.new_sensor(config[key])

        filterconf = []
        if (config.get(CONF_GLOBAL_FILTERS)):
            for filter in config[CONF_GLOBAL_FILTERS]:
               tmp = filter.copy()
               tmp['type_id'] = ID(tmp['type_id'].id + key, type=tmp['type_id'].type)
               filterconf.append(tmp)
        
        if filterconf:
            cg.add_define("USE_SENSOR_FILTER")
            filters = await sensor.build_filters(filterconf)    
            cg.add(sens.set_filters(filters))

        cg.add(var.register_sensor(sens, key, value['PDO'], value['CONV'].value, value['div'] if 'div' in value else 1))

    for key, value in textSensors.items():
        sens = await text_sensor.new_text_sensor(config[key])
        lamda = cppg.RawExpression(f'''
[](uint8_t *vals) -> std::string {{ 
    {value['code']}
}}
''')
        cg.add(var.register_textSensor(sens, key, value['PDO'], lamda))

    for key in rmiTextSensors:
        sens = await text_sensor.new_text_sensor(config[key])
        cg.add(var.register_rmiTextSensor(sens, key))

    for key in rmiSensors:
        sens = await sensor.new_sensor(config[key])
        cg.add(var.register_rmiSensor(sens, key))

    for key, value in rmiNumbers.items():
        num = await number.new_number(config[key], min_value=value["min"], max_value=value["max"], step=value["step"])
        cg.add(var.register_rmiNumber(num, key))

    for key, options in rmiSelects.items():
        sel = await select.new_select(config[key], options=options)
        cg.add(var.register_rmiSelect(sel, key))

    for key, value in binarySensors.items():
        sens = await binary_sensor.new_binary_sensor(config[key])
        lamda = cppg.RawExpression(f'''
[](uint8_t *vals) -> bool {{ 
    {value['code']}
}}
''')
        cg.add(var.register_binarySensor(sens, key, value['PDO'], lamda))

    # Register climate entity with new ESPHome API (must be awaited!)
    await climate.register_climate(var, config)
