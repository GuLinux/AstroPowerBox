import Table from 'react-bootstrap/Table';
import { useDispatch, useSelector } from 'react-redux';
import { Number, formatPercentage } from '../../Number';
import { selectHeaters, selectHeatersHistory, setHeaterAsync } from './heatersSlice';
import { FaPowerOff, FaLightbulb } from "react-icons/fa6";
import Button from 'react-bootstrap/Button';
import Modal from 'react-bootstrap/Modal';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import DropdownButton from 'react-bootstrap/DropdownButton';
import Dropdown from 'react-bootstrap/Dropdown';
import Form from 'react-bootstrap/Form';
import OverlayTrigger from 'react-bootstrap/OverlayTrigger';
import Tooltip from 'react-bootstrap/Tooltip';
import Badge from 'react-bootstrap/Badge';
import Collapse from 'react-bootstrap/Collapse';
import { useEffect, useState, useId } from 'react';
import { selectAmbient } from '../ambient/ambientSlice';
import { LineChart, CartesianGrid, XAxis, YAxis, Legend, Line, ResponsiveContainer } from 'recharts'
import Accordion from 'react-bootstrap/Accordion';


const HeaterModes = {
    off: 'Off',
    fixed: 'Fixed',
    target_temperature: 'Set to Temperature',
    dewpoint: 'Set to Dewpoint',
}

const HeatersChart = ({}) => {
    const id = useId();
    const history = useSelector(selectHeatersHistory)
    
    const [selected, setSelected] = useState(null);
    const [activeHeater, setActiveHeater] = useState(null);
    const shouldShow = key => selected === null || selected === key;
    if(history.length === 0) {
        return null;
    }

    const heaters = [...Array(history[history.length-1].heaters).keys()]
    return <>
        <Form>
            <Form.Group as={Row} className="justify-content-md-center">
                <Form.Label column sm={4}>
                    Select heater number to show history
                </Form.Label>
                <Col sm={4}>
                    <DropdownButton title='Heater'>
                        {heaters.map(index =>
                            <Dropdown.Item active={index === activeHeater} onClick={() => setActiveHeater(index)}>
                                Heater {index}
                            </Dropdown.Item>)}

        </DropdownButton>

                </Col>
            </Form.Group>
        </Form>//MLin
        
        {activeHeater !== null &&
        <div style={{ height: 400 }}>
                <ResponsiveContainer>
                <LineChart width={600} height={300} data={history}>
                    <CartesianGrid stroke="#ccc" />
                    <XAxis dataKey="name" />
                    <YAxis yAxisId={`tempYAxis-${id}`} />
                    <YAxis yAxisId={`dutyYAxis-${id}`} orientation='right' />
                    <Legend onClick={({dataKey}) => setSelected(selected !== dataKey ? dataKey : null)}/>
                    {shouldShow(`heater-${activeHeater}-temperature`) && <Line animationDuration={100} name={`Heater ${activeHeater} temperature`} yAxisId={`tempYAxis-${id}`} type="monotone" dataKey={`heater-${activeHeater}-temperature`} stroke="#ffaa22" />}
                    {shouldShow(`heater-${activeHeater}-duty`) && <Line animationDuration={100} name={`Heater ${activeHeater} duty`} yAxisId={`dutyYAxis-${id}`} type="monotone" dataKey={`heater-${activeHeater}-duty`} stroke="#22aaff" />}
                </LineChart>
            </ResponsiveContainer>
        </div>
        }
    </>
}

const HeaterMode = ({mode}) => <option value={mode}>{HeaterModes[mode]}</option>

const SetHeaterModal = ({heater: originalHeater, show, onClose, index}) => {
    const [heater, setHeater] = useState({target_temperature: 25, dewpoint_offset: 5, ...originalHeater, duty: originalHeater.duty === 0 ? 1 : originalHeater.duty});
    const updateHeater = (field, parser = value => value) => event => setHeater({...heater, [field]: parser(event.target.value) })
    const dispatch = useDispatch()
    const onUpdateClicked = () => {
        dispatch(setHeaterAsync({index, heater}));
        onClose();
    }
    return <Modal show={show} onHide={onClose}>
        <Modal.Header closeButton>
          <Modal.Title>Set Heater {index+1}</Modal.Title>
        </Modal.Header>
        <Modal.Body>
            <Form>
                <Form.Group className='mb-3'>
                    <Form.Label>Mode</Form.Label>
                    <Form.Select value={heater.mode} onChange={updateHeater('mode')}>
                        <HeaterMode mode='off' />
                        <HeaterMode mode='fixed' />
                        { originalHeater.has_temperature && <HeaterMode mode='target_temperature' /> }
                        { originalHeater.has_temperature && <HeaterMode mode='dewpoint' /> }
                    </Form.Select>
                </Form.Group>
                <Collapse in={heater.mode !== 'off'}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Duty</Form.Label>
                        <Badge className='float-end'><Number value={heater.duty} formatFunction={formatPercentage} decimals={1} /></Badge>
                        <Form.Range min={0} max={1} step={0.001} value={heater.duty} onChange={updateHeater('duty', parseFloat)} />
                    </Form.Group>
                </Collapse>
                <Collapse in={heater.mode === 'target_temperature' && originalHeater.has_temperature}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Target Temperature</Form.Label>
                        <Badge className='float-end'><Number value={heater.target_temperature} unit='°C'/></Badge>
                        <Form.Range min={-20} max={50} value={heater.target_temperature} onChange={updateHeater('target_temperature', parseFloat)}/>
                    </Form.Group>
                </Collapse>
                <Collapse in={heater.mode === 'dewpoint' && originalHeater.has_temperature}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Dewpoint Offset</Form.Label>
                        <Badge className='float-end'><Number value={heater.dewpoint_offset} unit='°C' /></Badge>
                        <Form.Range min={-20} max={20} value={heater.dewpoint_offset} onChange={updateHeater('dewpoint_offset', parseFloat)}/>
                    </Form.Group>
                </Collapse>
            </Form>
        </Modal.Body>
        <Modal.Footer>
          <Button variant="secondary" onClick={onClose}>
            Close
          </Button>
          <Button variant="primary" onClick={onUpdateClicked}>Apply</Button>
        </Modal.Footer>
      </Modal>
}



const Heater = ({heater, index}) => {
    const [modalVisible, setShowModal] = useState(false);
    const { dewpoint } = useSelector(selectAmbient);
    const hideModal = () => setShowModal(false);
    const showModal = () => setShowModal(true);

    const renderDewpointTooltip = (props) => (
        <Tooltip {...props}>
            Dewpoint <Number value={dewpoint} unit='°C'/>, offset <Number value={heater.dewpoint_offset} unit='°C'/>
        </Tooltip>
    )
    
    return <tr>
        <th scope="row">{index+1}</th>
        <td>{HeaterModes[heater.mode]}</td>
        <td>
            { heater.mode === 'dewpoint' ?
                <OverlayTrigger overlay={renderDewpointTooltip} placement='top'>
                    <a href='#'>
                        <Number value={heater.dewpoint_offset + dewpoint} unit='°C'/>
                    </a>
                </OverlayTrigger> :
                <Number value={heater.target_temperature} unit='°C'/>
            }
            
        </td>
        <td>
            <span className='align-middle d-flex justify-content-start align-items-center'>
                { heater.active ? <FaLightbulb color='green'/> : <FaPowerOff color='grey' /> }
                <span className='px-2'>
                    <Number value={heater.duty} formatFunction={formatPercentage} decimals={1} />
                </span>
            </span>
            </td>
        <td><Number value={heater.temperature} unit='°C'/></td>
        <td><Button size='sm' onClick={showModal}>set</Button><SetHeaterModal heater={heater} onClose={hideModal} index={index} show={modalVisible} /></td>
    </tr>
}

export const Heaters = () => {
    const { heaters } = useSelector(selectHeaters);
    return <>
        <Table>
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
        <Accordion>
            <Accordion.Item eventKey='0'>
                <Accordion.Header>History</Accordion.Header>
                <Accordion.Body>
                    <HeatersChart/>
                </Accordion.Body>
            </Accordion.Item>
        </Accordion>
    </>

}