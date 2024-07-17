import { Ambient } from "./sensors/ambient/Ambient"
import { Heaters } from "./sensors/heaters/Heaters"
import { Power } from "./sensors/power/Power"

export const Home = () => {
    return <>
        <h2>Environment</h2>
        <Ambient />
        <h2>Heaters</h2>
        <Heaters />
        <h2>Power</h2>
        <Power />
    </>
}