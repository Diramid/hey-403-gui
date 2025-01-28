const { app, BrowserWindow, ipcMain } = require('electron');

const path = require('path');
const DNSFileCheck = require('./src/DNSFileCheck');

ipcMain.handle('getdns', async () => {
  try {
    return await DNSFileCheck();
  } catch (err) {
    return err.message;
  }
  
});

function createWindow() {
  const win = new BrowserWindow({
    width: 600,
    height: 600,
    webPreferences: {
      nodeIntegration: true,
      preload: path.join(__dirname, 'preload.js'), // Enable preload script
    },
  });
  win.setMenuBarVisibility(false)
  win.loadFile('src/index.html');
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});