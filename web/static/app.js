
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
    set_temperature: 'Set to Temperature',
}

const onHeatersResponse = heaters => {
    heaters.forEach((heater, index) => {
        document.getElementById(`heaters_${index+1}_mode`).textContent = HeaterModes[heater.mode]
        document.getElementById(`heaters_${index+1}_temperature`).textContent = heater.has_temperature ? `${heater.temperature.toFixed(2)}°C` : "N/A"
        document.getElementById(`heaters_${index+1}_target`).textContent = heater.target_temperature? `${heater.target_temperature.toFixed(2)}°C` : "N/A"
        document.getElementById(`heaters_${index+1}_duty`).textContent = `${(heater.pwm * 100).toFixed(2)}%`
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


const syncRangeLabel = (rangeElement, labelElement, formatFunction = v => `${v}`) => {
    document.getElementById(labelElement).textContent = formatFunction(parseFloat(document.getElementById(rangeElement).value))
}

const setCollapse = (elementId, visible) => {
    const collapse = bootstrap.Collapse.getOrCreateInstance(document.getElementById(elementId))
    if(visible) {
        collapse.show();
    } else {
        collapse.hide();
    }
}

const addHeaterModalListener = () => {
    const syncPwmValueLabel = () => syncRangeLabel('setHeaterModal_pwm', 'setHeaterModal_pwm_valueLabel', v => `${v.toFixed(2)}%`)
    const toggleContainersModeCollapse = mode => {
        setCollapse('setHeaterModal_pwm_container', mode !== 'off')
    }
    setCollapse('setHeaterModal_pwm_container', false)

    const heatersModal = document.getElementById('setHeaterModal')
    heatersModal.addEventListener('show.bs.modal', event => {
        const button = event.relatedTarget
        const index = button.getAttribute('data-bs-index')
        const heater = window.heaters[index];
        document.getElementById('setHeaterModal_index').value = index;
        document.getElementById('setHeaterModal_mode').value = heater.mode;
        document.getElementById('setHeaterModal_pwm').value = heater.pwm*100;
        toggleContainersModeCollapse(heater.mode);

        // document.getElementById('setHeaterModal_pwm').disabled = heater.mode === 'off';
        document.getElementById('setHeaterModal_mode').children[2].disabled = !heater.has_temperature;
        syncPwmValueLabel()
    })

    document.getElementById('setHeaterModal_mode').addEventListener('change', event =>  {
        const value = event.target.value;
        toggleContainersModeCollapse(value)
        // document.getElementById('setHeaterModal_pwm').disabled = value === 'off';
        if(value !== 'off') {
            document.getElementById('setHeaterModal_pwm').value = 100;
        }
        syncPwmValueLabel();
    });
    document.getElementById('setHeaterModal_pwm').addEventListener('change', event =>  syncPwmValueLabel());
    document.getElementById('setHeaterModal_pwm').addEventListener('input', event => syncPwmValueLabel()); 
    document.getElementById('setHeaterModal_apply').addEventListener('click', async event => {
        const payload = {
            index: parseInt(document.getElementById('setHeaterModal_index').value),
            mode: document.getElementById('setHeaterModal_mode').value
        }
        if(payload.mode !== 'off') {
            payload.duty = parseFloat(document.getElementById('setHeaterModal_pwm').value)/100;
        }
        // console.log(payload)
        const response = await payloadJson('/api/heater', 'POST', payload)
        onHeatersResponse(response);
        bootstrap.Modal.getInstance(document.getElementById('setHeaterModal')).hide()
        // console.log(response)
    })
}

window.addEventListener('load', () => {
    console.log("window loaded")
    window.setInterval(queryHeaters, 1000)
    window.setInterval(queryAmbient, 1000)

    addHeaterModalListener();
})
