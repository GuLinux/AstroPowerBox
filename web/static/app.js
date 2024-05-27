
const queryAmbient = async () => {
    const ambient = await fetchJson('/api/ambient')
    window.ambient = ambient
    document.getElementById('ambient_temperature').textContent = `${ambient.temperature.toFixed(2)}°C`;
    document.getElementById('ambient_humidity').textContent = `${ambient.humidity.toFixed(2)}%`;
    document.getElementById('ambient_dewpoint').textContent = `${ambient.dewpoint.toFixed(2)}°C`;
}

const queryPower = async () => {
    const power = await fetchJson('/api/power')
    window.power = power
    document.getElementById('power_voltage').textContent = `${power.busVoltage.toFixed(2)}V`;
    document.getElementById('power_current').textContent = `${power.current.toFixed(2)}A`;
    document.getElementById('power_watts').textContent = `${power.power.toFixed(2)}°W`;
}


window.addEventListener('load', () => {
    setupHeaters();
    window.setInterval(queryAmbient, 1000)
    window.setInterval(queryPower, 1000)
})
