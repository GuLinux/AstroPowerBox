import React, { useEffect } from 'react';
// import './App.css';
import { AppNavbar } from './AppNavbar';
import { setAmbient } from './features/sensors/ambient/ambientSlice';
import { useDispatch, useSelector } from 'react-redux';
import { getHeatersAsync, setHeaters, updateHeaters } from './features/sensors/heaters/heatersSlice';
import { setPower } from './features/sensors/power/powerSlice';
import { tabSelector } from './features/app/appSlice';
import Tab from 'react-bootstrap/Tab';
import Container from 'react-bootstrap/Container';
import { Home } from './features/Home';
import 'bootswatch/dist/darkly/bootstrap.min.css';

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

function App() {
  const dispatch = useDispatch();
  useEffect(() => { dispatch(getHeatersAsync()) })
  useEffect(() => registerEventSource(dispatch), [dispatch]);
  const activeTab = useSelector(tabSelector);
  return (
    <>
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
