class WiFiConfig {
    constructor() {
        this.isScanning = false;
        this.init();
    }

    init() {
        this.loadCurrentStatus();
        this.setupEventListeners();
        this.startAutoRefresh();
        this.readScanNetworksCache();
    }

    setupEventListeners() {
        // WiFi表单提交
        document.getElementById('config-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.saveWiFiConfig();
        });
    }

    async loadCurrentStatus() {
        try {
            const response = await fetch('/api/status');
            const data = await response.json();
            this.updateStatusDisplay(data);
        } catch (error) {
            this.showToast('无法获取设备状态', 'error');
            console.error('Error loading status:', error);
        }
    }

    updateStatusDisplay(data) {
        document.getElementById('current-ssid').textContent = data.ssid || '未连接';
        document.getElementById('current-ip').textContent = data.ip || '--';
        document.getElementById('current-rssi').textContent = data.rssi ? `${data.rssi} dBm` : '--';
        document.getElementById('uptime').textContent = this.formatUptime(data.uptime);
        
        const statusElement = document.getElementById('status');
        if (data.wifi_status === 'connected') {
            statusElement.textContent = '已连接';
            statusElement.style.background = 'rgba(40, 167, 69, 0.3)';
        } else if (data.wifi_status === 'ap_mode') {
            statusElement.textContent = 'AP模式';
            statusElement.style.background = 'rgba(255, 193, 7, 0.3)';
        } else {
            statusElement.textContent = '连接中...';
            statusElement.style.background = 'rgba(108, 117, 125, 0.3)';
        }
    }

    formatUptime(seconds) {
        if (!seconds) return '--';
        
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        
        if (days > 0) {
            return `${days}天 ${hours}小时 ${minutes}分钟`;
        } else if (hours > 0) {
            return `${hours}小时 ${minutes}分钟`;
        } else {
            return `${minutes}分钟`;
        }
    }

    async saveWiFiConfig() {
        const formData = new FormData(document.getElementById('config-form'));
        const config = {
            ssid: formData.get('ssid'),
            password: formData.get('password'),
            host_ip: formData.get('host_ip'),
            host_port: formData.get('host_port')
        };

        if (!config.ssid) {
            this.showToast('请输入WiFi名称', 'warning');
            return;
        }

        try {
            const response = await fetch('/api/wifi/config', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(config)
            });

            const result = await response.json();

            if (result.success) {
                this.showToast('WiFi配置已保存，设备将重新启动连接', 'success');
                setTimeout(() => {
                    this.loadCurrentStatus();
                }, 3000);
            } else {
                this.showToast(`保存失败: ${result.message}`, 'error');
            }
        } catch (error) {
            this.showToast('保存配置时发生错误', 'error');
            console.error('Error saving config:', error);
        }
    }

    async scanNetworks() {
        if (this.isScanning) {
            return;
        }

        const userConfirmed = confirm('⚠️ 重要提示：\n\n扫描WiFi网络可能会导致当前连接中断。\n\n扫描完成后：\n• 如果设备在AP模式：需要重新连接到ESP32的WiFi\n• 如果设备在STA模式：需要等待设备重新连接后访问\n\n确定要扫描吗？');
        if (!userConfirmed) {
            return;
        }

        this.isScanning = true;
        const scanBtn = document.querySelector('.scan-btn');
        const originalText = scanBtn.innerHTML;
        scanBtn.innerHTML = '扫描中...';
        scanBtn.classList.add('loading');
        scanBtn.disabled = true;

        try {
            this.showToast('正在扫描网络...', 'warning');
            
            const response = await fetch('/api/wifi/scan', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({"read_cache": false})
            });
            const networks = await response.json();

            this.displayNetworks(networks);
        } catch (error) {
            this.showToast('扫描网络失败', 'error');
            console.error('Error scanning networks:', error);
        } finally {
            this.isScanning = false;
            scanBtn.innerHTML = originalText;
            scanBtn.classList.remove('loading');
            scanBtn.disabled = false;
        }
    }

    async readScanNetworksCache() {
        try {
            const response = await fetch('/api/wifi/scan', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({"read_cache": true})
            });
            const networks = await response.json();

            this.displayNetworks(networks);
        } catch (error) {
            console.error('Error scanning networks:', error);
        }
    }

    displayNetworks(networks) {
        const container = document.getElementById('networks-list');
        const resultsCard = document.getElementById('scan-results');
        
        if (!networks || networks.length === 0) {
            container.innerHTML = '<p>未发现可用网络</p>';
            resultsCard.style.display = 'block';
            return;
        }

        container.innerHTML = networks.map(network => `
            <div class="network-item" onclick="wifiConfig.selectNetwork('${network.ssid}', ${network.encryption !== 'open'})">
                <div>
                    <strong>${this.escapeHtml(network.ssid)}</strong>
                    <div class="signal">信号: ${network.rssi} dBm</div>
                    <div class="encryption">加密: ${network.encryption}</div>
                </div>
            </div>
        `).join('');

        resultsCard.style.display = 'block';
    }

    selectNetwork(ssid, hasEncryption) {
        document.getElementById('ssid').value = ssid;
        
        if (hasEncryption) {
            document.getElementById('password').focus();
        } else {
            document.getElementById('password').value = '';
        }
        
        // 滚动到表单
        document.getElementById('wifi-form').scrollIntoView({ 
            behavior: 'smooth' 
        });
    }

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    async restartDevice() {
        if (!confirm('确定要重启设备吗？')) return;

        try {
            const response = await fetch('/api/system/restart', { method: 'POST' });
            this.showToast('设备正在重启...', 'warning');
        } catch (error) {
            console.error('Error restarting device:', error);
        }
    }

    async resetConfig() {
        if (!confirm('确定要恢复出厂设置吗？这将清除所有WiFi配置。')) return;

        try {
            const response = await fetch('/api/system/reset', { method: 'POST' });
            this.showToast('正在恢复出厂设置...', 'warning');
        } catch (error) {
            console.error('Error resetting config:', error);
        }
    }

    showToast(message, type = 'info') {
        const toast = document.getElementById('toast');
        toast.textContent = message;
        toast.className = `toast show ${type}`;
        
        setTimeout(() => {
            toast.classList.remove('show');
        }, 3000);
    }

    startAutoRefresh() {
        // 每10秒更新一次状态
        setInterval(() => {
            this.loadCurrentStatus();
        }, 10000);
    }
}

// 初始化应用
const wifiConfig = new WiFiConfig();