import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';

const initialState = {
    busVoltage: null,
    current: null,
    power: null,
    shuntVoltage: null,
};

export const selectPower = state => state.power;

export const powerSlice = createSlice({
  name: 'power',
  initialState,
  reducers: {
    setPower: (state, action) => {
        const { busVoltage, current, power, shuntVoltage } = action.payload;
        state.busVoltage = busVoltage;
        state.current = current;
        state.power = power;
        state.shuntVoltage = shuntVoltage;
    },
  },
});
 
export default powerSlice.reducer;
export const { setPower } = powerSlice.actions;