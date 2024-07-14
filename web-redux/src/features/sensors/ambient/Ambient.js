import Table from 'react-bootstrap/Table';
import { useSelector } from 'react-redux';
import { selectAmbient } from './ambientSlice';
import { Number } from '../../Number';
export const Ambient = () => {
    const { dewpoint, humidity, temperature } = useSelector(selectAmbient);
    return <Table>
        <tbody>
        <tr>
            <th scope="row">Temperature</th>
            <td><Number value={temperature} unit='°C' /></td>
        </tr>
        <tr>
            <th scope="row">Humidity</th>
            <td><Number value={humidity} unit='%' /></td>
        </tr>
        <tr>
            <th scope="row">Dewpoint</th>
            <td><Number value={dewpoint} unit='°C' /></td>
        </tr>
        </tbody>
    </Table>
}