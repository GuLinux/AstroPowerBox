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

export const fetchHeaters = async () => {
    return await fetchJson('/api/heaters')
}

export const setHeater = async (index, heater) => {
    const payload = {index, ...heater}
    console.log(payload)
    return await payloadJson('/api/heater', 'POST', payload)
}