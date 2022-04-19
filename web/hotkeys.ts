export default {
  PLAY_OR_PAUSE: 'space',
  UNDO: 'ctrl+z',
  REDO: 'ctrl+y'
}

export const defaultHandlers = {
  PLAY_OR_PAUSE: () => $client.rpc.setProjectStatus({ isPlaying: !$globalData.isPlaying }),
  UNDO: () => $client.rpc.undo({ }),
  REDO: () => $client.rpc.redo({ })
}
