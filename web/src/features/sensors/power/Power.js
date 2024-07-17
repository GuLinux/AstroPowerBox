import Table from 'react-bootstrap/Table';
import { useSelector } from 'react-redux';
import { Number } from '../../Number';
import { selectPower } from './powerSlice';

export const Power = () => {
    const { busVoltage, current, power, shuntVoltate } = useSelector(selectPower);
    return <Table>
        <tbody>
        <tr>
            <th scope="row">Voltage</th>
            <td><Number value={busVoltage} unit="V"/></td>
        </tr>
        <tr>
            <th scope="row">Current</th>
            <td><Number value={current} unit="A"/></td>
        </tr>
        <tr>
            <th scope="row">Power</th>
            <td><Number value={power} unit="W"/></td>
        </tr>
        </tbody>
    </Table>
}