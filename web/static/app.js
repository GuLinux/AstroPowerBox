
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

const HeaterModes = {
    off: 'Off',
    fixed: 'Fixed',
    target_temperature: 'Target Temperature',
    dewpoint: 'Dewpoint',
}

const onHeatersResponse = heaters => {
    const formatTargetTemperature = heater => {
        if(heater.mode === 'target_temperature')
            return `${heater.target_temperature.toFixed(2)}°C`

        if(heater.mode === 'dewpoint') {
            sign = heater.dewpoint_offset < 0 ? '' : '+'
            const dewpointOffsetString = `dewpoint ${sign}${heater.dewpoint_offset}°C`
            return window.ambient && window.ambient.dewpoint ? `${(heater.dewpoint_offset + window.ambient.dewpoint).toFixed(2)}°C (${dewpointOffsetString})` : dewpointOffsetString
        }
        return 'N/A'
    }
    heaters.forEach((heater, index) => {
        document.getElementById(`heaters_${index+1}_mode`).textContent = HeaterModes[heater.mode]
        document.getElementById(`heaters_${index+1}_temperature`).textContent = heater.has_temperature ? `${heater.temperature.toFixed(2)}°C` : "N/A"
        document.getElementById(`heaters_${index+1}_target`).textContent = formatTargetTemperature(heater)
        document.getElementById(`heaters_${index+1}_duty`).textContent = `${(heater.duty * 100).toFixed(2)}%`
        const heaterActiveElement = document.getElementById(`heaters_${index+1}_active`);
        heaterActiveElement.classList.remove('text-bg-warning')
        heaterActiveElement.classList.remove('text-bg-success')
        heaterActiveElement.classList.add(heater.active ? 'text-bg-success' : 'text-bg-warning')
        heaterActiveElement.textContent = heater.active ? '✓' : '❌';
    });
    window.heaters = heaters
}

const queryHeaters = async () => {
    const heaters = await fetchJson('/api/heaters')
    onHeatersResponse(heaters);
}

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



const syncRangeLabel = (rangeElement, labelElement, formatFunction = v => `${v}`) => {
    const value = parseFloat(document.getElementById(rangeElement).value)
    document.getElementById(labelElement).textContent = formatFunction(value)
    return value;
}

const syncDutyValue = () => {
    const value = syncRangeLabel('setHeaterModal_duty', 'setHeaterModal_duty_valueLabel', v => `${v.toFixed(2)}%`)
    window.heater.duty = value/100.0
}

const syncTargetTemperatureValue = () => {
    const value = syncRangeLabel('setHeaterModal_target_temperature', 'setHeaterModal_target_temperature_valueLabel', v => `${v.toFixed(2)}°C`)
    window.heater.target_temperature = value
}

const syncDewpointOffsetValue = () => {
    const value = syncRangeLabel('setHeaterModal_dewpoint_offset', 'setHeaterModal_dewpoint_offset_valueLabel', v => `${v.toFixed(2)}°C`)
    window.heater.dewpoint_offset = value
}

const setCollapse = (elementId, visible=false) => {
    const collapse = bootstrap.Collapse.getOrCreateInstance(document.getElementById(elementId))
    if(visible) {
        collapse.show();
    } else {
        collapse.hide();
    }
}

const syncHeaterModal = () => {
    const heater = window.heater;
    document.getElementById('setHeaterModal_mode').value = heater.mode;
    document.getElementById('setHeaterModal_duty').value = heater.duty*100;
    document.getElementById('setHeaterModal_target_temperature').value = heater.target_temperature;
    document.getElementById('setHeaterModal_dewpoint_offset').value = heater.dewpoint_offset;

    document.getElementById('setHeaterModal_mode').children[2].disabled = !heater.has_temperature;
    document.getElementById('setHeaterModal_mode').children[3].disabled = !heater.has_temperature;

    setCollapse('setHeaterModal_duty_container', heater.mode !== 'off')
    setCollapse('setHeaterModal_target_temperature_container', heater.mode === 'target_temperature')
    setCollapse('setHeaterModal_dewpoint_container', heater.mode === 'dewpoint')
    syncDutyValue();
    syncTargetTemperatureValue();
    syncDewpointOffsetValue();
}

const addHeaterModalListener = () => {
    ['setHeaterModal_duty_container', 'setHeaterModal_target_temperature_container', 'setHeaterModal_dewpoint_container'].forEach(c => setCollapse(c, false));
    setCollapse('setHeaterModal_duty_container', false)

    const heatersModal = document.getElementById('setHeaterModal')
    heatersModal.addEventListener('show.bs.modal', ({relatedTarget}) => {
        const index = parseInt(relatedTarget.getAttribute('data-bs-index'))
        window.heater = {...window.heaters[index], index}
        syncHeaterModal();
    })

    document.getElementById('setHeaterModal_mode').addEventListener('change', ({ target: {value} })=>  {
        if(value !== 'off' && window.heater.mode === 'off' && window.heater.duty === 0) {
            window.heater.duty = 1;
        }
        window.heater.mode = value;
        syncHeaterModal();
    });

    document.getElementById('setHeaterModal_duty').addEventListener('change', event =>  syncDutyValue());
    document.getElementById('setHeaterModal_duty').addEventListener('input', event => syncDutyValue()); 
    document.getElementById('setHeaterModal_target_temperature').addEventListener('change', event => syncTargetTemperatureValue());
    document.getElementById('setHeaterModal_target_temperature').addEventListener('input', event => syncTargetTemperatureValue()); 
    document.getElementById('setHeaterModal_dewpoint_offset').addEventListener('change', event => syncDewpointOffsetValue());
    document.getElementById('setHeaterModal_dewpoint_offset').addEventListener('input', event => syncDewpointOffsetValue()); 

    document.getElementById('setHeaterModal_apply').addEventListener('click', async event => {
        console.log(window.heater)
        const response = await payloadJson('/api/heater', 'POST', window.heater)
        console.log(response)
        onHeatersResponse(response);
        bootstrap.Modal.getInstance(document.getElementById('setHeaterModal')).hide()
        
    })
}

window.addEventListener('load', () => {
    window.setInterval(queryHeaters, 1000)
    window.setInterval(queryAmbient, 1000)
    window.setInterval(queryPower, 1000)

    addHeaterModalListener();
})
