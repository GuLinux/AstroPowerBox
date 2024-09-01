import Table from 'react-bootstrap/Table';
import { useSelector } from 'react-redux';
import { Number } from '../../Number';
import { selectPower, selectPowerHistory } from './powerSlice';
import { LineChart, CartesianGrid, XAxis, YAxis, Legend, Line, ResponsiveContainer } from 'recharts'
import Accordion from 'react-bootstrap/Accordion';
import { useId, useState } from 'react';

const PowerChart= ({}) => {
    const id = useId();
    const history = useSelector(selectPowerHistory)
    const [selected, setSelected] = useState(null);
    const shouldShow = key => selected === null || selected === key;
    return <div style={{ height: 400 }}>
            <ResponsiveContainer>
            <LineChart width={600} height={300} data={history}>
                <CartesianGrid stroke="#ccc" />
                <XAxis dataKey="name" />
                <YAxis yAxisId={`voltageAxis-${id}`} />
                <YAxis yAxisId={`powerAxis-${id}`} />
                <YAxis yAxisId={`currentYAxis-${id}`} orientation='right' />
                <Legend onClick={({dataKey}) => setSelected(selected !== dataKey ? dataKey : null)}/>
                { shouldShow('busVoltage') && <Line animationDuration={100} yAxisId={`voltageAxis-${id}`} type="monotone" dataKey="busVoltage" stroke="#8884d8" /> }
                { shouldShow('current') && <Line animationDuration={100} yAxisId={`currentYAxis-${id}`} type="monotone" dataKey="current" stroke="#2284d8" /> }
                { shouldShow('power') && <Line animationDuration={100} yAxisId={`powerAxis-${id}`} type="monotone" dataKey="power" stroke="#8822d8" /> }

            </LineChart>
        </ResponsiveContainer>
    </div>
}


export const Power = () => {
    const { busVoltage, current, power } = useSelector(selectPower);
    const [historyVisible, setHistoryVisible] = useState(false);
    return <>
        <Table>
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
        <Accordion>
            <Accordion.Item eventKey='0'>
                <Accordion.Header>History</Accordion.Header>
                <Accordion.Body onEnter={() => setHistoryVisible(true)} onExited={() => setHistoryVisible(false)}>
                    { historyVisible && <PowerChart /> }
                </Accordion.Body>
            </Accordion.Item>
        </Accordion>
    </>
}