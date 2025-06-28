import { createAsyncThunk, createSelector, createSlice } from '@reduxjs/toolkit';
import { fetchPWMOutputs, setPWMOutput } from '../../app/api';
import { historyEntryTimestamp } from '../../../utils';
import { getHistoryAsync } from '../../app/appSlice';
import { selectPowerHistory } from '../power/powerSlice';

const initialState = {
  pwmOutputs: [],
  history: [],
};

export const getPWMOutputsAsync = createAsyncThunk(
  'pwmOutputs/getPWMOutputs',
  async () => await fetchPWMOutputs()
);

export const setPWMOutputAsync = createAsyncThunk(
  'pwmOutputs/setPWMOutput',
  async ({index, pwmOutput}) => await setPWMOutput(index, pwmOutput)
)

const transformDuty = pwmOutput => {
  return {...pwmOutput, duty: pwmOutput.active ? pwmOutput.duty : 0}
}
const addPWMOutputsToHistory = (state) => {
  state.history = [...state.history, { timestamp: new Date().getTime(), pwmOutputs: state.pwmOutputs.map(transformDuty) }]
}
const onPWMOutputsReceived = (state, payload) => {
  state.pwmOutputs = payload;
  addPWMOutputsToHistory(state)
}

const addIndexToPWMOutput = (pwmOutput, index) => ({...pwmOutput, index});
const pwmOutputsByType = (pwmOutputs, type) => pwmOutputs.filter(pwmOutput => pwmOutput.type === type);

export const selectPWMOutputs = state => state.pwmOutputs.pwmOutputs;
export const selectPWMOutputsWithIndex = createSelector([selectPWMOutputs], (pwmOutputs) => pwmOutputs.map(addIndexToPWMOutput));
export const selectPWMOutputsByType = createSelector([selectPWMOutputsWithIndex, (state, type) => type], pwmOutputsByType);

export const selectPWMOutputsHistory = state => state.pwmOutputs.history;
export const selectPWMOutputsHistoryAsMap = createSelector([selectPWMOutputsHistory, (state, type) => type], (history, type) => {
  const indexes = new Set();
  const mappedHistory = history.map(entry => {
    const pwmOutputs = pwmOutputsByType(entry.pwmOutputs.map(addIndexToPWMOutput), type);
    const mappedEntry = { name: new Date(entry.timestamp).toLocaleTimeString(), pwmOutputs: pwmOutputs.length }
    pwmOutputs.forEach((pwmOutput) => {
      indexes.add(pwmOutput.index);
      Object.keys(pwmOutput).forEach(key => {
        mappedEntry[`pwmOutput-${pwmOutput.index}-${key}`] = pwmOutput[key]
      })
      
    })
    return mappedEntry;
  });
  return { history: mappedHistory, indexes }
});


export const pwmOutputsSlice = createSlice({
  name: 'pwmOutputs',
  initialState,
  reducers: {
    updatePWMOutputs: (state, action) => {
        action.payload.forEach((pwmOutput, index) => state.pwmOutputs[index] = pwmOutput);
        addPWMOutputsToHistory(state)
    }
  },
  extraReducers: (builder) => {
    builder
      .addCase(getPWMOutputsAsync.fulfilled, (state, action) => onPWMOutputsReceived(state, action.payload))
      .addCase(setPWMOutputAsync.fulfilled, (state, action) => onPWMOutputsReceived(state, action.payload))
      .addCase(setPWMOutputAsync.rejected, (state, action) => console.log(action.error.response))
      .addCase(getHistoryAsync.fulfilled, (state, { payload }) => {
        if(!payload) {
          return;
        }
        const { now: bootSeconds, entries } = payload;
        entries.forEach( ({uptime,  pwmOutputs}) => {
          if(pwmOutputs.length > 0) {
            state.history = [...state.history, { timestamp: historyEntryTimestamp(bootSeconds, uptime), pwmOutputs }]
          }
        })
      })
  },
});
 
export default pwmOutputsSlice.reducer;
export const { setPWMOutputs, updatePWMOutputs } = pwmOutputsSlice.actions;