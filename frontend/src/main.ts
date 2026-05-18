interface CpuInfo {
    idle: number;
    system: number;
    usage_percent: number;
    user: number;
}

interface MemInfo {
    available: number;
    buffers: number;
    cached: number;
    free: number;
    total: number;
    used: number;
}

interface ProcessInfo {
    command: string;
    cpu_percent: number;
    mem_percent: number;
    ni: number;
    pid: number;
    pr: number;
    res: number;
    s: string;
    shr: number;
    time: string;
    user: string;
    virt: number;
}

interface TasksInfo {
    running: number;
    sleeping: number;
    stopped: number;
    total: number;
    zombie: number;
}

interface MonitoringData {
    cpu: CpuInfo;
    mem: MemInfo;
    processes: ProcessInfo[];
    tasks: TasksInfo;
}

let ws: WebSocket | null = null;

const ipInput = document.getElementById('ip-input') as HTMLInputElement;
const portInput = document.getElementById('port-input') as HTMLInputElement;
const localPortInput = document.getElementById('local-port-input') as HTMLInputElement;
const connectRemoteBtn = document.getElementById('connect-remote') as HTMLButtonElement;
const connectLocalBtn = document.getElementById('connect-local') as HTMLButtonElement;
const statusDiv = document.getElementById('status') as HTMLDivElement;

function updateStatus(msg: string, color: string = 'blue'){
    statusDiv.innerText = msg;
    statusDiv.style.color = color;
}

function connect(ip: string, port: string){
    if(ws){
        ws.close();
    }

    updateStatus(`Подключение к ${ip}:${port}...`);

    try{
        ws = new WebSocket(`ws://${ip}:${port}/get_monitoring_resources`);


        ws.onmessage = (event: MessageEvent) => {
            try{
                const data: MonitoringData = JSON.parse(event.data);

                console.log(data);
                updateStatus(`Подключено к ${ip}:${port}`, 'green');
            }
            catch(error){
                console.error('Ошибка парсинга: ', error);
            }
        };


        ws.onerror = () => {
            updateStatus('Ошибка подключения', 'red');
        };

        ws.onclose = () => {
            updateStatus('Соединение закрыто', 'gray');
        };
    }
    catch(e){
        updateStatus('Некорректный адрес или порт', 'red');
    }
}


// buttons
connectRemoteBtn.onclick = () => {
    connect(ipInput.value, portInput.value);
};

connectLocalBtn.onclick = () => {
    connect('localhost', localPortInput.value);
};
