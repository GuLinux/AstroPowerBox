import { configureStore } from '@reduxjs/toolkit';
import ambientReducer from '../features/ambientSlice';
import heatersReducer from '../features/heatersSlice';
import powerReducer from '../features/powerSlice';
import { createLogger } from 'redux-logger'

const logger = createLogger({});

export const store = configureStore({
  reducer: {
    ambient: ambientReducer,
    heaters: heatersReducer,
    power: powerReducer,
  },
  middleware: (getDefaultMiddleware) => getDefaultMiddleware().concat(logger),
  devTools: true,
});
