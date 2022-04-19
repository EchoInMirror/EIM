export default {
  PLAY_OR_PAUSE: 'space',
  UNDO: 'command+z',
  REDO: 'command+y'
}

export const defaultHandlers = {
  PLAY_OR_PAUSE: () => $client.rpc.setProjectStatus({ isPlaying: !$globalData.isPlaying }),
  UNDO: () => $client.rpc.undo({ }),
  REDO: () => $client.rpc.redo({ })
}
