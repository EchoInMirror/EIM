import './SideBar.less'
import React, { useState, useEffect } from 'react'
import TreeView from '@mui/lab/TreeView'
import TreeItem from '@mui/lab/TreeItem'
import { Paper, useTheme, alpha, Toolbar, Button } from '@mui/material'
import { FavoriteOutlined, SettingsInputHdmiOutlined, ExpandMore, ChevronRight } from '@mui/icons-material'

// eslint-disable-next-line react/jsx-key
const icons = [<FavoriteOutlined />, <SettingsInputHdmiOutlined />]

const Explorer: React.FC<{ type: number }> = ({ type }) => {
  useEffect(() => {

  }, [type])
  return (
    <TreeView
      defaultCollapseIcon={<ExpandMore />}
      defaultExpandIcon={<ChevronRight />}
      sx={{ flexGrow: 1, overflowY: 'auto' }}
    >
      <TreeItem nodeId='1' label="Applications">
        <TreeItem nodeId="2" label="Calendar" />
      </TreeItem>
      <TreeItem nodeId="5" label="Documents">
        <TreeItem nodeId="10" label="OSS" />
        <TreeItem nodeId="6" label="MUI">
          <TreeItem nodeId="8" label="index.js" />
        </TreeItem>
      </TreeItem>
    </TreeView>
  )
}

const AppBar: React.FC = () => {
  const theme = useTheme()
  const [type, setType] = useState(1)
  return (
    <nav className='side-bar' style={{ zIndex: theme.zIndex.appBar - 1 }}>
      <Paper square elevation={3} sx={{ background: theme.palette.background.bright }}>
        <Toolbar />
        <ul className='types'>
          {icons.map((icon, i) => (
            <Button
              key={i}
              component='li'
              onClick={() => setType(i)}
              sx={{
                color: i === type ? '#fff' : theme.palette.background.brightest,
                borderRadius: 0,
                boxShadow: i === type ? 'inset 2px 0px ' + theme.palette.primary.main : undefined,
                backgroundColor: i === type ? alpha(theme.palette.primary.main, 0.2) : undefined,
                '&:hover': {
                  color: '#fff',
                  backgroundColor: i === type ? alpha(theme.palette.primary.main, 0.2) : 'inherit'
                }
              }}
            >
              {icon}
            </Button>
          ))}
        </ul>
      </Paper>
      <div className='explorer'>
        <Toolbar />
        <Explorer type={type} key={type} />
      </div>
    </nav>
  )
}

export default AppBar
