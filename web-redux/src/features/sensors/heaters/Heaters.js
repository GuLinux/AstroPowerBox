import Table from 'react-bootstrap/Table';
import { useSelector } from 'react-redux';
import { Number, formatPercentage } from '../../Number';
import { selectHeaters } from './heatersSlice';
import { FaPowerOff, FaLightbulb } from "react-icons/fa6";


const Heater = ({heater, index}) => {
    return <tr>
        <th scope="row">{index}</th>
        <td>{heater.mode}</td>
        <td><Number value={heater.target_temperature} unit='°C'/></td>
        <td className='align-middle d-flex justify-content-start align-items-center'>
            { heater.active ? <FaLightbulb color='green'/> : <FaPowerOff color='grey' /> }
            <span className='px-2'>
                <Number value={heater.duty} formatFunction={formatPercentage} decimals={1} unit='%'/>
            </span>
            </td>
        <td><Number value={heater.temperature} unit='°C'/></td>
    </tr>
}

export const Heaters = () => {
    const { heaters } = useSelector(selectHeaters);
    return <Table>
          <thead>
            <tr>
              <th scope="col">#</th>
              <th scope="col">mode</th>
              <th scope="col">target</th>
              <th scope="col">duty</th>
              <th scope="col">temperature</th>
              <th></th>
            </tr>
          </thead>
        <tbody>
            { heaters.map((heater, index) => <Heater heater={heater} key={index} index={index} />)}
        </tbody>
    </Table>
}