import { useSelector } from "react-redux"
import { Ambient } from "./sensors/ambient/Ambient"
import { Heaters } from "./sensors/heaters/Heaters"
import { Power } from "./sensors/power/Power"
import { selectHeatersCount } from "./sensors/heaters/heatersSlice"
import { appStatusSelector } from "./app/appSlice"

export const Home = () => {
    const heatersCount = useSelector(selectHeatersCount);
    const hasAmbient = useSelector(appStatusSelector).hasAmbientSensor
    return <>
        { hasAmbient && <>
            <h2 className="mb-5">Environment</h2>
            <Ambient />
        </> }
        { heatersCount > 0 && <>
            <h2 className="mb-5">Heaters</h2>
            <Heaters />
        </>
        }
        <h2 className="mb-5">Power</h2>
        <Power />
    </>
}