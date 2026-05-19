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

// сортировка
let sortKey: keyof ProcessInfo = 'cpu_percent';
let sortDirection: 'asc' | 'desc' = 'desc';

// управление
const configContainer = document.getElementById('config-connect') as HTMLDivElement;
const monitorContainer = document.getElementById('monitor-container') as HTMLDivElement;
const ipInput = document.getElementById('ip-input') as HTMLInputElement;
const portInput = document.getElementById('port-input') as HTMLInputElement;
const localPortInput = document.getElementById('local-port-input') as HTMLInputElement;
const connectRemoteBtn = document.getElementById('connect-remote') as HTMLButtonElement;
const connectLocalBtn = document.getElementById('connect-local') as HTMLButtonElement;
const disconnectBtn = document.getElementById('disconnect-btn') as HTMLButtonElement;
const statusDiv = document.getElementById('status') as HTMLDivElement;

// отображение
const cpuMemStats = document.getElementById('cpu-mem-stats') as HTMLDivElement;
const tasksStats = document.getElementById('tasks-stats') as HTMLDivElement;
const processesTableBody = document.getElementById('processes-table-body') as HTMLTableSectionElement;
const tableHeaders = document.getElementById('table-headers') as HTMLTableRowElement;

// функционал сортировки
tableHeaders.querySelectorAll('th').forEach(th => {
    th.onclick = () => {
        const key = th.getAttribute('data-key') as keyof ProcessInfo;

        if(sortKey === key){
            sortDirection = sortDirection === 'asc' ? 'desc' : 'asc';
        }
        else{
            sortKey = key;
            sortDirection = 'desc';
        }

        updateHeaderStyles();
    };
});

// чистка и отрисовка стрелок
function updateHeaderStyles(){
    tableHeaders.querySelectorAll('th').forEach(th => {
        const key = th.getAttribute('data-key');

        th.innerHTML = th.innerText.replace(/ [▲▼]/g, '');

        if(key === sortKey){
            th.innerHTML += sortDirection === 'asc' ? ' ▲' : ' ▼';
        }
    });
}

// статус
function updateStatus(msg: string, color: string = 'blue'){
    statusDiv.innerText = msg;
    statusDiv.style.color = color;
}

// всё сделал на одной странице, поэтому такое быстрое скрытие объектов
function toggleUI(connected: boolean){
    if(connected){
        configContainer.style.display = 'none';
        monitorContainer.style.display = 'block';
        updateHeaderStyles();
    }
    else{
        configContainer.style.display = 'block';
        monitorContainer.style.display = 'none';
        cpuMemStats.innerHTML = '';
        tasksStats.innerHTML = '';
        processesTableBody.innerHTML = '';
    }
}

// wc connect
function connect(ip: string, port: string){
    if(ws && ws.readyState === WebSocket.CONNECTING){
        console.log('Уже пытаемся подключиться...');
        return;
    }

    if(ws){
        ws.onclose = null;
        ws.onerror = null;
        ws.close();
    }

    updateStatus(`Подключение к ${ip}:${port}...`);

    try{
        ws = new WebSocket(`ws://${ip}:${port}/get_monitoring_resources`);


        ws.onmessage = (event: MessageEvent) => {
            try{
                const data: MonitoringData = JSON.parse(event.data);
                renderData(data);

                updateStatus(`Подключено к ${ip}:${port}`, 'green');
            }
            catch(error){
                console.error('Ошибка парсинга:', error);
            }
        };

        ws.onopen = () => {
            toggleUI(true);
            updateStatus(`Подключено к ${ip}:${port}`, 'green');
        };

        ws.onerror = (e) => {
            console.error('ошибка:', e);
            updateStatus('Ошибка подключения', 'red');
        };

        ws.onclose = (e) => {
            console.log(e.code, e.reason);
            toggleUI(false);

            if(e.code === 1006){
                updateStatus('Соединение прервано сервером или сетью', 'red');
            }
            else{
                updateStatus('Соединение закрыто', 'gray');
            }
        };
    }
    catch(e){
        updateStatus('Некорректный адрес или порт', 'red');
    }
}

// закрытие wc
function disconnect(){
    if(ws){
        ws.close();
        ws = null;
    }

    toggleUI(false);
}

// закрытие wc при закрытии страницы
window.addEventListener('beforeunload', () => {
    disconnect();
});

// форматирование к гигабайтам и мегабайтам
function formatMemory(kb: number): string {
    if(kb > 1024*1024) return (kb / 1024 / 1024).toFixed(1)+'G';
    if(kb > 1024) return (kb / 1024).toFixed(1)+'M';

    return kb.toString()+'K';
}

// отображение
function renderData(data: MonitoringData) {
    const cpuUsage = data.cpu.usage_percent.toFixed(2);
    const memUsed = (data.mem.used / 1024 / 1024).toFixed(2);
    const memTotal = (data.mem.total / 1024 / 1024).toFixed(2);
    const memPercent = ((data.mem.used / data.mem.total) * 100).toFixed(2);

    // CPU + MEM
    cpuMemStats.innerHTML = `
        <strong>CPU:</strong> ${cpuUsage}% | 
        <strong>MEM:</strong> ${memPercent}% (${memUsed} GB / ${memTotal} GB)
    `;

    // tasks
    tasksStats.innerHTML = `
        Tasks: ${data.tasks.total} total, 
        ${data.tasks.running} running, 
        ${data.tasks.sleeping} sleeping, 
        ${data.tasks.zombie} zombie
    `;

    // сорт по ключу
    const sortedProcesses = [...data.processes].sort((a, b) => {
        let valA = a[sortKey];
        let valB = b[sortKey];

        if(typeof valA === 'string'){
            valA = valA.toLowerCase();
            valB = (valB as string).toLowerCase();
        }

        if(valA < valB) return sortDirection === 'asc' ? -1 : 1;
        if(valA > valB) return sortDirection === 'asc' ? 1 : -1;
        return 0;
    });

    // таблица
    processesTableBody.innerHTML = '';

    sortedProcesses.forEach(proc => {
        const row = document.createElement('tr');

        row.innerHTML = `
            <td>${proc.pid}</td>
            <td>${proc.user}</td>
            <td>${proc.pr}</td>
            <td>${proc.ni}</td>
            <td>${formatMemory(proc.virt)}</td>
            <td>${formatMemory(proc.res)}</td>
            <td>${formatMemory(proc.shr)}</td>
            <td>${proc.s}</td>
            <td style="color: ${proc.cpu_percent > 20 ? 'red' : 'inherit'}">${proc.cpu_percent.toFixed(1)}</td>
            <td>${proc.mem_percent.toFixed(1)}</td>
            <td>${proc.time}</td>
            <td>${proc.command}</td>
        `;

        processesTableBody.appendChild(row);
    });
}

// btns
connectRemoteBtn.onclick = () => {
    connect(ipInput.value, portInput.value);
};

connectLocalBtn.onclick = () => {
    connect('localhost', localPortInput.value);
};

disconnectBtn.onclick = () => {
    disconnect();
};
