import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BACKEND_MAIN = ROOT / 'backend' / 'main.py'
FRONTEND_API = ROOT / 'frontend' / 'src' / 'features' / 'app' / 'api.jsx'
FRONTEND_APP = ROOT / 'frontend' / 'src' / 'App.jsx'


def _parse_backend_routes() -> dict[str, set[str]]:
    text = BACKEND_MAIN.read_text()
    routes: dict[str, set[str]] = {}

    for path, methods_group in re.findall(r"@app\.route\('([^']+)'(?:, methods=\[([^\]]+)\])?\)", text):
        if methods_group:
            methods = set(re.findall(r"'([A-Z]+)'", methods_group))
        else:
            methods = {'GET'}
        routes.setdefault(path, set()).update(methods)

    return routes


def _parse_frontend_calls() -> dict[str, tuple[str, str]]:
    text = FRONTEND_API.read_text()
    calls: dict[str, tuple[str, str]] = {}

    fetch_pattern = re.compile(
        r"export const (\w+) = async [^=]*=> await fetchJson\('([^']+)'(?:,\s*\{\s*method:\s*'([A-Z]+)'\s*\})?\)"
    )
    payload_pattern = re.compile(
        r"export const (\w+) = async [^=]*=> await payloadJson\('([^']+)',\s*'([A-Z]+)'"
    )

    for name, path, method in fetch_pattern.findall(text):
        calls[name] = (path, method or 'GET')
    for name, path, method in payload_pattern.findall(text):
        calls[name] = (path, method)

    return calls


def test_frontend_implemented_api_calls_match_backend_routes():
    backend_routes = _parse_backend_routes()
    frontend_calls = _parse_frontend_calls()

    implemented_contract = {
        'fetchConfig': ('/api/config', 'GET'),
        'saveConfig': ('/api/config/write', 'POST'),
        'saveWiFiAccessPointConfig': ('/api/config/wifi/accessPoint', 'POST'),
        'saveWiFiStationConfig': ('/api/config/wifi/station', 'POST'),
        'removeWiFiStationConfig': ('/api/config/wifi/station', 'DELETE'),
        'setStatusLedDuty': ('/api/config/statusLedDuty', 'POST'),
        'fetchPinoutConfig': ('/api/config/pinout', 'GET'),
        'fetchPinoutFiles': ('/api/config/pinouts', 'GET'),
        'setPinoutConfig': ('/api/config/pinout', 'POST'),
    }

    for call_name, expected in implemented_contract.items():
        assert call_name in frontend_calls, f'Missing frontend API call: {call_name}'
        assert frontend_calls[call_name] == expected, f'Frontend API call mismatch for {call_name}'

        route_path, route_method = expected
        assert route_path in backend_routes, f'Missing backend route: {route_path}'
        assert route_method in backend_routes[route_path], f'Backend route method mismatch for {route_path}'


def test_frontend_sse_contract_matches_backend_event_name():
    backend_text = BACKEND_MAIN.read_text()
    frontend_text = FRONTEND_APP.read_text()

    assert "EventSource('/api/events')" in frontend_text
    assert "addEventListener('pins'" in frontend_text

    assert "sse_broadcaster.publish('pins'" in backend_text
    assert "await sse.send(board.pin_event_snapshot(), event='pins')" in backend_text
