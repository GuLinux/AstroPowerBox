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

const cloneTo = (sourceSelector, destinationSelector) => {
    const source = document.querySelector(sourceSelector);
    const dest = document.querySelector(destinationSelector);
    return dest.appendChild(source.cloneNode(true));
}

const setCollapse = (elementId, visible=false) => {
    const collapse = bootstrap.Collapse.getOrCreateInstance(document.getElementById(elementId))
    if(visible) {
        collapse.show();
    } else {
        collapse.hide();
    }
}


const syncRangeLabel = (rangeElement, labelElement, formatFunction = v => `${v}`) => {
    const value = parseFloat(document.querySelector(rangeElement).value)
    document.querySelector(labelElement).textContent = formatFunction(value)
    return value;
}