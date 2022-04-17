import React, { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { initialState } from './reducer'
import { colorValues } from './utils'

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

;(window as any).$coll = (elm: HTMLElement, name: string) => {
  const rect = elm.getBoundingClientRect()
  const node = document.createElement('div')
  const color = colorValues[colorValues.length * Math.random() | 0]
  node.style.border = '1px dashed'
  node.style.borderColor = color
  node.style.borderRadius = '4px 0 4px 4px'
  node.style.position = 'fixed'
  node.style.zIndex = '100'
  node.style.textAlign = 'end'
  node.style.left = rect.left + 'px'
  node.style.top = rect.top + 'px'
  node.style.width = rect.width + 'px'
  node.style.height = rect.height + 'px'
  node.style.pointerEvents = 'none'
  const nameNode = document.createElement('div')
  nameNode.innerText = name
  nameNode.style.backgroundColor = color
  nameNode.style.transform = 'translateY(-100%)'
  nameNode.style.display = 'inline-block'
  nameNode.style.fontSize = '14px'
  nameNode.style.top = '-2px'
  nameNode.style.right = '-1px'
  nameNode.style.position = 'relative'
  nameNode.style.padding = '0 4px'
  nameNode.style.borderRadius = '4px 4px 0 0'
  node.appendChild(nameNode)
  document.body.appendChild(node)
}
