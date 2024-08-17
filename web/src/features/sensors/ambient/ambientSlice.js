import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { getHistoryAsync } from '../../app/appSlice';

const initialState = {
    dewpoint: null,
    humidity: null,
    temperature: null,
    history: [],
};

export const selectAmbient = state => state.ambient;
export const selectAmbientHistory = state => state.ambient.history.map(entry => ({...entry, name: new Date(entry.timestamp).toLocaleTimeString()}))

export const ambientSlice = createSlice({
  name: 'ambient',
  initialState,
  reducers: {
    setAmbient: (state, action) => {
        const { dewpoint, humidity, temperature } = action.payload;
        state.dewpoint = dewpoint;
        state.humidity = humidity;
        state.temperature = temperature;
        state.history = [...state.history, { timestamp: new Date().getTime(), temperature, humidity, dewpoint}].slice(-1000)
    },
  },
  extraReducers: (builder) => {
    builder
      .addCase(getHistoryAsync.fulfilled, (state, { payload }) => {
        if(!payload) {
          return;
        }
        const { now: bootSeconds, entries } = payload;
        const dateStarted = new Date().getTime() - (bootSeconds * 1000);
        entries.forEach( ({uptime, ambientTemperature, ambientHumidity, ambientDewpoint }) => {
          state.history = [...state.history, { timestamp: dateStarted + uptime, temperature: ambientTemperature, humidity: ambientHumidity, dewpoint: ambientDewpoint}]
        })
      })

  }
});
 
export default ambientSlice.reducer;
export const { setAmbient } = ambientSlice.actions;