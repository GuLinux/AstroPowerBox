from microdot import Microdot, send_file
from microdot.sse import with_sse
import logging
import os
import time
from board_compat import asyncio, server_port, server_debug
from board import Board
from config import WiFi

app = Microdot()
if server_debug:
    logging.basicConfig(level=logging.DEBUG)
board = Board()


class SSEBroadcaster:
    def __init__(self):
        self._queues = []

    def subscribe(self):
        queue = asyncio.Queue(32)
        self._queues.append(queue)
        return queue

    def unsubscribe(self, queue):
        self._queues = [item for item in self._queues if item is not queue]

    def publish(self, event_name: str, payload: dict):
        message = {'event': event_name, 'data': payload}
        for queue in self._queues:
            try:
                if queue.full():
                    try:
                        queue.get_nowait()
                    except Exception:
                        pass
            except Exception:
                pass

            try:
                queue.put_nowait(message)
            except Exception:
                asyncio.create_task(queue.put(message))


sse_broadcaster = SSEBroadcaster()
board.on_pin_update(lambda update: sse_broadcaster.publish('pins', update))


def _clamp_duty(value) -> float:
    try:
        duty = float(value)
    except (TypeError, ValueError):
        return 0.0
    return max(0.0, min(1.0, duty))


def _pwm_control_pin_ids() -> list[str]:
    pin_ids = []
    for pin in board.pin_status_snapshot().get('pins', []):
        if pin.get('kind') != 'pwm':
            continue
        if pin.get('role') not in ('heater', 'output'):
            continue
        pin_id = pin.get('id')
        if pin_id in board.output_pins:
            pin_ids.append(pin_id)
    return pin_ids


def _pwm_outputs_payload() -> dict:
    pwm_outputs = []
    for pin_id in _pwm_control_pin_ids():
        state = board.pin_states.get(pin_id, {})
        pwm_outputs.append({
            'type': 'heater' if state.get('role') == 'heater' else 'output',
            'active': bool(state.get('on', False)),
            'duty': _clamp_duty(state.get('duty', 0.0)),
            'mode': 'fixed' if state.get('on', False) else 'off',
            'max_duty': _clamp_duty(state.get('duty', 0.0)),
            'min_duty': 0.0,
            'apply_at_startup': False,
            'target_temperature': None,
            'dewpoint_offset': None,
            'temperature': None,
            'has_temperature': state.get('role') == 'heater',
        })
    return {'pwmOutputs': pwm_outputs}

def try_send_file(path):
    try:
        return send_file(path, compressed=True, file_extension='.gz')
    except FileNotFoundError:
        return send_file(path)


@app.route('/')
async def index(request):
        return try_send_file('static/index.html')

@app.route('/assets/<path:path>')
async def asset(request, path):
    return try_send_file('static/assets/' + path)

@app.route('/api/config/write', methods=['POST'])
async def write_config(request):
    board.config.save()
    return board.config.json

@app.route('/api/config/wifi/station', methods=['POST', 'DELETE'])
async def set_wifi_stations(request):
    method = request.method
    index = request.json['index']
    if method == 'POST':
        if index == -1:
            board.config.stations.append(WiFi.from_json(request.json))
        else:
            board.config.stations[index] = WiFi.from_json(request.json)
    elif method == 'DELETE':
        if 0 <= index < len(board.config.stations):
            del board.config.stations[index]
    else:
        return {'error': 'Invalid method'}, 400
    return WiFi.to_json_list(board.config.stations)

@app.route('/api/config/wifi/accessPoint', methods=['POST'])
async def set_wifi_access_point(request):
    board.config.ap= WiFi.from_json(request.json)
    return board.config.ap.json

@app.route('/api/config/statusLedDuty', methods=['POST'])
async def set_status_led_duty(request):
    board.config.status_led_duty = request.json['duty']
    return board.config.json

@app.route('/api/config')
async def get_config(request):
    return board.config.json


@app.route('/api/status')
async def get_status(request):
    return {
        'has_power_monitor': False,
        'has_ambient_sensor': False,
        'status': 'ok',
    }


@app.route('/api/history')
async def get_history(request):
    return {
        'now': int(time.time()),
        'entries': [],
    }


@app.route('/api/info')
async def get_info(request):
    stat = os.statvfs('/')
    total_space = stat.f_blocks * stat.f_frsize
    free_space = stat.f_bfree * stat.f_frsize

    return {
        'mem': {
            'heapSize': 0,
            'freeHeap': 0,
            'usedHeap': 0,
            'maxAllocHeap': 0,
        },
        'sketch': {
            'totalSpace': total_space,
            'size': total_space - free_space,
            'MD5': '',
        },
        'esp': {
            'chipModel': os.uname().machine,
            'chipCores': os.cpu_count() or 1,
            'cpuFreqMHz': 0,
        },
    }


@app.route('/api/restart', methods=['POST'])
async def restart(request):
    return {'restarting': False}


@app.route('/api/wifi/connect', methods=['POST'])
async def reconnect_wifi(request):
    await board.wifi_manager.connect_stations()
    return {'ok': True}


@app.route('/api/pwmOutputs')
async def get_pwm_outputs(request):
    return _pwm_outputs_payload()


@app.route('/api/pwmOutput', methods=['POST'])
async def set_pwm_output(request):
    payload = request.json or {}

    try:
        index = int(payload.get('index', -1))
    except (TypeError, ValueError):
        return {'error': 'index must be an integer'}, 400

    pin_ids = _pwm_control_pin_ids()
    if index < 0 or index >= len(pin_ids):
        return {'error': f'Invalid pwm output index: {index}'}, 400

    pin_id = pin_ids[index]
    output_pin = board.output_pins[pin_id]

    mode = payload.get('mode')
    active = payload.get('active')
    if mode == 'off' or active is False:
        duty = 0.0
    elif 'max_duty' in payload:
        duty = _clamp_duty(payload.get('max_duty'))
    elif 'duty' in payload:
        duty = _clamp_duty(payload.get('duty'))
    else:
        duty = 1.0 if active else _clamp_duty(getattr(output_pin, 'duty', 0.0))

    if output_pin.is_pwm:
        output_pin.duty = duty
    else:
        output_pin.on = duty > 0

    return _pwm_outputs_payload()


@app.route('/api/config/fanDuty', methods=['POST'])
async def set_fan_duty(request):
    payload = request.json or {}
    return {'duty': _clamp_duty(payload.get('duty'))}


@app.route('/api/config/powerSourceType', methods=['POST'])
async def set_power_source_type(request):
    payload = request.json or {}
    return {'powerSourceType': payload.get('powerSourceType', 'AC')}


@app.route('/api/config/pinout')
async def get_pinout_config(request):
    return board.get_pinout_selection()


@app.route('/api/config/pinout', methods=['POST'])
async def set_pinout_config(request):
    payload = request.json or {}
    pinout_file = payload.get('file', '')
    try:
        return board.set_pinout_file(pinout_file)
    except ValueError as error:
        return {'error': str(error)}, 400


@app.route('/api/config/pinouts')
async def get_pinout_files(request):
    return {
        'files': board.list_available_pinout_files(),
        'current': board.get_pinout_selection(),
    }


@app.route('/api/events')
@with_sse
async def events(request, sse):
    queue = sse_broadcaster.subscribe()
    await sse.send(board.pin_event_snapshot(), event='pins')
    try:
        while True:
            message = await queue.get()
            await sse.send(message['data'], event=message['event'])
    finally:
        sse_broadcaster.unsubscribe(queue)

async def main():
    await board.start()
    await board.wifi_manager.connect_stations()
    await app.start_server(port=server_port, debug=server_debug)

if __name__ == '__main__':
    asyncio.run(main())
