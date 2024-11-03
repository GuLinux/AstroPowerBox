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
    selectWiFiStationsConfig
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

export const WiFi = () => {
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