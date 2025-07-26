import { createAsyncThunk, createSelector, createSlice } from '@reduxjs/toolkit';
import { fetchConfig, removeWiFiStationConfig, saveConfig, saveWiFiAccessPointConfig, saveWiFiStationConfig, setPowerSourceType, setStatusLedDuty } from './api';

const initialState = {
    ready: false,
    accessPoint: {
        essid: '',
        psk: '',
    },
    stations: [],
    ledDuty: 1,
    powerSourceType: 'AC',
};

export const getConfigAsync = createAsyncThunk(
  'config/getConfig',
  async () => await fetchConfig()
);

export const saveAccessPointConfigAsync = createAsyncThunk(
  'config/saveAccessPoint',
  async payload => await saveWiFiAccessPointConfig(payload)
);

export const saveStationConfigAsync = createAsyncThunk(
  'config/saveStation',
  async payload => await saveWiFiStationConfig(payload)
);

export const removeStationConfigAsync = createAsyncThunk(
  'config/removeStation',
  async payload => await removeWiFiStationConfig(payload)
);


export const saveConfigAsync = createAsyncThunk(
  'config/saveConfig',
  async () => await saveConfig()
);

export const setStatusLedDutyAsync = createAsyncThunk(
  'config/setStatusLedDuty',
  async ({duty}) => await setStatusLedDuty({ duty })
)

export const setPowerSourceTypeAsync = createAsyncThunk(
  'config/setPowerSourceType',
  async ({powerSourceType}) => await setPowerSourceType({ powerSourceType })
)




export const selectConfig = state => state.config;
export const selectConfigReady = createSelector([selectConfig], config => config.ready)
export const selectPowerSourceType = createSelector([selectConfig], config => config.powerSourceType)
export const selectStatusLedDuty = createSelector([selectConfig], config => config.ledDuty)
export const selectWiFiAccessPointConfig = createSelector([selectConfig], config => config.accessPoint)
export const selectWiFiStationsConfig = createSelector([selectConfig], config => config.stations)

export const wifiSlice = createSlice({
  name: 'config',
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
      .addCase(setPowerSourceTypeAsync.fulfilled, (state, {payload: {powerSourceType}}) => {
        state.powerSourceType = powerSourceType
      })
      .addCase(setStatusLedDutyAsync.fulfilled, (state, {payload: {duty}}) => {
        state.ledDuty = duty;
      })

  },
});
 
export default wifiSlice.reducer;
export const { setWiFiConfig } = wifiSlice.actions;