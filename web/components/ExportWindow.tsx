import './ExportWindow.less'
import React, { useEffect } from 'react'
import packets, { ClientboundPacket } from '../../packets'
import { FileChooserFlags } from '../utils'
import {
  Box, Button, styled, Dialog, DialogTitle, DialogContent, DialogActions, IconButton, Typography, LinearProgress,
  LinearProgressProps, Select, MenuItem, FormControl, SelectChangeEvent, TextField, InputAdornment
} from '@mui/material'
import CloseIcon from '@mui/icons-material/Close'
import BrowserUpdatedIcon from '@mui/icons-material/BrowserUpdated'

function LinearProgressWithLabel (props: LinearProgressProps & { value: number }) {
  return (
    <Box sx={{
      display: 'flex',
      alignItems: 'center'
    }}
    >
      <Box sx={{
        width: '100%',
        mr: 1
      }}
      >
        <LinearProgress variant='determinate' {...props} />
      </Box>
      <Box sx={{ minWidth: 35 }}>
        <Typography variant='body2' color='text.secondary'>{`${Math.round(
          props.value
        )}%`}
        </Typography>
      </Box>
    </Box>
  )
}

// function LinearWithValueLabel () {
//   const [progress, setProgress] = React.useState(10)
//
//   React.useEffect(() => {
//     const timer = setInterval(() => {
//       setProgress((prevProgress) => (prevProgress >= 100 ? 10 : prevProgress + 10))
//     }, 800)
//     return () => {
//       clearInterval(timer)
//     }
//   }, [])
//
//   return (
//     <Box sx={{ width: '100%' }}>
//       <LinearProgressWithLabel value={progress} />
//     </Box>
//   )
// }

const BootstrapDialog = styled(Dialog)(({ theme }) => ({
  '& .MuiDialogContent-root': {
    paddingTop: theme.spacing(2),
    paddingBottom: theme.spacing(2),
    paddingLeft: theme.spacing(6),
    paddingRight: theme.spacing(6)
  },
  '& .MuiDialogActions-root': {
    padding: theme.spacing(1)
  }
}))

export interface DialogTitleProps {
  id: string;
  children?: React.ReactNode;
  onClose: () => void;
}

const BootstrapDialogTitle = (props: DialogTitleProps) => {
  const { children, onClose, ...other } = props

  return (
    <DialogTitle className='export-window-title' sx={{ m: 0, p: 2 }} {...other}>
      {children}
      {onClose
        ? (
          <IconButton
            aria-label='close'
            onClick={onClose}
            sx={{
              position: 'absolute',
              right: 8,
              top: 8,
              color: (theme) => theme.palette.grey[500]
            }}
          >
            <CloseIcon />
          </IconButton>
          )
        : null}
    </DialogTitle>
  )
}

const ExportWindow: React.FC<{ open: boolean, setOpen: (val: boolean) => void }> = ({
  open,
  setOpen
}) => {
  const [format, setFormat] = React.useState('WAV') // 格式
  const [bitDepth, setBitDepth] = React.useState('16') // 位深
  const [exportFilePath, setExportFilePath] = React.useState('') // 导出路径
  const [progress, setProgress] = React.useState(0) // 导出进度
  const [keepOpen, setKeepOpen] = React.useState(false) // 保持窗口不会被关闭

  // const handleClickOpen = () => {
  //   setOpen(true)
  // }
  const handleClose = () => {
    if (keepOpen) return
    setOpen(false)
  }

  const selectFormatChange = (event: SelectChangeEvent) => {
    setFormat(event.target.value)
  }

  const selectBitDepth = (event: SelectChangeEvent) => {
    setBitDepth(event.target.value)
  }

  const changeExportFilePath = () => {
    $client.browserPath({
      type: FileChooserFlags.saveMode,
      title: '选择导出位置',
      patterns: '*.wav'
    }).then(it => {
      if (!it) return
      if (it.endsWith('.wav')) setExportFilePath(it)
      else setExportFilePath(it + '.wav')
    })
  }

  const exportYourMusic = () => {
    if (exportFilePath === '') {
      $notice.enqueueSnackbar('请选择导出位置!', { variant: 'error' })
      return
    }

    $client.rpc.render({
      path: exportFilePath,
      sampleRate: 44100, // TODO 待改成全局变量
      bitsPreSample: +bitDepth
    }).then(r => {
      console.log('render', r)
    })

    setKeepOpen(true)
  }

  useEffect(() => {
    const handleRenderProgress = (data: packets.IClientboundRenderProgress) => {
      setProgress(data.progress! * 100)
      if (data.progress! >= 1 && data.progress! < 100) setKeepOpen(true)
      else setKeepOpen(false)
    }

    $client.on(ClientboundPacket.RenderProgress, handleRenderProgress)
    return () => { $client.off(ClientboundPacket.RenderProgress, handleRenderProgress) }
  }, [])

  return (
    <div className='export-window'>
      <BootstrapDialog
        onClose={handleClose}
        aria-labelledby='customized-dialog-title'
        open={open}
      >
        <BootstrapDialogTitle id='customized-dialog-title' onClose={handleClose}>
          导出
        </BootstrapDialogTitle>
        <DialogContent dividers>
          <div className='export-window-line'>
            <span>格式</span>
            <FormControl className='export-window-input' variant='standard' sx={{ m: 1, flex: 1 }}>
              <Select value={format} onChange={selectFormatChange}>
                <MenuItem value='WAV'>WAV</MenuItem>
              </Select>
            </FormControl>
          </div>

          {format === 'WAV' &&
            <div className='export-window-line'>
              <span>位深</span>
              <FormControl className='export-window-input' variant='standard' sx={{ m: 1, flex: 1 }}>
                <Select value={bitDepth} onChange={selectBitDepth}>
                  <MenuItem value='16'>16 Bit</MenuItem>
                  <MenuItem value='24'>24 Bit</MenuItem>
                  <MenuItem value='32'>32 Bit</MenuItem>
                </Select>
              </FormControl>
            </div>}

          <div className='export-window-line'>
            <span>导出位置</span>
            <TextField
              className='export-window-input'
              value={exportFilePath}
              InputProps={{
                readOnly: true,
                endAdornment: <InputAdornment position='end'><BrowserUpdatedIcon /></InputAdornment>
              }}
              variant='standard'
              onClick={changeExportFilePath}
            />
          </div>
        </DialogContent>
        <DialogActions>
          <Box className='export-window-linear-progress' sx={{ width: '100%' }}>
            <LinearProgressWithLabel value={progress} />
          </Box>
          <Button autoFocus onClick={exportYourMusic}>
            导出
          </Button>
        </DialogActions>
      </BootstrapDialog>
    </div>
  )
}

export default ExportWindow
