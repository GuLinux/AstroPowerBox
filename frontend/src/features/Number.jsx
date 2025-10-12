export const formatDecimals = (value, decimals, unit) => `${value.toFixed(decimals)}${unit}`
export const formatPercentage = (value, decimals, unit) => `${(value * 100.0).toFixed(decimals)}${ unit != '' ? unit : '%'}`
export const formatSize = (value, decimals) => {
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    if(value === 0) {
        return '0 B'
    }
    const converted = units.map((unit, index) => ({ unitValue: value/Math.pow(1024, index), unit}))
        .filter((v, index) => v.unitValue >= 0.99 && (v.unitValue < 1024 || index === units.length-1) )[0]
    return `${converted.unitValue.toFixed(decimals)} ${converted.unit}`
}

export const formatTime = (value) => {
    const seconds = String(parseInt(value % 60)).padStart(2, '0'); value /= 60;
    const minutes = String(parseInt(value % 60)).padStart(2, '0'); value /= 60;
    const hours = String(parseInt(value % 24)).padStart(2, '0'); value /= 24;
    const days = parseInt(value);
    let formattedTime = `${hours}:${minutes}:${seconds}`;
    if (days > 0) {
        formattedTime = `${days} days, ${formattedTime}`
    }
    return formattedTime;
}
export const Number = ({value, notAvailableValue="N/A", decimals=2, unit="", formatFunction = formatDecimals}) => {
    if(value === null || value === undefined) {
        return notAvailableValue;
    }
    return formatFunction(value, decimals, unit);
}