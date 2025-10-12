import Table from 'react-bootstrap/Table';
import { useDispatch, useSelector } from 'react-redux';
import { Number, formatPercentage } from '../../Number';
import { selectPWMOutputs, selectPWMOutputsByType, selectPWMOutputsHistoryAsMap, setPWMOutputAsync } from './pwmOutputsSlice';
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


const PWMOutputModes = {
    off: 'Off',
    fixed: 'Fixed',
    target_temperature: 'Set to Temperature',
    dewpoint: 'Set to Dewpoint',
}

const PWMOutputsChart = ({type, showTemperature=true}) => {
    const id = useId();
    const { history, indexes } = useSelector(state => selectPWMOutputsHistoryAsMap(state, type))
    
    const [selected, setSelected] = useState(null);
    const [activePWMOutput, setActivePWMOutput] = useState(null);
    const shouldShow = key => selected === null || selected === key;
    if(history.length === 0) {
        return null;
    }

    const pwmOutputsToShow = activePWMOutput === null ? Array.from(indexes) : [activePWMOutput];

    
    const dropdownTitle = activePWMOutput === null ? 'all' : `${type} ${activePWMOutput}`
    const strokes = [
        { temp: 'ffaa22', duty: '22aaff'},
        { temp: 'ff27e9', duty: 'ffdb6e'},
        { temp: 'ff0206', duty: '8cffff'},
        { temp: '01ff51', duty: 'ffd91a'},
        { temp: 'ffd91a', duty: '01ff51'},
        { temp: 'ffbd6e', duty: 'ff27e9'},
        { temp: '22aaff', duty: 'ffaa22'},
        { temp: '8cffff', duty: 'ff0206'},
        
    ]
    const getStroke = (index, type) => {
        const stroke =  strokes[index%strokes.length][type]
        return `#${stroke}`
    }
 
    return <>
        <Form>
            <Form.Group as={Row} className="justify-content-md-center">
                <Form.Label column sm={4}>Filter by {type}</Form.Label>
                <Col sm={4}>
                    <DropdownButton title={dropdownTitle}>
                        <Dropdown.Item active={null === activePWMOutput} onClick={() => setActivePWMOutput(null)}>all</Dropdown.Item>
                        {Array.from(indexes).map(index =>
                            <Dropdown.Item key={index} active={index === activePWMOutput} onClick={() => setActivePWMOutput(index)}>
                                {type} {index}
                            </Dropdown.Item>)}

                    </DropdownButton>

                </Col>
            </Form.Group>
        </Form>

        {pwmOutputsToShow.length > 0 &&
        <div style={{ height: 400 }}>
                <ResponsiveContainer>
                <LineChart width={600} height={300} data={history}>
                    <CartesianGrid stroke="#ccc" />
                    <XAxis dataKey="name" />
                    <YAxis yAxisId={`tempYAxis-${id}`} />
                    <YAxis yAxisId={`dutyYAxis-${id}`} orientation='right' />
                    <Legend onClick={({dataKey}) => setSelected(selected !== dataKey ? dataKey : null)}/>
                    {pwmOutputsToShow.map(index => 
                        <>
                        {showTemperature && shouldShow(`pwmOutput-${index}-temperature`) && <Line animationDuration={100} name={`${type} ${index} temperature`} yAxisId={`tempYAxis-${id}`} type="monotone" dataKey={`pwmOutput-${index}-temperature`} stroke={getStroke(index, 'temp')} />}
                        {shouldShow(`pwmOutput-${index}-duty`) && <Line animationDuration={100} name={`${type} ${index} duty`} yAxisId={`dutyYAxis-${id}`} type="monotone" dataKey={`pwmOutput-${index}-duty`} stroke={getStroke(index, 'duty')} />}
                        </>
                    )}
                </LineChart>
            </ResponsiveContainer>
        </div>
        }
    </>
}

const PWMOutputMode = ({mode}) => <option value={mode}>{PWMOutputModes[mode]}</option>

const SetPWMOutputModal = ({pwmOutput: originalPWMOutput, show, onClose, index}) => {
    const [pwmOutput, setPWMOutput] = useState({target_temperature: 25, dewpoint_offset: 5, ...originalPWMOutput, duty: originalPWMOutput.duty === 0 ? 1 : originalPWMOutput.duty});
    const updatePWMOutput = (field, parser = value => value) => event => setPWMOutput({...pwmOutput, [field]: parser(event.target.value) })
    const dispatch = useDispatch()
    const onUpdateClicked = () => {
        dispatch(setPWMOutputAsync({index, pwmOutput}));
        onClose();
    }
    return <Modal show={show} onHide={onClose}>
        <Modal.Header closeButton>
          <Modal.Title>Set PWM Output {index+1}</Modal.Title>
        </Modal.Header>
        <Modal.Body>
            <Form>
                <Form.Group className='mb-3'>
                    <Form.Label>Mode</Form.Label>
                    <Form.Select value={pwmOutput.mode} onChange={updatePWMOutput('mode')}>
                        <PWMOutputMode mode='off' />
                        <PWMOutputMode mode='fixed' />
                        { originalPWMOutput.has_temperature && <PWMOutputMode mode='target_temperature' /> }
                        { originalPWMOutput.has_temperature && <PWMOutputMode mode='dewpoint' /> }
                    </Form.Select>
                </Form.Group>
                <Collapse in={pwmOutput.mode !== 'off'}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Duty</Form.Label>
                        <Badge className='float-end'><Number value={pwmOutput.max_duty} formatFunction={formatPercentage} decimals={1} /></Badge>
                        <Form.Range min={0} max={1} step={0.001} value={pwmOutput.max_duty} onChange={updatePWMOutput('max_duty', parseFloat)} />
                        { ['target_temperature', 'dewpoint'].includes(pwmOutput.mode) && 
                        <Form.Text>When ramp is set to a non zero value, and mode is either <code>Dewpoint offset</code> or <code>Target temperature</code>,
                        this will be a maximum value rather than the real duty.</Form.Text> }
                    </Form.Group>
                </Collapse>
                <Collapse in={pwmOutput.mode === 'target_temperature' && originalPWMOutput.has_temperature}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Target Temperature</Form.Label>
                        <Badge className='float-end'><Number value={pwmOutput.target_temperature} unit='°C'/></Badge>
                        <Form.Range min={-20} max={50} value={pwmOutput.target_temperature} onChange={updatePWMOutput('target_temperature', parseFloat)}/>
                        <Form.Text>When the temperature sensor will reach this temperature, the PWM output will turn off.</Form.Text>
                    </Form.Group>
                </Collapse>
                <Collapse in={pwmOutput.mode === 'dewpoint' && originalPWMOutput.has_temperature}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Dewpoint Offset</Form.Label>
                        <Badge className='float-end'><Number value={pwmOutput.dewpoint_offset} unit='°C' /></Badge>
                        <Form.Range min={-20} max={20} value={pwmOutput.dewpoint_offset} onChange={updatePWMOutput('dewpoint_offset', parseFloat)}/>
                        <Form.Text>
                            Offset to the dewpoint temperature (either positive or negative).
                            For instance, if the dewpoint is <code>10°C</code>, and the offset is set to <code>5°C</code>, the target temperature will be <code>15°C</code>.
                        </Form.Text>
                    </Form.Group>
                </Collapse>
                <Collapse in={['dewpoint', 'target_temperature'].includes(pwmOutput.mode) && originalPWMOutput.has_temperature}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Ramp Offset</Form.Label>
                        <Badge className='float-end'><Number value={pwmOutput.ramp_offset || 0} unit='°C' /></Badge>
                        <Form.Range min={0} max={5} step={.1} value={pwmOutput.ramp_offset || 0} onChange={updatePWMOutput('ramp_offset', parseFloat)}/>
                        <Form.Text>
                            Set this to a number greater than <code>0</code> to start ramping down the duty proportionally to the difference with the target temperature.
                            For instance, if set to <code>3°C</code>, with a target temperature of <code>25°C</code>, a current temperature of <code>24°C</code>
                            and a maximum duty of <code>100%</code>, the actual duty will be <code>(25-24)/3 = 33%</code>.
                        </Form.Text>
                    </Form.Group>
                </Collapse>
                <Collapse in={pwmOutput.ramp_offset>0 && ['dewpoint', 'target_temperature'].includes(pwmOutput.mode) && originalPWMOutput.has_temperature}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Mimimum duty</Form.Label>
                        <Badge className='float-end'><Number value={pwmOutput.min_duty*100|| 0} unit='°%' /></Badge>
                        <Form.Range min={0.0} max={1.0} step={.01} value={pwmOutput.min_duty|| 0} onChange={updatePWMOutput('min_duty', parseFloat)}/>
                        <Form.Text>
                            Minimum duty for PWM ramping.<br />
                            Note: the PWM output will set to <code>0</code> regardless of minimum duty when target temperature is reached.
                        </Form.Text>
                    </Form.Group>

                </Collapse>
                <Form.Check id={`applyAtStartup-${index}`} label='Apply at startup' checked={pwmOutput.apply_at_startup} onChange={e => setPWMOutput({...pwmOutput, 'apply_at_startup': e.target.checked})} />
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



const PWMOutput = ({pwmOutput, index, hasTemperature}) => {
    const [modalVisible, setShowModal] = useState(false);
    const { dewpoint } = useSelector(selectAmbient);
    const hideModal = () => setShowModal(false);
    const showModal = () => setShowModal(true);

    const renderDewpointTooltip = (props) => (
        <Tooltip {...props}>
            Dewpoint <Number value={dewpoint} unit='°C'/>, offset <Number value={pwmOutput.dewpoint_offset} unit='°C'/>
        </Tooltip>
    )
    
    return <tr>
        <th scope="row">{index}</th>
        <td>{PWMOutputModes[pwmOutput.mode]}</td>
        { hasTemperature &&
        <td>
            { pwmOutput.mode === 'dewpoint' ?
                <OverlayTrigger overlay={renderDewpointTooltip} placement='top'>
                    <a href='#'>
                        <Number value={pwmOutput.dewpoint_offset + dewpoint} unit='°C'/>
                    </a>
                </OverlayTrigger> :
                <Number value={pwmOutput.target_temperature} unit='°C'/>
            }
            
        </td> }
        <td>
            <span className='align-middle d-flex justify-content-start align-items-center'>
                { pwmOutput.active ? <FaLightbulb color='green'/> : <FaPowerOff color='grey' /> }
                <span className='px-2'>
                    <Number value={pwmOutput.duty} formatFunction={formatPercentage} decimals={1} />
                </span>
            </span>
            </td>
        { hasTemperature && <td><Number value={pwmOutput.temperature} unit='°C'/></td> }
        <td className='text-end'><Button size='sm' onClick={showModal}>set</Button><SetPWMOutputModal pwmOutput={pwmOutput} onClose={hideModal} index={index} show={modalVisible} /></td>
    </tr>
}

export const PWMOutputs = ({type}) => {
    const pwmOutputs = useSelector(state => selectPWMOutputsByType(state, type) );
    const [historyVisible, setHistoryVisible] = useState(false);
    const hasTemperature = type === 'heater';
    return <>
        <Table>
            <thead>
                <tr>
                <th scope="col">#</th>
                <th scope="col">mode</th>
                { hasTemperature && <th scope="col">target</th> }
                <th scope="col">duty</th>
                { hasTemperature && <th scope="col">temperature</th> }
                <th></th>
                </tr>
            </thead>
            <tbody>
                { pwmOutputs.map((pwmOutput) => <PWMOutput hasTemperature={hasTemperature} pwmOutput={pwmOutput} key={pwmOutput.index} index={pwmOutput.index} />)}
            </tbody>
        </Table>
        <Accordion>
            <Accordion.Item eventKey='0'>
                <Accordion.Header>History</Accordion.Header>
                <Accordion.Body onEnter={() => setHistoryVisible(true)} onExited={() => setHistoryVisible(false)}>
                    { historyVisible && <PWMOutputsChart type={type} showTemperature={hasTemperature} /> }
                </Accordion.Body>
            </Accordion.Item>
        </Accordion>
    </>

}