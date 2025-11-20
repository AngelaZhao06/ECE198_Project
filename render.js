const portsSelect = document.getElementById("ports");
const refreshBtn = document.getElementById("refresh");
const connectBtn = document.getElementById("connect");
const disconnectBtn = document.getElementById("disconnect");
const dataBox = document.getElementById("data");

// --- Load available serial ports ---
async function loadPorts() {
  const ports = await window.api.listPorts();
  portsSelect.innerHTML = "";

  ports.forEach(p => {
    const option = document.createElement("option");
    option.value = p.path;
    option.textContent = `${p.path} (${p.manufacturer})`;
    portsSelect.appendChild(option);
  });
}

refreshBtn.onclick = loadPorts;
loadPorts();

// --- Connect to selected port ---
connectBtn.onclick = async () => {
  const portPath = portsSelect.value;
  if (!portPath) return alert("No port selected");

  try {
    await window.api.openPort(portPath);
    dataBox.innerHTML += "✓ Connected\n";
  } catch (e) {
    dataBox.innerHTML += "Error opening port: " + e + "\n";
  }
};

// --- Disconnect ---
disconnectBtn.onclick = async () => {
  await window.api.closePort();
  dataBox.innerHTML += "Disconnected\n";
};

// --- When JSON sensor data arrives ---
window.api.onSensorUpdate(data => {
  dataBox.innerHTML += JSON.stringify(data) + "\n";
  dataBox.scrollTop = dataBox.scrollHeight;
});

// --- When raw text arrives ---
window.api.onRaw(text => {
  dataBox.innerHTML += text + "\n";
  dataBox.scrollTop = dataBox.scrollHeight;
});

// --- On error ---
window.api.onError(msg => {
  dataBox.innerHTML += "Error: " + msg + "\n";
});
