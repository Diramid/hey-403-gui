const { exec } = require('child_process');
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const os = require('os');

function createWindow() {
  const win = new BrowserWindow({
    width: 600,
    height: 600,
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      preload: path.join(__dirname, 'preload.js'),
    },
  });

  win.setMenuBarVisibility(false);
  win.loadFile('src/index.html');
}

ipcMain.on('setdns', (event, command) => { 
  exec(command, (err, stdout, stderr) => {
    if (err) {
      event.reply('command-output', `Error: ${err.message}`);
      return;
    }
    if (stderr) {
      event.reply('command-output', `Stderr: ${stderr}`);
      return;
    }
    event.reply('command-output', stdout);
  });
});

ipcMain.handle('getos', async () => {
  return os.platform();
});

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