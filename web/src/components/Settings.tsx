import React, { useEffect, useState } from 'react'
// import { useSnackbar, SnackbarKey } from 'notistack'
import { Config } from '../Client'
import { Dialog, DialogTitle, Tabs, Tab, CircularProgress, IconButton, colors } from '@mui/material'

import Close from '@mui/icons-material/Close'

const Settings: React.FC<{ open: boolean, setOpen: (val: boolean) => void }> = ({ open, setOpen }) => {
  const [tab, setTab] = useState(0)
  const [config, setConfig] = useState<Config>()
  // const { enqueueSnackbar, closeSnackbar } = useSnackbar()

  useEffect(() => {
    $client.config().then(it => setConfig(JSON.parse(it)))
  }, [])
  const onClose = () => setOpen(false)

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
              <Tabs orientation='vertical' value={tab} onChange={(_, val) => val === 2 ? $client.openPluginManager() : setTab(val)} sx={{ borderRight: 1, borderColor: 'divider' }}>
                <Tab label='常规' />
                <Tab label='音频' />
                <Tab label='插件管理器' />
              </Tabs>
            </div>
          </>
          )
        : <CircularProgress sx={{ margin: 'auto', position: 'relative', top: '-20px' }} />}
    </Dialog>
  )
}

export default Settings
