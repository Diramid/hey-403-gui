const { contextBridge, ipcRenderer } = require('electron');

// Expose a safe function for the renderer to call
contextBridge.exposeInMainWorld('electron', {
  getdns: () => ipcRenderer.invoke('getdns')
});
