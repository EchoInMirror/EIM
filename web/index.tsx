import React from 'react'
import { render } from 'react-dom'
import { initialState } from './reducer'

import App from './components/App'
import Client from './Client'

window.$dispatch = () => console.warn('Not initialized!')
window.$globalData = { ...initialState }
window.$client = new Client('ws://127.0.0.1:8088')
document.addEventListener('dragend', () => (window.$dragObject = undefined))
render(<App />, document.getElementById('root'))
