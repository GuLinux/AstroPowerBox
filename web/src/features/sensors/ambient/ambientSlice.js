import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';

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
});
 
export default ambientSlice.reducer;
export const { setAmbient } = ambientSlice.actions;