import { useSelector } from "react-redux"
import { Ambient } from "./sensors/ambient/Ambient"
import { PWMOutputs } from "./sensors/pwmOutputs/PWMOutputs"
import { Power } from "./sensors/power/Power"
import { selectPWMOutputsByType } from "./sensors/pwmOutputs/pwmOutputsSlice"
import { appStatusSelector } from "./app/appSlice"

export const Home = () => {
    const hasAmbient = useSelector(appStatusSelector).hasAmbientSensor

    const heaters = useSelector(state => selectPWMOutputsByType(state, 'heater'))
    const outputs = useSelector(state => selectPWMOutputsByType(state, 'output'))

    return <>
        { hasAmbient && <>
            <h2 className="mb-5">Environment</h2>
            <Ambient />
        </> }
        { heaters.length > 0 && <>
            <h2 className="mb-5 mt-5">Dew Heaters</h2>
            <PWMOutputs type='heater' />
        </>
        }
        { outputs.length > 0 && <>
            <h2 className="mb-5 mt-5">Outputs</h2>
            <PWMOutputs type='output' />
        </>
        }

        <h2 className="mb-5 mt-5">Power</h2>
        <Power />
    </>
}