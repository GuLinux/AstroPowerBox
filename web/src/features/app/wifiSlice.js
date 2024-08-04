import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { fetchConfig, removeWiFiStationConfig, saveConfig, saveWiFiAccessPointConfig, saveWiFiStationConfig } from './api';

const initialState = {
    ready: false,
    accessPoint: {
        essid: '',
        psk: '',
    },
    stations: [],
};

export const getConfigAsync = createAsyncThunk(
  'wifi/getConfig',
  async () => await fetchConfig()
);

export const saveAccessPointConfigAsync = createAsyncThunk(
  'wifi/saveAccessPoint',
  async payload => await saveWiFiAccessPointConfig(payload)
);

export const saveStationConfigAsync = createAsyncThunk(
  'wifi/saveStation',
  async payload => await saveWiFiStationConfig(payload)
);

export const removeStationConfigAsync = createAsyncThunk(
  'wifi/removeStation',
  async payload => await removeWiFiStationConfig(payload)
);


export const saveConfigAsync = createAsyncThunk(
  'wifi/saveConfig',
  async () => await saveConfig()
);



export const selectWiFiConfig = state => state.wifi;
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
      .addCase(getConfigAsync.pending, (state) => { state.ready = false })
      .addCase(getConfigAsync.fulfilled, (_, {payload}) => ({...payload, ready: true}))
      .addCase(saveAccessPointConfigAsync.fulfilled, (_, {payload}) => ({...payload, ready: true}))
      .addCase(saveConfigAsync.fulfilled, (_, {payload}) => ({...payload, ready: true}))
      .addCase(saveStationConfigAsync.fulfilled, (_, {payload}) => ({...payload, ready: true}))
      .addCase(removeStationConfigAsync.fulfilled, (_, {payload}) => ({...payload, ready: true}))
  },
});
 
export default wifiSlice.reducer;
export const { setWiFiConfig } = wifiSlice.actions;