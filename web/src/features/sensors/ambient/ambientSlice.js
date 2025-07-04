import { createAsyncThunk, createSlice, createSelector } from '@reduxjs/toolkit';
import { getHistoryAsync } from '../../app/appSlice';
import { historyEntryTimestamp } from '../../../utils';

const initialState = {
    dewpoint: null,
    humidity: null,
    temperature: null,
    history: [],
};

export const selectAmbient = state => state.ambient;
export const selectAmbientHistory = createSelector([selectAmbient], ambient => ambient.history.map(entry => ({...entry, name: new Date(entry.timestamp).toLocaleTimeString()})));

export const ambientSlice = createSlice({
  name: 'ambient',
  initialState,
  reducers: {
    setAmbient: (state, action) => {
        const { dewpoint, humidity, temperature } = action.payload;
        state.dewpoint = dewpoint;
        state.humidity = humidity;
        state.temperature = temperature;
        state.history = [...state.history, { timestamp: new Date().getTime(), temperature, humidity, dewpoint}]
    },
  },
  extraReducers: (builder) => {
    builder
      .addCase(getHistoryAsync.fulfilled, (state, { payload }) => {
        if(!payload) {
          return;
        }
        const { now: bootSeconds, entries } = payload;
        entries.forEach( ({uptime, ambientTemperature: temperature, ambientHumidity: humidity, ambientDewpoint: dewpoint }) => {
          if(temperature !== null && humidity !== null ) {
            state.history = [...state.history, { timestamp: historyEntryTimestamp(bootSeconds, uptime), temperature, humidity, dewpoint}]
          }
        })
      })
  }
});
 
export default ambientSlice.reducer;
export const { setAmbient } = ambientSlice.actions;