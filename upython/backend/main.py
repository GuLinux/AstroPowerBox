from microdot import Microdot, send_file
import os
from board_compat import asyncio, server_port, server_debug
from board import Board
from config import WiFi

board = Board()
app = Microdot()

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

async def main():
    await board.start()
    await board.wifi_manager.connect_stations()
    await app.start_server(port=server_port, debug=server_debug)

asyncio.run(main())
