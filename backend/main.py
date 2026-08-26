from microdot import Microdot, send_file
from microdot.sse import with_sse
import logging
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

@app.route('/')
async def index(request):
    return send_file('/static/index.html', compressed=True, file_extension='.gz')

@app.route('/assets/<path:path>')
async def asset(request, path):
    return send_file('/static/assets/' + path, compressed=True, file_extension='.gz')

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
    await sse.send(board.pin_status_snapshot(), event='pins')
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
