import React, { useEffect, useState } from 'react'
import { ClientboundPacket } from '../../packets'
import { Config } from '../Client'
import {
  Dialog, DialogTitle, Tabs, Tab, CircularProgress, IconButton, colors, List, ListItem, ListItemText,
  ListSubheader, ListItemSecondaryAction, DialogActions, Button, Divider, FormControl, InputLabel,
  Select, MenuItem, Switch, FormControlLabel, Typography, ListItemButton, ListItemIcon, Checkbox,
  TextField
} from '@mui/material'

import Close from '@mui/icons-material/Close'
import Delete from '@mui/icons-material/Delete'
import Block from '@mui/icons-material/Block'

const scanningFiles: Record<string, number> = { }
let currentFile = 0
let allFiles = 0
const Settings: React.FC<{ open: boolean, setOpen: (val: boolean) => void }> = ({ open, setOpen }) => {
  const [tab, setTab] = useState(0)
  const [config, setConfig] = useState<Config>()
  const [, update] = useState(0)

  useEffect(() => {
    $client.rpc.config({ }).then(it => setConfig(JSON.parse(it.value!)))
  }, [open])

  useEffect(() => {
    $client.on(ClientboundPacket.SetScanningVST, data => {
      if (data.isFinished) {
        delete scanningFiles[data.file!]
      } else {
        allFiles = data.count!
        currentFile = data.current!
        scanningFiles[data.file!] = data.thread!
      }
      update(i => i + 1)
    })
  }, [])

  const onClose = () => setOpen(false)

  const files = Object.entries(scanningFiles).map(([it, value]) => (
    <ListItem key={it}>
      <ListItemText primary={it} />
      <ListItemSecondaryAction>
        <IconButton edge='end' onClick={() => $client.rpc.skipScanning({ value })}><Block /></IconButton>
      </ListItemSecondaryAction>
    </ListItem>
  ))

  return (
    <Dialog onClose={onClose} open={open} sx={{ '& .MuiDialog-paper': { minWidth: 180, minHeight: 200 } }}>
      <DialogTitle>
        首选项
        <IconButton
          onClick={onClose}
          sx={{
            position: 'absolute',
            right: 8,
            top: 8,
            color: colors.grey[500]
          }}
        >
          <Close />
        </IconButton>
      </DialogTitle>
      {config
        ? (
          <>
            <div style={{ display: 'flex' }}>
              <Tabs orientation='vertical' value={tab} onChange={(_, val) => setTab(val)} sx={{ borderRight: 1, borderColor: 'divider' }}>
                <Tab label='常规' />
                <Tab label='音频' />
                <Tab label='插件' />
              </Tabs>
              {tab === 0 && (
                <div>
                  <FormControl variant='standard' sx={{ m: 1, flex: 1 }}>
                    <InputLabel id='eim-settings-language-label'>语言</InputLabel>
                    <Select
                      labelId='eim-settings-language-label'
                      value={1}
                      label='语言'
                    >
                      <MenuItem value={0}>English</MenuItem>
                      <MenuItem value={1}>简体中文</MenuItem>
                    </Select>
                  </FormControl>
                  <TextField
                    label='自动保存时间间隔 (分钟)'
                    type='number'
                    value={10}
                    sx={{ m: 1 }}
                    InputLabelProps={{ shrink: true }}
                    variant='standard'
                  />
                  <Divider />
                  <List dense subheader={<ListSubheader component='div'>MIDI 输入设备</ListSubheader>}>
                    <ListItem>
                      <ListItemButton role={undefined} dense>
                        <ListItemIcon>
                          <Checkbox
                            edge='start'
                            tabIndex={-1}
                            disableRipple
                          />
                        </ListItemIcon>
                        <ListItemText primary='My First MIDI Keyboard' />
                      </ListItemButton>
                    </ListItem>
                  </List>
                </div>
              )}
              {tab === 1 && (
                <div>
                  <div style={{ display: 'flex' }}>
                    <FormControl variant='standard' sx={{ m: 1, flex: 1 }}>
                      <InputLabel id='eim-settings-audio-device-label'>输入/输出设备</InputLabel>
                      <Select
                        labelId='eim-settings-audio-device-label'
                        value={0}
                        label='输入/输出设备'
                      >
                        <ListSubheader>DirectSound 设备</ListSubheader>
                        <MenuItem value={0}>主声音驱动程序</MenuItem>
                        <ListSubheader>ASIO 设备</ListSubheader>
                        <MenuItem value={1}>ASIO4ALL</MenuItem>
                      </Select>
                    </FormControl>
                    <FormControlLabel control={<Switch />} label='自动关闭' />
                  </div>
                  <Typography variant='caption' sx={{ m: 2, whiteSpace: 'nowrap' }}>
                    采样率: 44100 Hz, 缓冲区大小: 1024, 输出: 2 个通道, 输入: 2 个通道, 延迟: 6458 个采样
                  </Typography>
                </div>
              )}
              {tab === 2 && (
                <List dense>
                  <ListSubheader>插件扫描路径</ListSubheader>
                  {config.pluginManager.scanPaths.map(it => (
                    <ListItem key={it}>
                      <ListItemText primary={it} />
                      <ListItemSecondaryAction>
                        <IconButton edge='end'><Delete /></IconButton>
                      </ListItemSecondaryAction>
                    </ListItem>
                  ))}
                  <Divider />
                  <ListSubheader>忽略扫描的文件列表</ListSubheader>
                  {config.pluginManager.skipFiles.map(it => (
                    <ListItem key={it}>
                      <ListItemText primary={it} />
                      <ListItemSecondaryAction>
                        <IconButton edge='end'><Delete /></IconButton>
                      </ListItemSecondaryAction>
                    </ListItem>
                  ))}
                  {files.length
                    ? (
                      <>
                        <Divider />
                        <ListSubheader>扫描中 ({currentFile}/{allFiles})</ListSubheader>
                        {files}
                      </>)
                    : undefined}
                </List>
              )}
            </div>
            {tab === 0 && (
              <DialogActions>
                <Button color='primary'>重新扫描 MIDI 设备</Button>
              </DialogActions>
            )}
            {tab === 1 && (
              <DialogActions>
                <Button color='primary'>打开 ASIO 面板</Button>
              </DialogActions>
            )}
            {tab === 2 && (
              <DialogActions>
                <Button color='primary'>添加扫描路径</Button>
                <Button onClick={() => $client.rpc.scanVSTs({ })} color='primary'>扫描插件</Button>
              </DialogActions>
            )}
          </>
          )
        : <CircularProgress sx={{ margin: 'auto', position: 'relative', top: '-20px' }} />}
    </Dialog>
  )
}

export default Settings
