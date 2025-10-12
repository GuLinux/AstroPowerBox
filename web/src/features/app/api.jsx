class FetchError extends Error {
    constructor(message, request, response, options) {
        super(message, options);
        this.request = request;
        this.response = response;
    }
}

const fetchJson = async (path, init) => {
    const response = await fetch(path, init)
    if(!response.ok) {
        throw new FetchError(`Response failed for ${path}`, { path, init}, response)
    }
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
export const fetchPWMOutputs = async () => await fetchJson('/api/pwmOutputs')
export const fetchConfig = async () => await fetchJson('/api/config')
export const fetchStatus = async () => await fetchJson('/api/status')
export const saveConfig = async () => await fetchJson('/api/config/write', { method: 'POST'})
export const saveWiFiAccessPointConfig = async payload => await payloadJson('/api/config/accessPoint', 'POST', payload)
export const saveWiFiStationConfig = async payload => await payloadJson('/api/config/station', 'POST', payload)
export const removeWiFiStationConfig = async payload => await payloadJson('/api/config/station', 'DELETE', payload)
export const fetchAppInfo = async () => await fetchJson('/api/info')
export const fetchRestart = async () => await fetchJson('/api/restart', { method: 'POST' })
export const fetchReconnectWiFi = async () => await fetchJson('/api/wifi/connect', { method: 'POST' })
export const setPWMOutput = async (index, pwmOutput) => await payloadJson('/api/pwmOutput', 'POST', {index, ...pwmOutput})
export const setStatusLedDuty = async payload => await payloadJson('/api/config/statusLedDuty', 'POST', payload)
export const setFanDuty = async payload => await payloadJson('/api/config/fanDuty', 'POST', payload)
export const setPowerSourceType = async payload => await payloadJson('/api/config/powerSourceType', 'POST', payload)