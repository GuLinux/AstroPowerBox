import Table from 'react-bootstrap/Table';
import Spinner from 'react-bootstrap/Spinner';
import Button from 'react-bootstrap/Button';
import { useDispatch, useSelector } from 'react-redux';
import { appInfoSelector, appUptimeSelector, getAppInfoAsync, reconnectWiFiAsync, restartAsync } from './appSlice';
import { Number, formatPercentage, formatSize, formatTime } from '../Number';
import { useEffect } from 'react';
import { saveConfigAsync } from './wifiSlice';
import { ConfirmModal } from '../../ConfirmModal';
import { RestartModalButton } from './RestartModalButton';


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
        <RestartModalButton className='ms-2' />
    </>
}