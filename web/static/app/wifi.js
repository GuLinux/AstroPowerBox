

const setupWiFi = async () => {
    const config = await fetchJson('/api/config');
    const wifiStatus = await fetchJson('/api/wifi');
    config.stations.forEach((station, index) => {
        const wifiConfigDom = cloneTo('#templates > table tr.wifi_row', '#wifi_tbody')
        wifiConfigDom.querySelector('.wifi_number').textContent = `${index+1}`
        wifiConfigDom.querySelector('.wifi_essid').textContent = station.essid
        const connectedElement = wifiConfigDom.querySelector('.wifi_connected');
        const connected = station.essid === wifiStatus.wifi.essid
        connectedElement.classList.remove('text-bg-secondary')
        connectedElement.classList.remove('text-bg-success')
        connectedElement.classList.add(connected ? 'text-bg-success' : 'text-bg-secondary')
        connectedElement.textContent = connected ? '✓' : '❌';

    })
    console.log(wifiStatus)
}