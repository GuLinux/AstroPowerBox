export const formatDecimals = (value, digits) => value.toFixed(digits)
export const formatPercentage = (value, digits) => (value * 100.0).toFixed(digits)

export const Number = ({value, notAvailableValue="N/A", decimals=2, unit="", formatFunction = formatDecimals}) => {
    if(value === null || value === undefined) {
        return notAvailableValue;
    }
    return `${formatFunction(value, decimals)}${unit}`;
}