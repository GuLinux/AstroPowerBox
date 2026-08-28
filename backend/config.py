from utils import check_required_keys
from protocols.config_storage import ConfigStorage
import protocols.config
from wifi import WiFi

class Config(protocols.config.Config):
    _status_led_duty: float
    _fan_duty: float
    _ap: WiFi
    _stations: list[WiFi]
    _pinout_file: str
    _pwm_output_startup: dict[str, dict]

    def __init__(self, storage: ConfigStorage):
        self.storage = storage
        self.load()

    @property
    def ap(self) -> WiFi:
        return self._ap

    @ap.setter
    def ap(self, ap: WiFi) -> None:
        self._ap = ap

    @property
    def stations(self) -> list[WiFi]:
        return self._stations

    @stations.setter
    def stations(self, stations: list[WiFi]) -> None:
        self._stations = stations

    @property
    def status_led_duty(self) -> float:
        return self._status_led_duty
    
    @status_led_duty.setter
    def status_led_duty(self, duty: float) -> None:
        self._status_led_duty = duty

    @property
    def fan_duty(self) -> float:
        return self._fan_duty

    @fan_duty.setter
    def fan_duty(self, duty: float) -> None:
        self._fan_duty = duty

    @property
    def pinout_file(self) -> str:
        return self._pinout_file

    @pinout_file.setter
    def pinout_file(self, pinout_file: str) -> None:
        self._pinout_file = pinout_file

    @property
    def pwm_output_startup(self) -> dict[str, dict]:
        return self._pwm_output_startup

    @pwm_output_startup.setter
    def pwm_output_startup(self, pwm_output_startup: dict[str, dict]) -> None:
        self._pwm_output_startup = pwm_output_startup

    @property
    def json(self) -> dict:
        return {
            'ap': self.ap.json if self.ap else None,
            'stations': WiFi.to_json_list(self._stations),
            'statusLedDuty': self._status_led_duty,
            'fanDuty': self._fan_duty,
            'pinoutFile': self._pinout_file,
            'pwmOutputStartup': self._pwm_output_startup,
        }

    def load(self) -> None:
        self._ap, self._stations = self.storage.load_wifi()
        self._status_led_duty = self.storage.load_float('stLedDuty', default=1) or 1.0
        self._fan_duty = self.storage.load_float('fanDuty', default=1) or 1.0
        self._pinout_file = self.storage.load_str('pinoutFile', default='') or ''
        startup_raw = self.storage.load_json('pwmOutputStartup', default={}) or {}
        self._pwm_output_startup = {}
        if isinstance(startup_raw, dict):
            for pin_id, profile in startup_raw.items():
                if isinstance(profile, dict):
                    self._pwm_output_startup[str(pin_id)] = dict(profile)
                else:
                    # Backward compatibility: old format stored duty directly.
                    try:
                        duty = float(profile)
                    except (TypeError, ValueError):
                        continue
                    self._pwm_output_startup[str(pin_id)] = {
                        'mode': 'fixed',
                        'max_duty': duty,
                        'min_duty': 0.0,
                        'target_temperature': None,
                        'dewpoint_offset': None,
                        'ramp_offset': 0.0,
                    }

    def save(self) -> None:
        self.storage.save_wifi(self._stations, self._ap)
        self.storage.save_float('stLedDuty', self._status_led_duty)
        self.storage.save_float('fanDuty', self._fan_duty)
        self.storage.save_str('pinoutFile', self._pinout_file)
        self.storage.save_json('pwmOutputStartup', self._pwm_output_startup)

    def __str__(self):
        return f'Config{self.json}'

    def __repr__(self):
        return self.__str__()