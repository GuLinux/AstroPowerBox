const HeaterModes = {
    off: 'Off',
    fixed: 'Fixed',
    target_temperature: 'Target Temperature',
    dewpoint: 'Dewpoint',
}

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

const updateHeater = (heater, heaterDom)  => {
    heaterDom.querySelector('.heater_mode').textContent = HeaterModes[heater.mode]
    heaterDom.querySelector('.heater_temperature').textContent = heater.has_temperature ? `${heater.temperature.toFixed(2)}°C` : "N/A"
    heaterDom.querySelector('.heater_target').textContent = formatTargetTemperature(heater)
    heaterDom.querySelector('.heater_duty').textContent = `${(heater.duty * 100).toFixed(2)}%`
    const heaterActiveElement = heaterDom.querySelector('.heater_active');
    heaterActiveElement.classList.remove('text-bg-secondary')
    heaterActiveElement.classList.remove('text-bg-success')
    heaterActiveElement.classList.add(heater.active ? 'text-bg-success' : 'text-bg-secondary')
    heaterActiveElement.textContent = heater.active ? '✓' : '❌';
}

const onHeatersResponse = heaters => {
    window.heaters = heaters;
    window.onHeatersUpdateListeners.forEach(listener => listener(heaters))
}


const setupHeaters = async () => {
    const heaters = await fetchJson('/api/heaters')
    window.onHeatersUpdateListeners = [];
    heaters.forEach((heater, index) => {
        const heaterDom = cloneTo('#templates > table tr.heater_row', '#heaters_tbody')
        heaterDom.querySelector('.heater_number').textContent = `${index+1}`
        heaterDom.querySelector('.modal_button').setAttribute('data-bs-index', `${index}`)
        window.onHeatersUpdateListeners.push(heaters => updateHeater(heaters[index], heaterDom))
    })

    onHeatersResponse(heaters)

    window.setInterval(async () => {
        const heaters = await fetchJson('/api/heaters')
        onHeatersResponse(heaters)
    }, 1000)
    addHeaterModalListener();
}



const syncDutyValue = () => {
    const value = syncRangeLabel('#setHeaterModal_duty', '#setHeaterModal_duty_valueLabel', v => `${v.toFixed(2)}%`)
    window.heater.duty = value/100.0
}

const syncTargetTemperatureValue = () => {
    const value = syncRangeLabel('#setHeaterModal_target_temperature', '#setHeaterModal_target_temperature_valueLabel', v => `${v.toFixed(2)}°C`)
    window.heater.target_temperature = value
}

const syncDewpointOffsetValue = () => {
    const value = syncRangeLabel('#setHeaterModal_dewpoint_offset', '#setHeaterModal_dewpoint_offset_valueLabel', v => `${v.toFixed(2)}°C`)
    window.heater.dewpoint_offset = value
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

    [...document.getElementsByClassName('toggle-password-button')].forEach(btn => {
        target = document.getElementById(btn.getAttribute('data-bs-target'));
        btn.addEventListener('click', event => target.type = target.type === 'password' ? 'text' : 'password');
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
        bootstrap.Modal.getInstance(document.getElementById('setHeaterModal')).hide()
        
    })
}