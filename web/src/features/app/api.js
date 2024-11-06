const fetchJson = async (path, init) => {
    let url = path;
    // console.log(process.env)
    if(process.env.NODE_ENV === 'development' && 'REACT_APP_ASTROPOWERBOX_API_HOST' in process.env) {
        url = `${process.env.REACT_APP_ASTROPOWERBOX_API_HOST}${path}`;
    }
    const response = await fetch(url, init)
    return await response.json();
}
const payloadJson = async (path, method, payload) => {
    return await fetchJson(path, {
        method,
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(payload)
    })
}

export const fetchHistory = async () => await fetchJson('/api/history')
export const fetchHeaters = async () => await fetchJson('/api/heaters')
export const fetchConfig = async () => await fetchJson('/api/config')
export const fetchStatus = async () => await fetchJson('/api/status')
export const saveConfig = async () => await fetchJson('/api/config/write', { method: 'POST'})
export const saveWiFiAccessPointConfig = async payload => await payloadJson('/api/config/accessPoint', 'POST', payload)
export const saveWiFiStationConfig = async payload => await payloadJson('/api/config/station', 'POST', payload)
export const removeWiFiStationConfig = async payload => await payloadJson('/api/config/station', 'DELETE', payload)
export const fetchAppInfo = async () => await fetchJson('/api/info')
export const fetchRestart = async () => await fetchJson('/api/restart', { method: 'POST' })
export const fetchReconnectWiFi = async () => await fetchJson('/api/wifi/connect', { method: 'POST' })
export const setHeater = async (index, heater) => await payloadJson('/api/heater', 'POST', {index, ...heater})
export const setStatusLedDuty = async payload => await payloadJson('/api/config/statusLedDuty', 'POST', payload)
export const setPowerSourceType = async payload => await payloadJson('/api/config/powerSourceType', 'POST', payload)