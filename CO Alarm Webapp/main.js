
let connectButton = document.getElementById('connect');
let disconnectButton = document.getElementById('disconnect');



const labels = [""];
const data = {
  labels: labels,
  datasets: [
    {
      label: 'Temperature',
      data: 0,
      backgroundColor: 'rgb(255, 255, 255)',
      borderColor: 'rgb(255, 99, 132)',
    },
    {
      label: 'Humidity',
      data: 0,
      backgroundColor: 'rgb(255, 255, 255)',
      borderColor: 'rgb(12, 12, 255)',
    },
    {
      label: 'Carbon Monoxide',
      data: 0,
      backgroundColor: 'rgb(255, 255, 255)',
      borderColor: 'rgb(12, 255, 12)',
    }
  ]
};

const config = {
  type: 'line',
  data: data,
  options: {
    responsive: true,
    plugins: {
      legend: {
        position: 'top',
      },
      title: {
        display: true,
        text: 'Carbon Monoxide Chart'
      }
    }
  }
};

var chart = new Chart(
  document.getElementById('chart'),
  config
);




connectButton.addEventListener('click', function() {
  connect();
});

disconnectButton.addEventListener('click', function() {
  disconnect();
});


let deviceCache = null;

let characteristicCache = null;

let readBuffer = '';


function connect() {
  return (deviceCache ? Promise.resolve(deviceCache) :
  requestBluetoothDevice()).
  then(device => connectDeviceAndCacheCharacteristic(device)).
  then(characteristic => startNotifications(characteristic));
}

function requestBluetoothDevice() {

  return navigator.bluetooth.requestDevice({
    filters: [{services: [0xFFE0]}],
  }).
  then(device => {
    deviceCache = device;
    deviceCache.addEventListener('gattserverdisconnected',
    handleDisconnection);

    return deviceCache;
  });
}

function handleDisconnection(event) {
  let device = event.target;

  connectDeviceAndCacheCharacteristic(device).
  then(characteristic => startNotifications(characteristic));
}

function connectDeviceAndCacheCharacteristic(device) {
  if (device.gatt.connected && characteristicCache) {
    return Promise.resolve(characteristicCache);
  }


  return device.gatt.connect().
  then(server => {

    return server.getPrimaryService(0xFFE0);
  }).
  then(service => {

    return service.getCharacteristic(0xFFE1);
  }).
  then(characteristic => {
    characteristicCache = characteristic;

    return characteristicCache;
  });
}

function startNotifications(characteristic) {

  return characteristic.startNotifications().
  then(() => {
    characteristic.addEventListener('characteristicvaluechanged',
    handleCharacteristicValueChanged);
  });
}

function handleCharacteristicValueChanged(event) {
  let value = new TextDecoder().decode(event.target.value);

  for (let c of value) {
    if (c === '\n') {
      let data = readBuffer.trim();
      readBuffer = '';

      if (data) {
        receive(data);
      }
    }
    else {
      readBuffer += c;
    }
  }
}

function receive(data) {
  switch(data[0]){
    case 'T':
    var temperatureReading = data.substring(1);
    document.getElementById("temp").innerHTML = parseFloat(temperatureReading);

    //Get Time
    var time = new Date();
    //Update graph=
    chart.data.labels.push(time.toLocaleTimeString());
    chart.data.datasets[0].data.push(temperatureReading);

    chart.update();

    break;
    case 'H':
    var humidityReading = data.substring(1);
    document.getElementById("humidity").innerHTML = parseFloat(humidityReading);


    //Get Time
    var time = new Date();
    //Update graph

    chart.data.datasets[1].data.push(humidityReading);


    chart.update();
    break;
    case 'C':

    //Get Reading
    var carbonReading = data.substring(1);
    //show reading value
    document.getElementById("coLevel").innerHTML =  parseInt(carbonReading);

    //Get Time
    var time = new Date();
    //Update graph

    chart.data.datasets[2].data.push(carbonReading);


    chart.update();

    if(carbonReading >= 150){
      document.getElementById("warningBox").style.display =  "block";
    } else {
      document.getElementById("warningBox").style.display =  "none";
    }
    break;
  }
}


function disconnect() {
  if (deviceCache) {
    deviceCache.removeEventListener('gattserverdisconnected',
    handleDisconnection);

    if (deviceCache.gatt.connected) {
      deviceCache.gatt.disconnect();

    }

  }

  if (characteristicCache) {
    characteristicCache.removeEventListener('characteristicvaluechanged',
    handleCharacteristicValueChanged);
    characteristicCache = null;
  }

  deviceCache = null;
}

function send(data) {
  data = String(data);

  if (!data || !characteristicCache) {
    return;
  }

  if (data.length > 20) {
    let chunks = data.match(/(.|[\r\n]){1,20}/g);

    writeToCharacteristic(characteristicCache, chunks[0]);

    for (let i = 1; i < chunks.length; i++) {
      setTimeout(() => {
        writeToCharacteristic(characteristicCache, chunks[i]);
      }, i * 100);
    }
  }
  else {
    writeToCharacteristic(characteristicCache, data);
  }


}

function writeToCharacteristic(characteristic, data) {
  characteristic.writeValue(new TextEncoder().encode(data));
}
