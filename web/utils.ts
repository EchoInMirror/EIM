import { colors } from '@mui/material'

export const colorValues: string[] = []
export const colorMap: Record<string, string> = { }
for (const name in colors) if (name !== 'grey' && name !== 'blueGrey') colorValues.push(colorMap[name] = (colors as any)[name][400])
