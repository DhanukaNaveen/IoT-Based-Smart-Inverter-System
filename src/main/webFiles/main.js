


// const x_slider = document.getElementById('x_slider');
// const atitude_slider = document.getElementById('atitude_slider');
// const emergency_stop_btn = document.getElementById('EM_stop_btn');
// emergency_stop_btn.addEventListener('click', () =>{
//   drone_X = 0;
//   drone_Y = 0;
//   atitude_slider.value = 0;
//   sendtows();
//   wsSendData('runns');
//   set_value_range_slider('atitude_slider');
// });
// x_slider.addEventListener('change', () => {
//     x_slider.value = 0; // Reset to the initial value (50 in this case)
//     set_value_range_slider('x_slider');
//     sendtows();
// });

// atitude_slider.addEventListener('change', () => {
//   atitude_slider.value = 0; // Reset to the initial value (50 in this case)
//   set_value_range_slider('atitude_slider');
//   sendtows();
// });

// const ws = new WebSocket('ws://'+ window.location.hostname +'/ws');
const ws = new WebSocket('ws://espserver.local/ws');

ws.onopen = () => {
  console.log('Connected to WebSocket');
document.getElementById('progress').innerText = "CONNECTED!";
};

ws.onmessage = (event) => {
  if (event.data instanceof Blob) {
    event.data.arrayBuffer().then((wsBufArr) => {
      // const wsBufArr = new Uint8Array(buffer);
      let len = wsBufArr.length;
      if (len == 0) return;

      const decoderTXT = new TextDecoder('utf-8');
      let dtType = decoderTXT.decode(wsBufArr.slice(0, 4));
      console.log("dtype :" + dtType + " buf ", wsBufArr);

      if (dtType === "ACDT") {
        document.getElementById('ac_voltage_txt_ptr').innerText = binaryToInt(wsBufArr.slice(4, 8), 'f32').toFixed(1) + " V";
        document.getElementById('ac_current_txt_ptr').innerText = binaryToInt(wsBufArr.slice(8, 12), 'f32').toFixed(3) + " A";
        document.getElementById('ac_power_txt_ptr').innerText = binaryToInt(wsBufArr.slice(12, 16), 'f32').toFixed(1) + " W";
        document.getElementById('ac_energy_txt_ptr').innerText = binaryToInt(wsBufArr.slice(16, 20), 'f32').toFixed(0) + " Wh";
        document.getElementById('ac_frequency_txt_ptr').innerText = binaryToInt(wsBufArr.slice(20, 24), 'f32').toFixed(1) + " Hz";
        document.getElementById('ac_powerfac_txt_ptr').innerText = binaryToInt(wsBufArr.slice(24, 28), 'f32').toFixed(2);

      } else if (dtType === "BTDT") {
        document.getElementById('batt_volt_txt_ptr').innerText = binaryToInt(wsBufArr.slice(4, 8), 'f32').toFixed(2) + " V";
        document.getElementById('batt_precent_txt_ptr').innerText = binaryToInt(wsBufArr.slice(8, 10), 'ui8') + " %";
        document.getElementById('temperature_txt_ptr').innerText = binaryToInt(wsBufArr.slice(14, 18), 'f32').toFixed(1) + " C";

        const dataArray = new Uint8Array(wsBufArr.slice(9, 14));

        document.getElementById('relay_1_status').innerText = (dataArray[0] == 2) ? "AUTO" : (dataArray[0] == 1) ? "ON" : "OFF";
        document.getElementById('relay_2_status').innerText = (dataArray[1] == 2) ? "AUTO" : (dataArray[1] == 1) ? "ON" : "OFF";
        document.getElementById('relay_3_status').innerText = (dataArray[2] == 2) ? "AUTO" : (dataArray[2] == 1) ? "ON" : "OFF";
        document.getElementById('relay_4_status').innerText = (dataArray[3] == 2) ? "AUTO" : (dataArray[3] == 1) ? "ON" : "OFF";
        document.getElementById('relay_5_status').innerText = (dataArray[4] == 2) ? "AUTO" : (dataArray[4] == 1) ? "ON" : "OFF";
      }
    }).catch((error) => console.log("error blob : ", error));
  } else {
    console.log('rx msg: ', event.data);

    let ws_dt_type = event.data.slice(0, 3);
    let ws_data = event.data.slice(3);

    if (ws_dt_type == "OTA") {
    }
  }
};

ws.onclose = () => {
  console.log('WebSocket connection closed');
};

function binaryToInt(bin, type) {
  const view = new DataView(bin);
  if (type === 'ui8') {
    return view.getUint8(0);
  } else if (type === 'ui16') {
    return view.getUint16(0, true); // Big-endian
  } else if (type === 'i16') {
    return view.getInt16(0, true); // Big-endian
  } else if (type === 'ui32') {
    return view.getUint32(0, true); // Big-endian
  } else if (type === 'i32') {
    return view.getInt32(0, true); // Big-endian
  } else if (type === 'f32') {
    return view.getFloat32(0, true);
  }
  return 0;
}
function intToByteArrayTyped(value, byteCount, type) {
  const buffer = new ArrayBuffer(byteCount);
  const view = new DataView(buffer);

  // Store the integer in the buffer as needed
  if (type === 'ui8') {
    view.setUint8(0, value);
  } else if (type === 'ui16') {
    view.setUint16(0, value, true); // Big-endian
  } else if (type === 'i16') {
    view.setInt16(0, value, true); // Big-endian
  } else if (type === 'ui32') {
    view.setUint32(0, value, true); // Big-endian
  } else if (type === 'i32') {
    view.setInt32(0, value, true); // Big-endian
  } else if (type === 'f32') {
    view.setFloat32(0, value, true);
  }

  return new Uint8Array(buffer);
}

const encoderTXT = new TextEncoder();
function getWsSendBuffer(HEAD, CMD, ...DATA) {
  const wsHeader = HEAD + CMD;
  const wsHeaderbyt = encoderTXT.encode(wsHeader);

  let totalbyteLen = DATA.reduce((acc, curr) => acc + curr.length, 0);
  totalbyteLen += wsHeaderbyt.length;

  let databytes = new Uint8Array(totalbyteLen);
  databytes.set(wsHeaderbyt, 0);

  let byteOffset = wsHeaderbyt.length;
  for (let data of DATA) {
    databytes.set(data, byteOffset);
    byteOffset += data.length;
  }
  return databytes;
}

document.getElementById('espReset').addEventListener('click', () => {
  ws.send("rest");
  console.log("rest");
});

function alarm_off(){
  ws.send("dataalam");
  console.log("dataalam");
}

function relay_control_submit(){
  let reNum = document.getElementById('relay_control_num').value;
  let reMode = document.querySelector('input[name="RELAYMODE"]:checked').value;
  let rePcen = document.getElementById('relay_control_precent').value;
  const wsBuf = getWsSendBuffer("data", "rely",
      intToByteArrayTyped(reNum, 1, 'ui8'),
      intToByteArrayTyped(reMode, 1, 'ui8'),
      intToByteArrayTyped(rePcen, 1, 'ui8'));

  console.log(wsBuf);
  ws.send(wsBuf);
}