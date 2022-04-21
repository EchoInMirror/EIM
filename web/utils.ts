import { colors } from '@mui/material'

export const colorValues: string[] = []
export const colorMap: Record<string, string> = { }
for (const name in colors) if (name !== 'grey' && name !== 'blueGrey' && name !== 'common' && name !== 'yellow') colorValues.push(colorMap[name] = (colors as any)[name][400])

export const merge = <T> (source: any, target: T) => {
  for (const key in source) if (source[key] != null) (target as any)[key] = source[key]
  return target
}

export const keyNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
export const levelMarks = [ // Math.sqrt(10 ** (db * 0.05)) * 100
  { value: 100, label: '0' },
  { value: 70.7945784384138, label: '-6' },
  { value: 50.11872336272722, label: '-12' }
]
