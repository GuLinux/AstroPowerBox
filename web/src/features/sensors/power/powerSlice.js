import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { getHistoryAsync } from '../../app/appSlice';
import { historyEntryTimestamp } from '../../../utils';

const initialState = {
    busVoltage: null,
    current: null,
    power: null,
    shuntVoltage: null,
    history: [],
};

export const selectPower = state => state.power;
export const selectPowerHistory = state => state.power.history.map(entry => ({...entry, name: new Date(entry.timestamp).toLocaleTimeString()}))

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
        state.history = [...state.history, { timestamp: new Date().getTime(), busVoltage, current, power}]
    },
  },
  extraReducers: (builder) => {
    builder
      .addCase(getHistoryAsync.fulfilled, (state, { payload }) => {
        if(!payload) {
          return;
        }
        const { now: bootSeconds, entries } = payload;
        entries.forEach( ({uptime, busVoltage, current, power }) => {
          state.history = [...state.history, { timestamp: historyEntryTimestamp(bootSeconds, uptime), busVoltage, power, current}]
        })
      })
  }
});
 
export default powerSlice.reducer;
export const { setPower } = powerSlice.actions;