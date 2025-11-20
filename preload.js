const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("api", {
  listPorts: () => ipcRenderer.invoke("list-ports"),
  openPort: (path) => ipcRenderer.invoke("open-port", { path }),
  closePort: () => ipcRenderer.invoke("close-port"),

  onSensorUpdate: (cb) => ipcRenderer.on("sensor-update", (_, data) => cb(data)),
  onRaw: (cb) => ipcRenderer.on("serial-raw", (_, text) => cb(text)),
  onError: (cb) => ipcRenderer.on("serial-error", (_, msg) => cb(msg)),
});
