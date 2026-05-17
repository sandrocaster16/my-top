let ip = "localhost"
let port = "12345"

const ws = new WebSocket('ws://'+ip+':'+port+'/get_monitoring_resources');

ws.onmessage = (event: MessageEvent) => {
    console.log(event.data);
};