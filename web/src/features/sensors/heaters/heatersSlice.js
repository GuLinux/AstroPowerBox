import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { fetchHeaters, setHeater } from '../../app/api';

const initialState = { heaters: [] };

export const getHeatersAsync = createAsyncThunk(
  'heaters/getHeaters',
  async () => await fetchHeaters()
);

export const setHeaterAsync = createAsyncThunk(
  'heaters/setHeater',
  async ({index, heater}) => await setHeater(index, heater)
)
export const selectHeaters = state => state.heaters;

export const heatersSlice = createSlice({
  name: 'heaters',
  initialState,
  reducers: {
    updateHeaters: (state, action) => {
        const heaters = action.payload;
        state.heaters.map((_, index) => {
            state.heaters[index] = heaters[index]
        })
    }
  },
  extraReducers: (builder) => {
    builder
      .addCase(getHeatersAsync.fulfilled, (state, action) => {
        state.heaters = action.payload
      })
      .addCase(setHeaterAsync.fulfilled, (state, action) => {
        console.log(action)
        state.heaters = action.payload
      })
  },
});
 
export default heatersSlice.reducer;
export const { setHeaters, updateHeaters } = heatersSlice.actions;