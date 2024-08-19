import Table from 'react-bootstrap/Table';
import Accordion from 'react-bootstrap/Accordion';
import { useSelector } from 'react-redux';
import { selectAmbient, selectAmbientHistory } from './ambientSlice';
import { Number } from '../../Number';
import { LineChart, CartesianGrid, XAxis, YAxis, Legend, Line, ResponsiveContainer } from 'recharts'
import { useEffect, useId, useState } from 'react';

const AmbientChart = ({}) => {
    const id = useId();
    const history = useSelector(selectAmbientHistory)
    const [selected, setSelected] = useState(null);
    const shouldShow = key => selected === null || selected === key;
    return <div style={{ height: 400 }}>
            <ResponsiveContainer>
            <LineChart width={600} height={300} data={history}>
                <CartesianGrid stroke="#ccc" />
                <XAxis dataKey="name" />
                <YAxis yAxisId={`tempYaxis-${id}`} />
                <YAxis yAxisId={`humidityYaxis-${id}`} orientation='right' />
                <Legend onClick={({dataKey}) => setSelected(selected !== dataKey ? dataKey : null)}/>
                { shouldShow('temperature') && <Line yAxisId={`tempYaxis-${id}`} type="monotone" dataKey="temperature" stroke="#8884d8" /> }
                { shouldShow('humidity') && <Line yAxisId={`humidityYaxis-${id}`} type="monotone" dataKey="humidity" stroke="#2284d8" /> }
                { shouldShow('dewpoint') && <Line yAxisId={`tempYaxis-${id}`} type="monotone" dataKey="dewpoint" stroke="#8822d8" /> }

            </LineChart>
        </ResponsiveContainer>
    </div>
}
export const Ambient = () => {
    const { dewpoint, humidity, temperature } = useSelector(selectAmbient);
    return <>
    <Table>
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
        <Accordion>
            <Accordion.Item eventKey='0'>
                <Accordion.Header>History</Accordion.Header>
                <Accordion.Body>
                    <AmbientChart temperature={temperature} />
                </Accordion.Body>
            </Accordion.Item>
        </Accordion>
    </>
}