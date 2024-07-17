import Table from 'react-bootstrap/Table';
import Spinner from 'react-bootstrap/Spinner';
import Button from 'react-bootstrap/Button';
import Modal from 'react-bootstrap/Modal';
import { useDispatch, useSelector } from 'react-redux';
import { appInfoSelector, appUptimeSelector, getAppInfoAsync, reconnectWiFiAsync, restartAsync } from './appSlice';
import { Number, formatPercentage, formatSize, formatTime } from '../Number';
import { useEffect, useState } from 'react';

const ConfirmModal = ({
        RenderButton,
        onCancel=() => {},
        onConfirm,
        title="Confirm?",
        text="Do you want to confirm?",
        cancelButton="Cancel",
        confirmButton="Confirm",
    }) => {
    const [show, setShow] = useState(false);
    const closeModal = () => setShow(false);
    const cancelModal = () => {
        onCancel();
        closeModal();
    }
    const confirmModal = () => {
        onConfirm();
        closeModal();
    }
    return <>
        <RenderButton onClick={() => setShow(true)} />
        <Modal show={show} onHide={onCancel}>
            <Modal.Header closeButton>
                <Modal.Title>{title}</Modal.Title>
            </Modal.Header>
            <Modal.Body>
                {text}
            </Modal.Body>
            <Modal.Footer>
                <Button variant="secondary" onClick={cancelModal}>{cancelButton}</Button>
                <Button variant="primary" onClick={confirmModal}>{confirmButton}</Button>
            </Modal.Footer>
        </Modal>
    </>
}


export const System = () => {
    const dispatch = useDispatch();
    useEffect(() => { dispatch(getAppInfoAsync()) }, [dispatch])
    const appUptime = useSelector(appUptimeSelector)
    const appInfo = useSelector(appInfoSelector);
    if(!appInfo.ready) {
        return <Spinner />
    }
    
    return <>
        <Table size='sm'>
            <tbody>
                <tr>
                    <th scope="row">Uptime</th>
                    <td><Number value={appUptime} formatFunction={formatTime} /></td>
                </tr>
            </tbody>
        </Table>
        <Table className='mt-5' size='sm'>
            <thead>
                <tr>
                    <th></th>
                    <th>Total</th>
                    <th>Free</th>
                    <th>Used</th>
                    <th>Max allocable</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <th scope='row'>Heap</th>
                    <td>
                        <Number value={appInfo.mem.heapSize} formatFunction={formatSize} />
                    </td>
                    <td>
                        <Number value={appInfo.mem.freeHeap} formatFunction={formatSize} />{" "}
                        (<Number value={appInfo.mem.freeHeap/appInfo.mem.heapSize} formatFunction={formatPercentage} />)
                    </td>
                    <td>
                        <Number value={appInfo.mem.usedHeap} formatFunction={formatSize} />{" "}
                        (<Number value={appInfo.mem.usedHeap/appInfo.mem.heapSize} formatFunction={formatPercentage} />)
                    </td>
                    <td>
                        <Number value={appInfo.mem.maxAllocHeap} formatFunction={formatSize} />{" "}
                        (<Number value={appInfo.mem.maxAllocHeap/appInfo.mem.heapSize} formatFunction={formatPercentage} />)
                    </td>
                </tr>
            </tbody>
        </Table>
        <Table className='mt-5' size='sm'>
            <thead>
                <tr>
                    <th></th>
                    <th>Total</th>
                    <th>Free</th>
                    <th>Used</th>
                    <th>MD5</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <th scope='row'>Sketch</th>
                    <td>
                        <Number value={appInfo.sketch.totalSpace} formatFunction={formatSize} />
                    </td>
                    <td>
                        <Number value={appInfo.sketch.freeSpace} formatFunction={formatSize} />{" "}
                        (<Number value={appInfo.sketch.freeSpaceRatio} formatFunction={formatPercentage} />)
                    </td>
                    <td>
                        <Number value={appInfo.sketch.size} formatFunction={formatSize} />{" "}
                        (<Number value={appInfo.sketch.sizeRatio} formatFunction={formatPercentage} />)
                    </td>
                    <td>{appInfo.sketch.MD5}</td>
                </tr>
            </tbody>
        </Table>
        <Table size='sm' className='mt-5'>
            <tbody>
                <tr>
                    <th scope="row">Chip model</th>
                    <td>{appInfo.esp.chipModel}</td>
                </tr>
                <tr>
                    <th scope="row">Cores</th>
                    <td>{appInfo.esp.chipCores}</td>
                </tr>
                <tr>
                    <th scope="row">Frequency</th>
                    <td>{appInfo.esp.cpuFreqMHz} MHz</td>
                </tr>
            </tbody>
        </Table>
        <h5 className='mt-5'>Actions</h5>
        <Button onClick={() => dispatch(getAppInfoAsync())}>Reload info</Button>
        <ConfirmModal 
            confirmButton='Reconnect' 
            text='Reconnecting WiFi might cause connectivity loss. Do you want to continue?'
            onConfirm={() => dispatch(reconnectWiFiAsync())}
            RenderButton={(props) => <Button className='ms-2' {...props} variant='warning'>Reconnect WiFi</Button>}
        />
        <ConfirmModal 
            confirmButton='Restart' 
            text='Are you sure you want to restart AstroPowerBox?'
            onConfirm={() => dispatch(restartAsync())}
            RenderButton={(props) => <Button {...props} className='ms-2' variant='danger'>Restart</Button>}
        />
    </>
}