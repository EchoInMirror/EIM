import React, { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { initialState } from './reducer'

import App from './components/App'
import Client from './Client'

window.$dispatch = () => console.warn('Not initialized!')
window.$globalData = { ...initialState }
const root = createRoot(document.getElementById('root')!)
window.$client = new Client('ws://127.0.0.1:8088', () => root.render(<StrictMode><App /></StrictMode>))
document.addEventListener('dragend', () => (window.$dragObject = undefined))
document.addEventListener('keypress', e => {
  if (e.code !== 'Space' || e.target instanceof HTMLInputElement) return
  $client.rpc.setProjectStatus({ isPlaying: !$globalData.isPlaying })
})
