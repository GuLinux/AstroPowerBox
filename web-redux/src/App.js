import React, { Suspense, lazy, useEffect } from 'react';
// import './App.css';
import { AppNavbar } from './AppNavbar';
import { setAmbient } from './features/sensors/ambient/ambientSlice';
import { useDispatch, useSelector } from 'react-redux';
import { getHeatersAsync, setHeaters, updateHeaters } from './features/sensors/heaters/heatersSlice';
import { setPower } from './features/sensors/power/powerSlice';
import { darkModeSelector, tabSelector } from './features/app/appSlice';
import Tab from 'react-bootstrap/Tab';
import Container from 'react-bootstrap/Container';
import { Home } from './features/Home';

const registerEventSource = dispatch => {
  const es = new EventSource('/api/events');
  es.addEventListener('status', m => {
    const data = JSON.parse(m.data);
    dispatch(setAmbient(data.ambient))
    dispatch(updateHeaters(data.heaters))
    dispatch(setPower(data.power))
  })
  return () => es.close()
}

const DarkMode = () => <link rel="stylesheet" type="text/css" href='darkly.min.css' />;
const LightMode = () => <link rel="stylesheet" type="text/css" href='flatly.min.css' />;

function App() {
  const dispatch = useDispatch();
  const darkMode = useSelector(darkModeSelector)
  useEffect(() => { dispatch(getHeatersAsync()) })
  useEffect(() => registerEventSource(dispatch), [dispatch]);
  const activeTab = useSelector(tabSelector);
  return (
    <>
      { darkMode ? <DarkMode /> : <LightMode /> }
      
      <AppNavbar />
      <Container className="pt-3">
        <Tab.Container activeKey={activeTab}>
          <Tab.Content>
            <Tab.Pane eventKey='home'><Home /></Tab.Pane>
            <Tab.Pane eventKey='wifi'>wifi</Tab.Pane>
          </Tab.Content>
        </Tab.Container>
      </Container>
    </>
  );
}

export default App;
