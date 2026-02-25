import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { fetchAppInfo, fetchHistory, fetchReconnectWiFi, fetchRestart, fetchStatus } from './api';

const initialState = {
    tab: 'home',
    darkMode: true,
    info: {
      ready: false,
      uptime: 0,
    },
    status: {}
};

export const tabSelector = state => state.app.tab
export const darkModeSelector = state => state.app.darkMode
export const appInfoSelector = state => state.app.info
export const appUptimeSelector = state => state.app.info.uptime
export const appStatusSelector = state => state.app.status

export const getAppInfoAsync = createAsyncThunk(
  'app/getInfo',
  async () => await fetchAppInfo()
);

export const getAppStatusAsync = createAsyncThunk(
  'app/getStatus',
  async () => await fetchStatus()
);

export const getHistoryAsync = createAsyncThunk(
  'app/getHistory',
  async () => await fetchHistory()
);


export const restartAsync = createAsyncThunk(
  'app/restart',
  async () => await fetchRestart()
);

export const reconnectWiFiAsync = createAsyncThunk(
  'app/reconnectWiFi',
  async () => await fetchReconnectWiFi()
);

export const appSlice = createSlice({
  name: 'app',
  initialState,
  reducers: {
    setTab: (state, action) => { state.tab = action.payload },
    setDarkMode: (state, action) => { state.darkMode = action.payload },
    setUptime: (state, action) => { state.info.uptime = action.payload },
  },
  extraReducers: (builder) => {
    builder
      .addCase(getAppStatusAsync.pending, (state) => {
        state.status.ready = false
      })
      .addCase(getAppStatusAsync.fulfilled, (state, {payload: status}) => {
        state.status.ready = true;
        state.status.hasPowerMonitor = status.has_power_monitor;
        state.status.hasAmbientSensor = status.has_ambient_sensor;
        state.status.status = status.status;
      })


      .addCase(getAppInfoAsync.pending, (state) => {
        state.info.ready = false
      })
      .addCase(getAppInfoAsync.fulfilled, (state, action) => {
        state.info = {...action.payload, uptime: state.info.uptime}
        state.info.sketch.freeSpace = state.info.sketch.totalSpace - state.info.sketch.size
        state.info.sketch.freeSpaceRatio = state.info.sketch.freeSpace / state.info.sketch.totalSpace
        state.info.sketch.sizeRatio = state.info.sketch.size / state.info.sketch.totalSpace

        state.info.ready = true
      })
    },
});
 
export default appSlice.reducer;
export const { setTab, setDarkMode, setUptime } = appSlice.actions;