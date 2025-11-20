const { app, BrowserWindow, ipcMain } = require("electron");
const path = require("path");

// --- SerialPort imports ---
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");

// --- Global variables ---
let port = null;
let parser = null;
let mainWindow = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1000,
    height: 700,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  mainWindow.loadFile("index.html");
}

app.whenReady().then(createWindow);


// ----------------------
// Clean shutdown
// ----------------------
app.on("window-all-closed", () => {
  if (port && port.isOpen) port.close();
  if (process.platform !== "darwin") app.quit();
});


// ----------------------
// IPC: List available serial ports
// ----------------------
ipcMain.handle("list-ports", async () => {
  const ports = await SerialPort.list();
  return ports.map(p => ({
    path: p.path,
    manufacturer: p.manufacturer || ""
  }));
});


// ----------------------
// IPC: Open a serial port
// ----------------------
ipcMain.handle("open-port", async (event, { path, baudRate = 115200 }) => {
  // If already open, close first
  if (port && port.isOpen) {
    try { port.close(); } catch (_) {}
  }

  // Create new port
  port = new SerialPort({
    path,
    baudRate,
    autoOpen: false
  });

  parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));

  return new Promise((resolve, reject) => {
    port.open(err => {
      if (err) return reject(err.message);

      parser.on("data", onSerialData);

      port.on("error", e =>
        mainWindow.webContents.send("serial-error", e.message)
      );

      port.on("close", () =>
        mainWindow.webContents.send("serial-closed")
      );

      resolve("ok");
    });
  });
});


// ----------------------
// IPC: Close serial port
// ----------------------
ipcMain.handle("close-port", async () => {
  if (!port) return "no-port";

  return new Promise(resolve => {
    if (parser) parser.removeAllListeners("data");
    port.close(() => resolve("closed"));
  });
});


// ----------------------
// Handle each line of serial data
// ----------------------
function onSerialData(line) {
  line = line.trim();
  if (!line) return;

  try {
    const obj = JSON.parse(line);
    mainWindow.webContents.send("sensor-update", obj);
  } catch (e) {
    mainWindow.webContents.send("serial-raw", line);
  }
}
