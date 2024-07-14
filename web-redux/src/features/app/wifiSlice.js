import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { fetchWifiStatus } from './api';

const initialState = {
    ready: false,
    accessPoint: {
        essid: '',
        psk: '',
    },
    stations: [],
};

export const getWiFiConfigAsync = createAsyncThunk(
  'wifi/getWiFiConfig',
  async () => await fetchWifiStatus()
);

export const selectWiFiConfigReady = state => state.wifi.ready;
export const selectWiFiAccessPointConfig = state => state.wifi.accessPoint;
export const selectWiFiStationsConfig = state => state.wifi.stations;

export const wifiSlice = createSlice({
  name: 'wifi',
  initialState,
  reducers: {
    setWiFiConfig: (state, action) => {
        state.accessPoint = action.accessPoint;
        state.stations = action.stations;
    }
  },
  extraReducers: (builder) => {
    builder
      .addCase(getWiFiConfigAsync.pending, (state) => { state.ready = false })
      .addCase(getWiFiConfigAsync.fulfilled, (state, {payload: {accessPoint, stations}}) => {
        state.accessPoint = accessPoint;
        state.stations = stations;
        state.ready = true;
      })
  },
});
 
export default wifiSlice.reducer;
export const { setWiFiConfig } = wifiSlice.actions;