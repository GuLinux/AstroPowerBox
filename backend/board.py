import json
import logging
import math
import os
import sys
from protocols.typing_compat import Callable
import protocols.gpio
from config import Config
import protocols.wifi_manager
import protocols.config_storage
from status_led import StatusLed
from board_compat import ConfigStorage, WiFiManager, pinout_config_path, gpio, asyncio

logger = logging.getLogger(__name__)


class Board:
    config_storage: protocols.config_storage.ConfigStorage
    config: Config
    wifi_manager: protocols.wifi_manager.WiFiManager
    pinout_config: dict
    pin_states: dict
    fan_pin_id: str | None
    heater_temperature_pin_names: dict[str, str]
    heater_temperature_pins: dict[str, protocols.gpio.AnalogInputPin]
    heater_temperature_models: dict[str, dict]
    temperature_stream_interval_s: float

    def list_available_pinout_files(self) -> list[dict]:
        files = []
        for entry in self.__scan_pinout_files():
            if not self.__is_pinout_compatible(entry['metadata']):
                continue

            metadata = entry['metadata']
            files.append({
                'file': entry['file'],
                'path': entry['path'],
                'hardware': metadata.get('hardware'),
                'variant': metadata.get('variant'),
                'board': metadata.get('board'),
                'hardwareName': metadata.get('hardware_name', metadata.get('hardware')),
            })

        files.sort(key=lambda item: item['file'])
        return files

    def get_pinout_selection(self) -> dict:
        return {
            'configured': self.config.pinout_file,
            'selected': self.pinout_config_file,
            'selectedFile': os.path.basename(self.pinout_config_file),
            'restartRequired': False,
        }

    def set_pinout_file(self, pinout_file: str) -> dict:
        if '/' in pinout_file or '\\' in pinout_file:
            raise ValueError('Pinout file must be a file name, not a path')

        available = self.list_available_pinout_files()
        matching = [entry for entry in available if entry['file'] == pinout_file]
        if len(matching) == 0:
            raise ValueError(f'Pinout file not found or incompatible: {pinout_file}')

        selected_path = matching[0]['path']
        self.config.pinout_file = pinout_file
        self.config.save()
        return {
            'configured': pinout_file,
            'selected': self.pinout_config_file,
            'selectedFile': os.path.basename(self.pinout_config_file),
            'restartRequired': selected_path != self.pinout_config_file,
        }

    def on_pin_update(self, callback: Callable[[dict], None]) -> None:
        self._pin_update_listeners.append(callback)

    def pin_status_snapshot(self) -> dict:
        self.__refresh_heater_temperatures()
        return {
            'pins': list(self.pin_states.values())
        }

    def pin_event_snapshot(self) -> dict:
        self.__refresh_heater_temperatures()
        return self.__build_pin_event_snapshot()

    async def start(self):
        self.apply_fan_duty()
        if len(getattr(self, 'heater_temperature_pins', {})) > 0:
            asyncio.create_task(self.__temperature_stream_loop())
        await self.status_led.start()

    def __init__(self):
        self.config_storage = ConfigStorage()
        self.config = Config(self.config_storage)
        self.wifi_manager = WiFiManager(self.config)
        self._pin_update_listeners: list[Callable[[dict], None]] = []
        self.pin_states = {}
        self.output_pins = {}
        self.button_pins = {}
        self.fan_pin_id = None
        self.heater_temperature_pin_names = {}
        self.heater_temperature_pins = {}
        self.heater_temperature_models = {}
        self.temperature_stream_interval_s = 2.0

        self.pinout_config_file = self.__resolve_pinout_config_path()
        with open(self.pinout_config_file, 'r') as pinout_config_file:
            print(f'Loading pinout file: {self.pinout_config_file}')
            self.pinout_config = json.load(pinout_config_file)

        self.__load_gpio_pin_definitions()

        output_configs = self.__collect_output_configs()
        configured_pins = {}
        for pin_id, role, output_cfg in output_configs:
            self.__log_pin_initialization(configured_pins, pin_id, role, output_cfg['type'], output_cfg['pin'])
            output_pin = self.__load_output(pin_id, role, output_cfg)
            self.__register_output_pin(pin_id, role, output_pin)
            self.output_pins[pin_id] = output_pin

            temperature_pin_name = self.__resolve_temperature_pin_name(role, output_cfg)
            if temperature_pin_name:
                self.heater_temperature_pin_names[pin_id] = str(temperature_pin_name)
                self.heater_temperature_models[pin_id] = self.__resolve_thermistor_model(output_cfg)

        for pin_id, button_pin_name in self.__collect_button_configs():
            self.__log_pin_initialization(configured_pins, pin_id, 'button', 'input', button_pin_name)
            button_pin = self.__load_button(pin_id, button_pin_name)
            self.button_pins[pin_id] = button_pin
            self.__register_button_pin(pin_id, button_pin)

        if 'status_led' not in self.output_pins:
            raise RuntimeError('Pinout config is missing status_led output definition')

        self.status_led = StatusLed(self.output_pins['status_led'], self.config)
        self.wifi_manager.on_connecting = lambda: self.status_led.wifi_connecting()
        self.wifi_manager.on_station_connected = lambda _: self.status_led.status_ok()
        self.wifi_manager.on_ap_started = lambda _: self.status_led.wifi_failed()
        self.__load_heater_temperature_pins()
        self.__refresh_heater_temperatures()

    def has_fan_output(self) -> bool:
        return self.fan_pin_id is not None and self.fan_pin_id in self.output_pins

    def apply_fan_duty(self) -> None:
        if not self.has_fan_output():
            return

        fan_pin = self.output_pins[self.fan_pin_id]
        duty = max(0.0, min(1.0, float(self.config.fan_duty)))
        fan_pin.duty = duty

    def set_fan_duty(self, duty: float) -> None:
        self.config.fan_duty = duty
        self.apply_fan_duty()

    def has_temperature_sensor(self, pin_id: str) -> bool:
        return pin_id in self.heater_temperature_pins
        

    def __load_output(self, pin_id: str, role: str, config: dict):
        pin_name = config['pin']
        try:
            if config['type'] == 'pwm':
                return gpio.PWMOutputPin(pin_name)
            return gpio.DigitalOutputPin(pin_name)
        except Exception as error:
            raise RuntimeError(
                f"Failed to initialize {role} pin '{pin_id}' configured as '{pin_name}': {error}"
            ) from error

    def __load_button(self, pin_id: str, pin_name: str):
        try:
            return gpio.ButtonPin(pin_name)
        except Exception as error:
            raise RuntimeError(
                f"Failed to initialize button pin '{pin_id}' configured as '{pin_name}': {error}"
            ) from error

    def __log_pin_initialization(self, configured_pins: dict, pin_id: str, role: str, pin_type: str, pin_name):
        previous_pin = configured_pins.get(pin_name)
        if previous_pin:
            logger.debug(
                "Pin '%s' (%s, %s) reuses configured pin '%s' already used by '%s'",
                pin_id, role, pin_type, pin_name, previous_pin,
            )
        else:
            logger.debug(
                "Initializing pin '%s' (%s, %s) configured as '%s'",
                pin_id, role, pin_type, pin_name,
            )
            configured_pins[pin_name] = pin_id

    def __collect_output_configs(self) -> list[tuple[str, str, dict]]:
        output_configs: list[tuple[str, str, dict]] = []

        status_led = self.pinout_config.get('status_led')
        if isinstance(status_led, dict):
            output_configs.append(('status_led', 'status_led', status_led))

        for index, heater in enumerate(self.pinout_config.get('heaters', [])):
            output_configs.append((f'heater_{index}', 'heater', heater))

        for index, output in enumerate(self.pinout_config.get('outputs', [])):
            output_configs.append((f'output_{index}', 'output', output))

        fan_output = self.__resolve_fan_output_config()
        if fan_output is not None:
            output_configs.append(('fan', 'fan', fan_output))
            self.fan_pin_id = 'fan'

        pinout_section = self.pinout_config.get('pinout', {})
        if isinstance(pinout_section, dict):
            status_led = pinout_section.get('status_led')
            if 'status_led' not in [pin_id for pin_id, _, _ in output_configs] and status_led is not None:
                if isinstance(status_led, dict):
                    output_configs.append(('status_led', 'status_led', status_led))
                else:
                    output_configs.append(('status_led', 'status_led', {'type': 'digital', 'pin': status_led}))

            for index, output in enumerate(pinout_section.get('pwm_outputs', [])):
                role = 'heater' if str(output.get('type', '')).lower() == 'heater' else 'output'
                config = {'type': 'pwm', 'pin': output['pin']}
                if 'thermistor_pin' in output:
                    config['thermistor_pin'] = output.get('thermistor_pin')
                output_configs.append((output.get('name', f'{role}_{index}'), role, config))

        return output_configs

    def __resolve_fan_output_config(self) -> dict | None:
        fan = self.pinout_config.get('fan')
        if isinstance(fan, dict):
            return fan

        pinout_section = self.pinout_config.get('pinout', {})
        if isinstance(pinout_section, dict):
            fan = pinout_section.get('fan')
            if isinstance(fan, dict):
                return fan

        # Backward compatibility for legacy pinout files.
        fan_pwm = self.pinout_config.get('fan_pwm')
        if fan_pwm is not None:
            if isinstance(fan_pwm, dict):
                return fan_pwm
            return {'type': 'pwm', 'pin': fan_pwm}

        if isinstance(pinout_section, dict):
            fan_pwm = pinout_section.get('fan_pwm')
            if fan_pwm is not None:
                if isinstance(fan_pwm, dict):
                    return fan_pwm
                return {'type': 'pwm', 'pin': fan_pwm}

        return None

    def __resolve_temperature_pin_name(self, role: str, output_cfg: dict) -> str | None:
        if role != 'heater':
            return None

        temp_pin = output_cfg.get('temp')
        if isinstance(temp_pin, dict):
            if str(temp_pin.get('type', 'thermistor')).lower() != 'thermistor':
                return None
            pin_name = temp_pin.get('pin')
            if pin_name not in (None, ''):
                return str(pin_name)
            return None

        if temp_pin not in (None, ''):
            return str(temp_pin)

        thermistor_pin = output_cfg.get('thermistor_pin')
        if thermistor_pin in (None, '', -1):
            return None
        return str(thermistor_pin)

    def __resolve_thermistor_model(self, output_cfg: dict) -> dict:
        defaults = {
            'beta': 3950.0,
            'r0': 10000.0,
            't0_c': 25.0,
            'series_resistor': 10000.0,
            'vcc': 3.3,
            'wiring': 'ntc_to_gnd',
        }

        global_model = self.pinout_config.get('thermistor')
        if isinstance(global_model, dict):
            defaults.update(global_model)

        pinout_section = self.pinout_config.get('pinout', {})
        if isinstance(pinout_section, dict):
            nested_global_model = pinout_section.get('thermistor')
            if isinstance(nested_global_model, dict):
                defaults.update(nested_global_model)

        temp_pin = output_cfg.get('temp')
        if isinstance(temp_pin, dict):
            defaults.update(temp_pin)

        local_model = output_cfg.get('thermistor')
        if isinstance(local_model, dict):
            defaults.update(local_model)

        model = {}
        for key in ['beta', 'r0', 't0_c', 'series_resistor', 'vcc']:
            try:
                model[key] = float(defaults[key])
            except Exception:
                model[key] = float({
                    'beta': 3950.0,
                    'r0': 10000.0,
                    't0_c': 25.0,
                    'series_resistor': 10000.0,
                    'vcc': 3.3,
                }[key])

        model['wiring'] = str(defaults.get('wiring', 'ntc_to_gnd')).lower()
        model['type'] = str(defaults.get('type', 'thermistor')).lower()
        return model

    def __voltage_to_temperature_c(self, voltage: float, model: dict) -> float | None:
        vcc = model['vcc']
        if voltage <= 0.0 or voltage >= vcc:
            return None

        series_resistor = model['series_resistor']
        if model.get('wiring') == 'ntc_to_vcc':
            resistance = series_resistor * (vcc - voltage) / voltage
        else:
            resistance = series_resistor * voltage / (vcc - voltage)

        if resistance <= 0.0:
            return None

        t0_k = model['t0_c'] + 273.15
        beta = model['beta']
        r0 = model['r0']

        try:
            inv_t = (1.0 / t0_k) + (1.0 / beta) * math.log(resistance / r0)
            temperature_k = 1.0 / inv_t
            return temperature_k - 273.15
        except Exception:
            return None

    def __load_heater_temperature_pins(self) -> None:
        self.heater_temperature_pins = {}
        for pin_id, pin_name in self.heater_temperature_pin_names.items():
            try:
                self.heater_temperature_pins[pin_id] = gpio.AnalogInputPin(pin_name)
            except Exception as error:
                logger.warning(
                    "Failed to initialize heater temperature sensor for '%s' configured as '%s': %s",
                    pin_id,
                    pin_name,
                    error,
                )

    def __refresh_heater_temperatures(self) -> bool:
        changed = False
        for pin_id, sensor in self.heater_temperature_pins.items():
            pin_state = self.pin_states.get(pin_id)
            if not isinstance(pin_state, dict):
                continue

            previous_value = pin_state.get('temperature')
            try:
                voltage = float(sensor.value)
                model = self.heater_temperature_models.get(pin_id, self.__resolve_thermistor_model({}))
                temperature_c = self.__voltage_to_temperature_c(voltage, model)
                pin_state['temperature'] = round(float(temperature_c), 2) if temperature_c is not None else None
                logger.debug(
                    "Heater '%s' raw voltage=%.4fV mapped temperature=%sC",
                    pin_id,
                    voltage,
                    'None' if pin_state['temperature'] is None else f"{pin_state['temperature']:.2f}",
                )
            except Exception as error:
                pin_state['temperature'] = None
                logger.warning("Failed reading heater temperature for '%s': %s", pin_id, error)

            if pin_state.get('temperature') != previous_value:
                changed = True

        return changed

    def __build_pin_event_snapshot(self) -> dict:
        event = {}
        for pin_id, state in self.pin_states.items():
            if pin_id == 'status_led':
                continue

            pin_data = {'on': bool(state.get('on', False))}
            if 'duty' in state:
                pin_data['duty'] = state.get('duty', 0.0)
            if 'temperature' in state:
                pin_data['temperature'] = state.get('temperature')
            event[pin_id] = pin_data

        return event

    async def __temperature_stream_loop(self):
        while True:
            if self.__refresh_heater_temperatures() and len(self._pin_update_listeners) > 0:
                event = self.__build_pin_event_snapshot()
                for callback in self._pin_update_listeners:
                    callback(event)
            await asyncio.sleep(self.temperature_stream_interval_s)

    def __collect_button_configs(self) -> list[tuple[str, str]]:
        button_configs: list[tuple[str, str]] = []

        for index, button in enumerate(self.pinout_config.get('buttons', [])):
            if isinstance(button, str):
                button_configs.append((f'button_{index}', button))
            elif isinstance(button, dict):
                button_configs.append((button.get('name', f'button_{index}'), button['pin']))

        pinout_section = self.pinout_config.get('pinout', {})
        if isinstance(pinout_section, dict):
            for index, button in enumerate(pinout_section.get('buttons', [])):
                if isinstance(button, dict):
                    button_configs.append((button.get('name', f'button_{len(button_configs) + index}'), button['pin']))

        return button_configs

    def __register_output_pin(self, pin_id: str, role: str, output_pin):
        if output_pin.is_pwm:
            output_pin.on_duty_changed(lambda duty, ref=pin_id, pin_role=role: self.__publish_pin_state(ref, pin_role, 'pwm', duty))
            output_pin.duty = 0.0
            self.__publish_pin_state(pin_id, role, 'pwm', output_pin.duty)
            return

        output_pin.on_level_changed(lambda on, ref=pin_id, pin_role=role: self.__publish_pin_state(ref, pin_role, 'digital', on))
        output_pin.on = False
        self.__publish_pin_state(pin_id, role, 'digital', output_pin.on)

    def __register_button_pin(self, pin_id: str, button_pin):
        button_pin.on_level_changed(lambda pressed, ref=pin_id: self.__publish_pin_state(ref, 'button', 'button', pressed))
        self.__publish_pin_state(pin_id, 'button', 'button', button_pin.value)

    def __publish_pin_state(self, pin_id: str, role: str, kind: str, value):
        pin_state: dict[str, object] = {
            'id': pin_id,
            'role': role,
            'kind': kind,
        }
        if kind == 'pwm':
            pin_state['duty'] = value
            pin_state['on'] = value > 0
        else:
            pin_state['on'] = bool(value)

        self.pin_states[pin_id] = pin_state
        if pin_id == 'status_led':
            return

        event = self.pin_event_snapshot()
        for callback in self._pin_update_listeners:
            callback(event)

    def __load_gpio_pin_definitions(self):
        if not hasattr(gpio, '_get_pin_config'):
            return

        config_path = self.__resolve_gpio_pinout_config_path()
        if not config_path:
            raise RuntimeError('No GPIO pin definition file found. Set GPIO_CONFIG_PATH or provide config_files/gpio_pinout_<board>.json')

        os.environ['GPIO_CONFIG_PATH'] = config_path
        with open(config_path, 'r') as gpio_config_file:
            print(f'Loading GPIO pin definition file: {config_path}')
            self.gpio_pinout_config = json.load(gpio_config_file)

        if hasattr(gpio, '_pin_config'):
            gpio._pin_config = None
        gpio._get_pin_config()

    def __resolve_gpio_pinout_config_path(self) -> str | None:
        candidates: list[str] = []

        env_pinout = os.environ.get('GPIO_CONFIG_PATH')
        if env_pinout:
            candidates.append(env_pinout)

        for key in ['gpio_config_path', 'gpio_pinout_path', 'gpio_pinout']:
            configured_path = self.pinout_config.get(key)
            if configured_path:
                candidates.append(configured_path)

        board_name = os.environ.get('BOARD_NAME')
        if board_name:
            candidates.append(f'config_files/gpio_pinout_{board_name}.json')
            candidates.append(f'pinouts/gpio_pinout_{board_name}.json')

        candidates.append('config_files/gpio_pinout.json')
        candidates.append('pinouts/gpio_pinout.json')

        for candidate in candidates:
            if os.path.exists(candidate):
                return candidate

        return None

    def __resolve_pinout_config_path(self) -> str:
        available_pinouts = self.__scan_pinout_files()
        compatible_pinouts = [
            entry for entry in available_pinouts
            if self.__is_pinout_compatible(entry['metadata'])
        ]

        preferred_candidates = []
        if self.config.pinout_file:
            preferred_candidates.append(self.config.pinout_file)
        preferred_candidates.append(pinout_config_path)
        preferred_candidates.append(os.path.basename(pinout_config_path))
        env_pinout = os.environ.get('PINOUT_CONFIG_PATH')
        if env_pinout:
            preferred_candidates.append(env_pinout)
            preferred_candidates.append(os.path.basename(env_pinout))

        for candidate in preferred_candidates:
            selected = self.__find_pinout_candidate(candidate, compatible_pinouts)
            if selected:
                return selected

        if len(compatible_pinouts) > 0:
            return compatible_pinouts[0]['path']

        if os.path.exists(pinout_config_path):
            return pinout_config_path

        raise RuntimeError('No compatible pinout configuration file found')

    def __scan_pinout_files(self) -> list[dict]:
        pinout_files: dict[str, dict] = {}
        for root in self.__pinout_roots():
            if not os.path.exists(root):
                continue

            try:
                names = os.listdir(root)
            except Exception:
                continue

            for name in names:
                if not self.__is_pinout_file_name(name):
                    continue

                path = self.__join_path(root, name)
                metadata = self.__read_json_file(path)
                if metadata is None:
                    continue

                if name not in pinout_files:
                    pinout_files[name] = {
                        'file': name,
                        'path': path,
                        'metadata': metadata,
                    }

        return list(pinout_files.values())

    def __pinout_roots(self) -> list[str]:
        roots = []
        default_dir = os.path.dirname(pinout_config_path)
        if default_dir:
            roots.append(default_dir)

        if sys.implementation.name == 'cpython':
            roots.append('config_files')
            roots.append('pinouts')
        else:
            roots.append('/')

        unique_roots = []
        for root in roots:
            if root not in unique_roots:
                unique_roots.append(root)
        return unique_roots

    def __find_pinout_candidate(self, candidate: str, pinout_files: list[dict]) -> str | None:
        candidate_basename = os.path.basename(candidate)
        for entry in pinout_files:
            if entry['file'] == candidate_basename or entry['path'] == candidate:
                return entry['path']
        return None

    def __is_pinout_file_name(self, file_name: str) -> bool:
        if not file_name.endswith('.json'):
            return False
        return file_name == 'pinout.json' or file_name.startswith('pinout_')

    def __join_path(self, root: str, file_name: str) -> str:
        if root == '/':
            return f'/{file_name}'
        return os.path.join(root, file_name)

    def __read_json_file(self, path: str) -> dict | None:
        try:
            with open(path, 'r') as file:
                parsed = json.load(file)
                return parsed if isinstance(parsed, dict) else None
        except Exception:
            return None

    def __is_pinout_compatible(self, metadata: dict) -> bool:
        target_hardware, target_variant = self.__runtime_hardware_profile()
        file_hardware = str(metadata.get('hardware', '')).lower().strip()
        file_variant = str(metadata.get('variant', '')).lower().strip()

        if not file_hardware:
            return True

        if target_hardware and file_hardware != target_hardware:
            return False

        if target_variant and file_variant and file_variant not in [target_variant, 'any']:
            return False

        return True

    def __runtime_hardware_profile(self) -> tuple[str, str]:
        if sys.implementation.name == 'micropython' and sys.platform == 'esp32':
            target_variant = ''
            default_file = os.path.basename(pinout_config_path)
            if default_file.startswith('pinout_esp32_') and default_file.endswith('.json'):
                target_variant = default_file[len('pinout_esp32_'):-len('.json')]
            return 'esp32', target_variant

        hardware = os.environ.get('BOARD_HARDWARE', '').lower().strip()
        variant = os.environ.get('BOARD_VARIANT', '').lower().strip()

        if not hardware:
            hardware = sys.implementation.name.lower().strip()

        return hardware, variant
