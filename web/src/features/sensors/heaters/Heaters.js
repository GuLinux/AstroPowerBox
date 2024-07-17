import Table from 'react-bootstrap/Table';
import { useDispatch, useSelector } from 'react-redux';
import { Number, formatPercentage } from '../../Number';
import { selectHeaters, setHeaterAsync } from './heatersSlice';
import { FaPowerOff, FaLightbulb } from "react-icons/fa6";
import Button from 'react-bootstrap/Button';
import Modal from 'react-bootstrap/Modal';
import Form from 'react-bootstrap/Form';
import OverlayTrigger from 'react-bootstrap/OverlayTrigger';
import Tooltip from 'react-bootstrap/Tooltip';
import Badge from 'react-bootstrap/Badge';
import Collapse from 'react-bootstrap/Collapse';
import { useState } from 'react';
import { selectAmbient } from '../ambient/ambientSlice';

const HeaterModes = {
    off: 'Off',
    fixed: 'Fixed',
    target_temperature: 'Set to Temperature',
    dewpoint: 'Set to Dewpoint',
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
                        { heater.has_temperature && <HeaterMode mode='target_temperature' /> }
                        { heater.has_temperature && <HeaterMode mode='dewpoint' /> }
                    </Form.Select>
                </Form.Group>
                <Collapse in={heater.mode !== 'off'}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Duty</Form.Label>
                        <Badge className='float-end'><Number value={heater.duty} formatFunction={formatPercentage} decimals={1} /></Badge>
                        <Form.Range min={0} max={1} step={0.001} value={heater.duty} onChange={updateHeater('duty', parseFloat)} />
                    </Form.Group>
                </Collapse>
                <Collapse in={heater.mode === 'target_temperature'}>
                    <Form.Group className='mb-3'>
                        <Form.Label>Target Temperature</Form.Label>
                        <Badge className='float-end'><Number value={heater.target_temperature} unit='°C'/></Badge>
                        <Form.Range min={-20} max={50} value={heater.target_temperature} onChange={updateHeater('target_temperature', parseFloat)}/>
                    </Form.Group>
                </Collapse>
                <Collapse in={heater.mode === 'dewpoint'}>
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