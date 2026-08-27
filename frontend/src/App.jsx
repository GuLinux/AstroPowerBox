import React, { useEffect } from 'react';
import { AppNavbar } from './AppNavbar';
import { useDispatch, useSelector } from 'react-redux';
import { getPWMOutputsAsync, updatePWMOutputs } from './features/sensors/pwmOutputs/pwmOutputsSlice';
import { darkModeSelector, getAppStatusAsync, getHistoryAsync, tabSelector } from './features/app/appSlice';
import Tab from 'react-bootstrap/Tab';
import Container from 'react-bootstrap/Container';
import { Home } from './features/Home';
import { Config } from './features/app/Config';
import { System } from './features/app/System';
import { selectWiFiAccessPointConfig } from './features/app/configSlice';

const registerEventSource = dispatch => {
  const es = new EventSource('/api/events');
  es.addEventListener('pins', m => {
    const data = JSON.parse(m.data);
    const pinStates = data && typeof data === 'object' && !Array.isArray(data) ? data : {};
    const pwmOutputs = Object.entries(pinStates)
      .filter(([id]) => id.startsWith('heater_') || id.startsWith('output_'))
      .map(([id, pin]) => {
        const duty = typeof pin.duty === 'number' ? pin.duty : (pin.on ? 1 : 0);
        return {
          duty,
          active: !!pin.on,
          type: id.startsWith('heater_') ? 'heater' : 'output',
          temperature: typeof pin.temperature === 'number' ? pin.temperature : undefined,
        };
      });
    if (pwmOutputs.length > 0) {
      dispatch(updatePWMOutputs(pwmOutputs));
    }
  })
  return () => es.close()
}

const DarkMode = () => <link rel="stylesheet" type="text/css" href='/assets/darkly.min.css' />;
const LightMode = () => <link rel="stylesheet" type="text/css" href='/assets/flatly.min.css' />;

function App() {
  const dispatch = useDispatch();
  const darkMode = useSelector(darkModeSelector)
  useEffect(() => { dispatch(getAppStatusAsync()) }, [dispatch])
  useEffect(() => { dispatch(getHistoryAsync()) }, [dispatch])
  useEffect(() => { dispatch(getPWMOutputsAsync()) }, [dispatch])
  useEffect(() => registerEventSource(dispatch), [dispatch]);
  const activeTab = useSelector(tabSelector);
  const accessPoint = useSelector(selectWiFiAccessPointConfig)
  useEffect(() => {
    if(!!accessPoint.ssid) {
      document.title = accessPoint.ssid;
    }
  });
  return (
    <>
      { darkMode ? <DarkMode /> : <LightMode /> }
      
      <AppNavbar />
      <Container className="pt-3">
        <Tab.Container activeKey={activeTab}>
          <Tab.Content>
            <Tab.Pane eventKey='home'><Home /></Tab.Pane>
            <Tab.Pane eventKey='config'><Config /></Tab.Pane>
            <Tab.Pane eventKey='system'><System /></Tab.Pane>
          </Tab.Content>
        </Tab.Container>
      </Container>
    </>
  );
}

export default App;
