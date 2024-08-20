import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { fetchHeaters, setHeater } from '../../app/api';
import { historyEntryTimestamp } from '../../../utils';
import { getHistoryAsync } from '../../app/appSlice';

const initialState = {
  heaters: [],
  history: [],
};

export const getHeatersAsync = createAsyncThunk(
  'heaters/getHeaters',
  async () => await fetchHeaters()
);

export const setHeaterAsync = createAsyncThunk(
  'heaters/setHeater',
  async ({index, heater}) => await setHeater(index, heater)
)

const addHeatersToHistory = (state) => {
  state.history = [...state.history, { timestamp: new Date().getTime(), heaters: state.heaters }].slice(-1000)
}
const onHeatersReceived = (state, payload) => {
  state.heaters = payload;
  addHeatersToHistory(state)
}

export const selectHeaters = state => state.heaters;
export const selectHeatersHistory = state => state.heaters.history.map(entry => {
  const mappedEntry = { name: new Date(entry.timestamp).toLocaleTimeString(), heaters: entry.heaters.length }
  entry.heaters.forEach((heater, index) => {
    Object.keys(heater).forEach(key => {
      mappedEntry[`heater-${index}-${key}`] = heater[key]
    })
    
  })
  return mappedEntry;
});


export const heatersSlice = createSlice({
  name: 'heaters',
  initialState,
  reducers: {
    updateHeaters: (state, action) => {
        action.payload.forEach((heater, index) => state.heaters[index] = heater);
        addHeatersToHistory(state)
    }
  },
  extraReducers: (builder) => {
    builder
      .addCase(getHeatersAsync.fulfilled, (state, action) => onHeatersReceived(state, action.payload))
      .addCase(setHeaterAsync.fulfilled, (state, action) => onHeatersReceived(state, action.payload))
      .addCase(getHistoryAsync.fulfilled, (state, { payload }) => {
        if(!payload) {
          return;
        }
        const { now: bootSeconds, entries } = payload;
        entries.forEach( ({uptime,  heaters}) => {
          state.history = [...state.history, { timestamp: historyEntryTimestamp(bootSeconds, uptime), heaters }]
        })
      })
  },
});
 
export default heatersSlice.reducer;
export const { setHeaters, updateHeaters } = heatersSlice.actions;