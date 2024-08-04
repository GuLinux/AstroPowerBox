const fetchJson = async (path, init) => {
    const response = await fetch(path, init)
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

export const fetchHeaters = async () => await fetchJson('/api/heaters')
export const fetchConfig = async () => await fetchJson('/api/config')
export const saveConfig = async () => await fetchJson('/api/config/write', { method: 'POST'})
export const saveWiFiAccessPointConfig = async payload => await payloadJson('/api/config/accessPoint', 'POST', payload)
export const saveWiFiStationConfig = async payload => await payloadJson('/api/config/station', 'POST', payload)
export const removeWiFiStationConfig = async payload => await payloadJson('/api/config/station', 'DELETE', payload)
export const fetchAppInfo = async () => await fetchJson('/api/info')
export const fetchRestart = async () => await fetchJson('/api/restart', { method: 'POST' })
export const fetchReconnectWiFi = async () => await fetchJson('/api/wifi/connect', { method: 'POST' })
export const setHeater = async (index, heater) => await payloadJson('/api/heater', 'POST', {index, ...heater})
