import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';

const initialState = {
    dewpoint: null,
    humidity: null,
    temperature: null,
};

export const selectAmbient = state => state.ambient;

export const ambientSlice = createSlice({
  name: 'ambient',
  initialState,
  reducers: {
    setAmbient: (state, action) => {
        const { dewpoint, humidity, temperature } = action.payload;
        state.dewpoint = dewpoint;
        state.humidity = humidity;
        state.temperature = temperature;
    },
  },
});
 
export default ambientSlice.reducer;
export const { setAmbient } = ambientSlice.actions;