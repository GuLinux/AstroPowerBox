import { createAsyncThunk, createSlice } from '@reduxjs/toolkit';
import { fetchHeaters } from './api';

const initialState = { heaters: [] };

export const getHeatersAsync = createAsyncThunk(
  'heaters/getHeaters',
  async () => {
    const response = await fetchHeaters();
    console.log(response)
    return response
  }
);


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
        console.log(action.payload, state)
        state.heaters = action.payload
      });
  },
});
 
export default heatersSlice.reducer;
export const { setHeaters, updateHeaters } = heatersSlice.actions;