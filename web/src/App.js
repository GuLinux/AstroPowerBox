import React, { useEffect } from 'react';
import { AppNavbar } from './AppNavbar';
import { setAmbient } from './features/sensors/ambient/ambientSlice';
import { useDispatch, useSelector } from 'react-redux';
import { getHeatersAsync, updateHeaters } from './features/sensors/heaters/heatersSlice';
import { setPower } from './features/sensors/power/powerSlice';
import { darkModeSelector, getHistoryAsync, setUptime, tabSelector } from './features/app/appSlice';
import Tab from 'react-bootstrap/Tab';
import Container from 'react-bootstrap/Container';
import { Home } from './features/Home';
import { WiFi } from './features/app/WiFi';
import { System } from './features/app/System';

const registerEventSource = dispatch => {
  const es = new EventSource('/api/events');
  es.addEventListener('status', m => {
    const data = JSON.parse(m.data);
    dispatch(setAmbient(data.ambient));
    dispatch(updateHeaters(data.heaters));
    dispatch(setPower(data.power));
    dispatch(setUptime(data.app.uptime));
  })
  return () => es.close()
}

const DarkMode = () => <link rel="stylesheet" type="text/css" href='darkly.min.css' />;
const LightMode = () => <link rel="stylesheet" type="text/css" href='flatly.min.css' />;

function App() {
  const dispatch = useDispatch();
  const darkMode = useSelector(darkModeSelector)
  useEffect(() => { dispatch(getHistoryAsync()) }, [dispatch])
  useEffect(() => { dispatch(getHeatersAsync()) }, [dispatch])
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
            <Tab.Pane eventKey='wifi'><WiFi /></Tab.Pane>
            <Tab.Pane eventKey='system'><System /></Tab.Pane>
          </Tab.Content>
        </Tab.Container>
      </Container>
    </>
  );
}

export default App;
