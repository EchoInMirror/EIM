import { app, BrowserWindow } from 'electron'
declare const MAIN_WINDOW_WEBPACK_ENTRY: any

app.allowRendererProcessReuse = false

if (require('electron-squirrel-startup')) {
  app.quit()
}

const createWindow = (): void => {
  const mainWindow = new BrowserWindow({
    height: 800,
    width: 1300,
    webPreferences: { nodeIntegration: true, enableRemoteModule: true, allowRunningInsecureContent: true }
  })

  mainWindow.loadURL(MAIN_WINDOW_WEBPACK_ENTRY)
  mainWindow.webContents.openDevTools()
}

app.on('ready', createWindow)

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow()
  }
})
