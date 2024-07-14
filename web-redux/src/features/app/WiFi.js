import { Fragment, useEffect, useState } from 'react';
import Form from 'react-bootstrap/Form';
import Button from 'react-bootstrap/Button';
import InputGroup from 'react-bootstrap/InputGroup';
import { useDispatch, useSelector } from 'react-redux';
import { getWiFiConfigAsync, selectWiFiAccessPointConfig, selectWiFiConfigReady, selectWiFiStationsConfig } from './wifiSlice';
import Spinner from 'react-bootstrap/Spinner';
import { FaRegEye, FaRegEyeSlash } from "react-icons/fa";
import Container from 'react-bootstrap/Container';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import { MdRowing } from 'react-icons/md';

const WiFiPasswordControl = (props) => {
    const [show, setShow] = useState(false);
    return <InputGroup>
        <Form.Control type={show ? 'text' : 'password'} {...props} autoComplete='new-password' />
        <Button variant='outline-secondary' onClick={() => setShow(!show) }>
            { show ? <FaRegEyeSlash /> : <FaRegEye />}
        </Button>
    </InputGroup>
}

const WiFiAccessPoint = () => {
    const accessPoint = useSelector(selectWiFiAccessPointConfig);
    const [essid, setEssid] = useState(accessPoint.essid);
    const [psk, setPsk] = useState(accessPoint.psk)
    return <Form>
        <Form.Group as={Row}>
            <Form.Label column sm={4}>Hostname/AccessPoint ESSID</Form.Label>
            <Col sm={8}>
                <Form.Control type='text' value={essid} onChange={e => setEssid(e.target.value)}/>
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

    </Form>
}

const WiFiStations = () => {
    const stations = useSelector(selectWiFiStationsConfig);
    return <Form>
        { stations.map((station, index) => <Fragment key={index}>
            <h4 className='pt-4'>Station {index}</h4>
            <Form.Group as={Row}>
                <Form.Label column sm={2}>ESSID</Form.Label>
                <Col sm={4}>
                    <Form.Control type='text' />
                </Col>
                <Form.Label column sm={2}>Password</Form.Label>
                <Col sm={4}>
                    <WiFiPasswordControl />
                </Col>
            </Form.Group>
        </Fragment>
        )}
    </Form>
}

export const WiFi = () => {
    const wifiConfigReady = useSelector(selectWiFiConfigReady)
    if(!wifiConfigReady) {
        return <Spinner /> 
    }
    return <Container>
        <Row>
            <Col md={{ span:10, offset:1 }}>
                <WiFiAccessPoint />
            </Col>
        </Row>
         <Row>
            <Col md={{ span:10, offset:1 }}>
                <WiFiStations />
            </Col>
        </Row>
        
    </Container>
    
        
}