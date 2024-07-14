import Navbar from 'react-bootstrap/Navbar';
import Nav from 'react-bootstrap/Nav';
import { useDispatch, useSelector } from 'react-redux';
import { setTab, tabSelector } from './features/app/appSlice';

const NavTabItem = ({eventKey, text}) => {
    const dispatch = useDispatch()
    const activeTab = useSelector(tabSelector);
    const navigateTo = tab => dispatch(setTab(tab))

    return <Nav.Item>
        <Nav.Link eventKey={eventKey} active={activeTab === eventKey} onClick={() => navigateTo(eventKey)}>{text}</Nav.Link>
    </Nav.Item>
}

export const AppNavbar = () => {
    return <Navbar expand='lg' className="bg-body-tertiary px-4" bg="dark" data-bs-theme="dark">
        <Navbar.Brand>AstroPowerBox</Navbar.Brand>
        <Navbar.Toggle />
        <Navbar.Collapse>
            <Nav variant='tabs'>
                <NavTabItem eventKey='home' text='Home' />
                <NavTabItem eventKey='wifi' text='WiFi' />
                <Nav.Item><Nav.Link href="/update" target="_blank">Update</Nav.Link></Nav.Item>
            </Nav>
        </Navbar.Collapse>
    </Navbar>
}