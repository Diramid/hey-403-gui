const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electron', {
    setdns: (cmd) => ipcRenderer.send('setdns', cmd),
    onCommandOutput: (callback) => ipcRenderer.on('command-output', (event, data) => callback(data)),
    getos: () => ipcRenderer.invoke('getos')
});