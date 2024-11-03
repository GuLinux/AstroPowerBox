import { configureStore } from '@reduxjs/toolkit';
import ambientReducer from '../features/sensors/ambient/ambientSlice';
import heatersReducer from '../features/sensors/heaters/heatersSlice';
import powerReducer from '../features/sensors/power/powerSlice';
import appReducer from '../features/app/appSlice';
import configReducer from '../features/app/configSlice';
import { createLogger } from 'redux-logger'

const logger = createLogger({});

export const store = configureStore({
  reducer: {
    ambient: ambientReducer,
    heaters: heatersReducer,
    power: powerReducer,
    app: appReducer,
    config: configReducer,
  },
  // middleware: (getDefaultMiddleware) => getDefaultMiddleware().concat(logger),
  devTools: true,
});
