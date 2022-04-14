import { colors } from '@mui/material'

export const colorValues: string[] = []
export const colorMap: Record<string, string> = { }
for (const name in colors) if (name !== 'grey' && name !== 'blueGrey' && name !== 'common' && name !== 'yellow') colorValues.push(colorMap[name] = (colors as any)[name][400])

export const merge = <T> (source: any, target: T) => {
  for (const key in source) if (source[key] != null) (target as any)[key] = source[key]
  return target
}
