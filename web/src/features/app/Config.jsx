import { useEffect, useState } from 'react';
import Form from 'react-bootstrap/Form';
import Button from 'react-bootstrap/Button';
import ButtonGroup from 'react-bootstrap/ButtonGroup';
import InputGroup from 'react-bootstrap/InputGroup';
import { useDispatch, useSelector } from 'react-redux';
import {
    getConfigAsync,
    removeStationConfigAsync,
    saveAccessPointConfigAsync,
    saveStationConfigAsync,
    selectWiFiAccessPointConfig,
    selectConfig,
    selectWiFiStationsConfig,
    selectPowerSourceType,
    selectStatusLedDuty,
    setStatusLedDutyAsync,
    setPowerSourceTypeAsync
} from './configSlice';
import Spinner from 'react-bootstrap/Spinner';
import { FaRegEye, FaRegEyeSlash } from "react-icons/fa";
import Container from 'react-bootstrap/Container';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import { ConfirmModal } from '../../ConfirmModal';
import { reconnectWiFiAsync } from './appSlice';
import { RestartModalButton } from './RestartModalButton';
import { SaveConfigModalButton } from './SaveConfigModalButton';
import { Number } from '../Number';
import Badge from 'react-bootstrap/Badge';
import { setPowerSourceType } from './api';

const WiFiPasswordControl = ({ label, ...props }) => {
    const [show, setShow] = useState(false);
    return <InputGroup>
        {label && <InputGroup.Text>{label}</InputGroup.Text>}
        <Form.Control type={show ? 'text' : 'password'} {...props} autoComplete='new-password' />
        <Button variant='outline-secondary' onClick={() => setShow(!show)}>
            {show ? <FaRegEyeSlash /> : <FaRegEye />}
        </Button>
    </InputGroup>
}

const WiFiAccessPoint = () => {
    const dispatch = useDispatch();
    const accessPoint = useSelector(selectWiFiAccessPointConfig);
    const [essid, setEssid] = useState(accessPoint.essid);
    const [psk, setPsk] = useState(accessPoint.psk)
    const isChanged = () => psk !== accessPoint.psk || essid !== accessPoint.essid
    const resetState = () => {
        setEssid(accessPoint.essid);
        setPsk(accessPoint.psk);
    }
    return <Form>
        <Form.Group as={Row}>
            <Form.Label column sm={4}>Hostname/AccessPoint ESSID</Form.Label>
            <Col sm={8}>
                <Form.Control type='text' value={essid} onChange={e => setEssid(e.target.value)} />
                <Form.Text muted>This will be used as he DHCP hostname sent to your server. It's also the access point ESSID that will be used if AstroPowerBox can't be connected to any WiFi station</Form.Text>
            </Col>
        </Form.Group>

        <Form.Group as={Row}>
            <Form.Label column sm={4}>AccessPoint Password</Form.Label>
            <Col sm={8}>
                <WiFiPasswordControl value={psk} onChange={e => setPsk(e.target.value)} />
                <Form.Text muted>The password that will be used to access AstroPowerBox if it can't connect to any WiFi station. If left blank, WiFi accesspoint will be open.</Form.Text>
            </Col>
        </Form.Group>
        <ButtonGroup className='float-end'>
            <Button disabled={!isChanged()} variant="secondary" onClick={resetState}>Reset</Button>
            <Button disabled={!isChanged()} variant="danger" onClick={() => dispatch(saveAccessPointConfigAsync({ essid, psk }))}>Update</Button>
        </ButtonGroup>
    </Form>
}

const WifiStation = ({ station, index }) => {
    const [essid, setEssid] = useState(station.essid)
    const [psk, setPsk] = useState(station.psk)
    const dispatch = useDispatch();
    const isChanged = () => station.essid !== essid || station.psk !== psk
    const resetState = () => {
        setEssid(station.essid)
        setPsk(station.psk)
    }
    useEffect(resetState, [station]);
    return <Form.Group as={Row} className='mt-2'>
        <Form.Label column lg={1}>Station {index}</Form.Label>
        <Col lg={4}>
            <InputGroup>
                <InputGroup.Text>ESSID</InputGroup.Text>
                <Form.Control type='text' value={essid} onChange={e => setEssid(e.target.value)} />
            </InputGroup>
        </Col>
        <Col lg={5}>
            <WiFiPasswordControl label='Password' value={psk} onChange={e => setPsk(e.target.value)} />
        </Col>
        <Col lg={2} className='d-grid'>
            <ButtonGroup className='float-end' size='sm'>
                <Button disabled={!isChanged()} variant="secondary" onClick={resetState}>Reset</Button>
                <Button disabled={!isChanged()} variant="danger" onClick={() => dispatch(saveStationConfigAsync({ index, essid, psk }))}>Update</Button>
                <Button disabled={!station.essid && !station.psk} variant="warning" onClick={() => dispatch(removeStationConfigAsync({ index }))}>Remove</Button>
            </ButtonGroup>
        </Col>
    </Form.Group>
}

const WiFiStations = () => {
    const stations = useSelector(selectWiFiStationsConfig);
    return <Form>
        {stations.map((station, index) => <WifiStation station={station} index={index} key={index} />)}
    </Form>
}

const AppSettings = () => {
    const dispatch = useDispatch();
    const upstreamPowerSourceType = useSelector(selectPowerSourceType);
    const upstreamLedBrightness = useSelector(selectStatusLedDuty);

    const [ledBrightness, setLedBrightness] = useState(upstreamLedBrightness);
    const [powerSupply, setPowerSupply] = useState(upstreamPowerSourceType);
    const [updateBrightnessTimer, setUpdateBrightnessTimer] = useState()
    useEffect(() => {
        setLedBrightness(upstreamLedBrightness);
        setPowerSupply(upstreamPowerSourceType);
    }, [setLedBrightness, setPowerSupply, upstreamLedBrightness, upstreamPowerSourceType]);

    const updateLedBrightness = duty => {
        setLedBrightness(duty)
        if(updateBrightnessTimer) {
            clearTimeout(updateBrightnessTimer)
        }
        setUpdateBrightnessTimer(setTimeout(() => {
            dispatch(setStatusLedDutyAsync({duty}))
        }, 1000))
        
    }
    const updatePowerSourceType = powerSourceType => {
        setPowerSupply(powerSourceType);
        dispatch(setPowerSourceTypeAsync({powerSourceType}))
    }
    return <Form>
        <Form.Group className='mb-3'>
            <Form.Label>Status led brightness</Form.Label>
            <Badge className='float-end'><Number value={ledBrightness*100} decimals={0} unit='%' /></Badge>
            <Form.Range min={0.0} max={1.0} step={0.01} value={ledBrightness} onChange={e => updateLedBrightness(parseFloat(e.target.value))}/>
        </Form.Group>
        <Form.Group className='mb-3'>
            <Form.Label>Power supply</Form.Label>
            <Form.Select value={powerSupply} onChange={e => updatePowerSourceType(e.target.value)}>
                <option value="AC">AC</option>
                <option value="lipo_3c">LiPo Battery 3 cells (12v)</option>
            </Form.Select>
        </Form.Group>
    </Form>
}

export const Config = () => {
    const dispatch = useDispatch();
    useEffect(() => { dispatch(getConfigAsync()) }, [dispatch])
    const wifiConfig = useSelector(selectConfig)

    if (!wifiConfig.ready) {
        return <Spinner />
    }
    return <Container>
        <Row>
            <Col md={{ span: 10, offset: 1 }}>
                <WiFiAccessPoint />
            </Col>
        </Row>
        <Row className='mt-5'>
            <WiFiStations />
        </Row>
        <Row className='mt-5'>
            <AppSettings />
        </Row>
        <Row className='mt-5'>
            <Col sm={10}>Unsaved changes will be lost after restart. Save configuration to flash storage to persist them.</Col>
            <Col sm={2} className='d-grid'><SaveConfigModalButton /></Col>
        </Row>
        <Row className='mt-2'>
            <Col sm={10}>Reconnecting wifi will allow to apply some changes without restarting.</Col>
            <Col sm={2} className='d-grid'>
                <ConfirmModal
                    confirmButton='Reconnect'
                    text='Reconnecting WiFi might cause connectivity loss. Do you want to continue?'
                    onConfirm={() => dispatch(reconnectWiFiAsync())}
                    RenderButton={(props) => <Button {...props} variant='warning'>Reconnect WiFi</Button>}
                />
            </Col>
        </Row>
        <Row className='mt-2'>
            <Col sm={10}>Restart AstroPowerBox to apply saved changes. Unsaved changes will be lost.</Col>
            <Col sm={2} className='d-grid'><RestartModalButton /></Col>
        </Row>
    </Container>


}