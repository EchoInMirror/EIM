import { colors } from '@mui/material'

export const colorValues: string[] = []
export const colorMap: Record<string, string> = { }
for (const name in colors) if (name !== 'grey' && name !== 'blueGrey' && name !== 'common' && name !== 'yellow') colorValues.push(colorMap[name] = (colors as any)[name][400])

export const merge = <T> (source: any, target: T) => {
  for (const key in source) if (source[key] != null) (target as any)[key] = source[key]
  return target
}

export const BACKEND_PATH = '//' + (process.env.NODE_ENV === 'development' ? '127.0.0.1:8088' : location.host) + '/'
export const FULL_BACKEND_PATH = location.protocol + BACKEND_PATH

export const allowAudioExtensions = ['.wav', '.flac', '.mp3', '.ogg']
export const keyNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
export const levelMarks = [ // Math.sqrt(10 ** (db * 0.05)) * 100
  { value: 100, label: '0' },
  { value: 70.7945784384138, label: '-6' },
  { value: 50.11872336272722, label: '-12' }
]

const images: Record<string, Promise<void>> = {}
export const tryLoadImage = (url: string) => {
  if (!images[url]) {
    let fn: () => void
    images[url] = new Promise(resolve => (fn = resolve))
    const tryLoad = () => {
      const img = new Image()
      img.onload = () => fn()
      img.onerror = () => setTimeout(tryLoad, 300)
      img.src = url
    }
    tryLoad()
  }
  return images[url]
}

export enum FileChooserFlags {
  // specifies that the component should allow the user to choose an existing file with the intention of opening it.
  openMode = 1,
  // specifies that the component should allow the user to specify the name of a file that will be used to save something.
  saveMode = 2,
  // specifies that the user can select files (can be used in conjunction with canSelectDirectories).
  canSelectFiles = 4,
  // specifies that the user can select directories (can be used in conjunction with canSelectFiles).
  canSelectDirectories = 8,
  // specifies that the user can select multiple items.
  canSelectMultipleItems = 16,
  // specifies that a tree-view should be shown instead of a file list.
  useTreeView = 32,
  // specifies that the user can't type directly into the filename box.
  filenameBoxIsReadOnly = 64,
  // specifies that the dialog should warn about overwriting existing files (if possible).
  warnAboutOverwriting = 128,
  // specifies that the file name should not be cleared upon root change.
  doNotClearFileNameOnRootChange = 256
}
