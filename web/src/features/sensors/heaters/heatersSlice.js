import { createAsyncThunk, createSelector, createSlice } from '@reduxjs/toolkit';
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

const transformDuty = heater => {
  return {...heater, duty: heater.active ? heater.duty : 0}
}
const addHeatersToHistory = (state) => {
  state.history = [...state.history, { timestamp: new Date().getTime(), heaters: state.heaters.map(transformDuty) }]
}
const onHeatersReceived = (state, payload) => {
  state.heaters = payload;
  addHeatersToHistory(state)
}

export const selectHeaters = state => state.heaters.heaters;
export const selectHeatersCount = createSelector([selectHeaters], heaters => heaters.length)
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
      .addCase(setHeaterAsync.rejected, (state, action) => console.log(action.error.response))
      .addCase(getHistoryAsync.fulfilled, (state, { payload }) => {
        if(!payload) {
          return;
        }
        const { now: bootSeconds, entries } = payload;
        entries.forEach( ({uptime,  heaters}) => {
          if(heaters.length > 0) {
            state.history = [...state.history, { timestamp: historyEntryTimestamp(bootSeconds, uptime), heaters }]
          }
        })
      })
  },
});
 
export default heatersSlice.reducer;
export const { setHeaters, updateHeaters } = heatersSlice.actions;