import Navbar from 'react-bootstrap/Navbar';
import Nav from 'react-bootstrap/Nav';
import { useDispatch, useSelector } from 'react-redux';
import { darkModeSelector, setDarkMode, setTab, tabSelector } from './features/app/appSlice';
import { MdDarkMode, MdLightMode } from "react-icons/md";

const NavTabItem = ({eventKey, text}) => {
    const dispatch = useDispatch()
    const activeTab = useSelector(tabSelector);
    const navigateTo = tab => dispatch(setTab(tab))

    return <Nav.Item>
        <Nav.Link eventKey={eventKey} active={activeTab === eventKey} onClick={() => navigateTo(eventKey)}>{text}</Nav.Link>
    </Nav.Item>
}

export const AppNavbar = () => {
    const darkMode = useSelector(darkModeSelector);
    const navbarBg = darkMode ? 'dark' : 'light'
    const dispatch = useDispatch();
    return <Navbar expand='lg' className="bg-body-tertiary px-4" bg={navbarBg} data-bs-theme={navbarBg} collapseOnSelect={true}>
        <Navbar.Brand>AstroPowerBox</Navbar.Brand>
        <Navbar.Toggle />
        <Navbar.Collapse>
            <Nav variant='tabs' className='me-auto'>
                <NavTabItem eventKey='home' text='Home' />
                <NavTabItem eventKey='config' text='Configuration' />
                <NavTabItem eventKey='system' text='System' />
                <Nav.Item><Nav.Link href="/update" target="_blank">Update</Nav.Link></Nav.Item>
            </Nav>
            <Nav>
                {
                    darkMode ? 
                        <Nav.Item><Nav.Link onClick={() => dispatch(setDarkMode(false))}><MdLightMode size='1.5em' /></Nav.Link></Nav.Item>
                        :
                        <Nav.Item><Nav.Link onClick={() => dispatch(setDarkMode(true))}><MdDarkMode size='1.5em' /></Nav.Link></Nav.Item>
                }
                
            </Nav>
        </Navbar.Collapse>
    </Navbar>
}