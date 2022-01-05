import './SideBar.less'
import React from 'react'
import { Paper, useTheme, alpha, Toolbar } from '@mui/material'
import { FavoriteOutlined, SettingsInputHdmiOutlined } from '@mui/icons-material'

// eslint-disable-next-line react/jsx-key
const icons = [<FavoriteOutlined />, <SettingsInputHdmiOutlined />]

const AppBar: React.FC = () => {
  const theme = useTheme()
  return (
    <nav className='side-bar' style={{ zIndex: theme.zIndex.appBar - 1 }}>
      <Paper square elevation={3} sx={{ background: theme.palette.background.light }}>
        <Toolbar />
        <ul>
          {icons.map((icon, i) => (
            <li key={i} style={i ? undefined : { boxShadow: 'inset 2px 0px ' + theme.palette.primary.main, backgroundColor: alpha(theme.palette.primary.main, 0.2) }}>
              {icon}
            </li>
          ))}
        </ul>
      </Paper>
      <div className='explorer'>
        <Toolbar />
        Explorer
      </div>
    </nav>
  )
}

export default AppBar
