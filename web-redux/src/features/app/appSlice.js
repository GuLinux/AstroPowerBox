import { createAsyncThunk, createSelector, createSlice } from '@reduxjs/toolkit';

const initialState = {
    tab: 'home',
};

export const tabSelector = state => state.app.tab

export const appSlice = createSlice({
  name: 'app',
  initialState,
  reducers: {
    setTab: (state, action) => {
        const tab = action.payload;
        state.tab= tab;
    },
  },
});
 
export default appSlice.reducer;
export const { setTab } = appSlice.actions;