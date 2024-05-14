
const fetchJson = async (path, init) => {
    const response = await fetch(path, init)
    return await response.json();
}

const queryHeaters = async () => {
    const heaters = await fetchJson('/api/heaters')
    console.log(heaters);
}

const queryAmbient = async () => {
    const ambient = await fetchJson('/api/ambient')
    document.getElementById('ambient_temperature').textContent = `${ambient.temperature.toFixed(2)}°C`;
    document.getElementById('ambient_humidity').textContent = `${ambient.humidity.toFixed(2)}%`;
    document.getElementById('ambient_dewpoint').textContent = `${ambient.dewpoint.toFixed(2)}°C`;
}


window.addEventListener('load', () => {
    console.log("window loaded")
    window.setInterval(queryHeaters, 1000)
    window.setInterval(queryAmbient, 1000)
})