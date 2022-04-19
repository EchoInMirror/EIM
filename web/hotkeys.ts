export default {
  PLAY_OR_PAUSE: 'space',
  UNDO: 'ctrl+z',
  REDO: 'ctrl+y',
  SAVE: 'ctrl+s'
}

export const defaultHandlers = {
  PLAY_OR_PAUSE: () => $client.rpc.setProjectStatus({ isPlaying: !$globalData.isPlaying }),
  UNDO: () => $client.rpc.undo({ }),
  REDO: () => $client.rpc.redo({ }),
  SAVE: (e: any) => {
    e.preventDefault()
    $client.rpc.save({ }).then(console.log)
  }
}
