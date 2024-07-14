import { createAsyncThunk, createSelector, createSlice } from '@reduxjs/toolkit';

const initialState = {
    tab: 'home',
    darkMode: true,
};

export const tabSelector = state => state.app.tab
export const darkModeSelector = state => state.app.darkMode

export const appSlice = createSlice({
  name: 'app',
  initialState,
  reducers: {
    setTab: (state, action) => { state.tab = action.payload },
    setDarkMode: (state, action) => { state.darkMode = action.payload },
  },
});
 
export default appSlice.reducer;
export const { setTab, setDarkMode } = appSlice.actions;