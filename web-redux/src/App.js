import React, { useEffect } from 'react';
// import './App.css';
import { AppNavbar } from './AppNavbar';
import { setAmbient } from './features/ambientSlice';
import { useDispatch } from 'react-redux';
import { getHeatersAsync, setHeaters, updateHeaters } from './features/heatersSlice';
import { fetchHeaters } from './features/api';
import { setPower } from './features/powerSlice';

function App() {
  const dispatch = useDispatch();
  useEffect(() => { dispatch(getHeatersAsync()) })
  useEffect(() => {
    const es = new EventSource('/api/events');
    es.addEventListener('status', m => {
      const data = JSON.parse(m.data);
      dispatch(setAmbient(data.ambient))
      dispatch(updateHeaters(data.heaters))
      dispatch(setPower(data.power))
    })
    return () => es.close()
  }, []);
  return (
    <AppNavbar />
  );
}

export default App;
