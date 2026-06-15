import { useCallback, useEffect, useMemo, useState } from "react";
import { DevicePanel } from "./components/DevicePanel";
import { MonitorPanel } from "./components/MonitorPanel";
import { StatePanel } from "./components/StatePanel";
import { TextPanel } from "./components/TextPanel";
import { getActiveWindow, isTauriRuntime, resolveWindowState, type ActiveWindowInfo } from "./services/activeWindow";
import { applyState, pingDevice, sendText, type PetState } from "./services/api";
import { loadDevice, saveDevice } from "./services/device";

type ConnectionState = "idle" | "connected" | "error";

const AUTO_RECONNECT_MS = 15000;
const WINDOW_MONITOR_MS = 1200;

export default function App() {
  // 设备信息只在首次渲染时从 localStorage 读取，避免每次状态更新都重复解析。
  const savedDevice = useMemo(loadDevice, []);
  const [deviceIp, setDeviceIp] = useState(savedDevice.ip);
  const [deviceName, setDeviceName] = useState(savedDevice.name);
  const [message, setMessage] = useState("主人，你回来啦");
  const [petState, setPetState] = useState<PetState>("online");
  const [connection, setConnection] = useState<ConnectionState>("idle");
  const [busy, setBusy] = useState(false);
  const [statusText, setStatusText] = useState("等待连接设备");
  const [monitorEnabled, setMonitorEnabled] = useState(true);
  const [monitorStatusText, setMonitorStatusText] = useState("Tauri 桌面窗口打开后，会根据活动窗口自动切换 DeskPet 状态。");
  const [activeWindow, setActiveWindow] = useState<ActiveWindowInfo | null>(null);
  const [activeWindowState, setActiveWindowState] = useState<PetState | null>(null);
  const monitorAvailable = isTauriRuntime();

  // silent=true 用于自动重连轮询：更新连接状态，但不打断用户当前的操作提示。
  const runPing = useCallback(async (silent = false) => {
    if (!deviceIp.trim()) {
      setConnection("error");
      setStatusText("请先填写设备 IP");
      return;
    }

    if (!silent) {
      setBusy(true);
      setStatusText("正在测试连接...");
    }

    try {
      const result = await pingDevice(deviceIp);
      setConnection(result.ok ? "connected" : "error");
      setStatusText(result.ok ? `连接成功：${result.device}` : "设备返回异常");
    } catch (error) {
      setConnection("error");
      if (!silent) {
        setStatusText(error instanceof Error ? error.message : "连接失败");
      }
    } finally {
      if (!silent) {
        setBusy(false);
      }
    }
  }, [deviceIp]);

  // MVP 阶段用轻量轮询做自动重连，后续 WebSocket 接入后可以替换为心跳/在线事件。
  useEffect(() => {
    const timer = window.setInterval(() => {
      if (deviceIp.trim()) {
        void runPing(true);
      }
    }, AUTO_RECONNECT_MS);

    return () => window.clearInterval(timer);
  }, [deviceIp, runPing]);

  useEffect(() => {
    if (!monitorAvailable) {
      setMonitorEnabled(false);
      setMonitorStatusText("当前是浏览器前端预览，活动窗口联动需要用 Tauri 桌面客户端启动。");
      return;
    }

    if (!monitorEnabled) {
      setMonitorStatusText("活动窗口联动已关闭。");
      return;
    }

    let lastSentState: PetState | null = null;
    let stopped = false;

    const syncWindowState = async () => {
      if (!deviceIp.trim()) {
        setMonitorStatusText("请先填写设备 IP。");
        return;
      }

      try {
        const windowInfo = await getActiveWindow();
        if (!windowInfo || stopped) {
          return;
        }

        const nextState = resolveWindowState(windowInfo);
        setActiveWindow(windowInfo);
        setActiveWindowState(nextState);

        if (nextState !== lastSentState) {
          await applyState(deviceIp, nextState);
          lastSentState = nextState;
          setPetState(nextState);
          setConnection("connected");
          setMonitorStatusText("已同步到 ESP32。");
        }
      } catch (error) {
        setConnection("error");
        setMonitorStatusText(error instanceof Error ? error.message : "活动窗口同步失败。");
      }
    };

    void syncWindowState();
    const timer = window.setInterval(() => void syncWindowState(), WINDOW_MONITOR_MS);

    return () => {
      stopped = true;
      window.clearInterval(timer);
    };
  }, [deviceIp, monitorAvailable, monitorEnabled]);

  const handleSave = () => {
    saveDevice({ ip: deviceIp, name: deviceName });
    setStatusText("设备配置已保存");
  };

  const handleSendText = async () => {
    setBusy(true);
    setStatusText("正在发送文字...");
    try {
      // 文本接口会让 ESP32 立即切到文本展示画面。
      await sendText(deviceIp, message.trim());
      setConnection("connected");
      setStatusText("文字已发送到 DeskPet");
    } catch (error) {
      setConnection("error");
      setStatusText(error instanceof Error ? error.message : "发送失败");
    } finally {
      setBusy(false);
    }
  };

  const handleApplyState = async () => {
    setBusy(true);
    setStatusText(`正在切换到 ${petState}...`);
    try {
      // 状态接口只发送状态 key，具体表情和文案由固件端统一映射。
      await applyState(deviceIp, petState);
      setConnection("connected");
      setStatusText(`状态已切换：${petState}`);
    } catch (error) {
      setConnection("error");
      setStatusText(error instanceof Error ? error.message : "状态切换失败");
    } finally {
      setBusy(false);
    }
  };

  return (
    <main className="app-shell">
      <header>
        <div>
          <p className="eyebrow">DeskPet MVP</p>
          <h1>DeskPet Control Center</h1>
        </div>
        <div className={connection === "connected" ? "connection-dot online" : "connection-dot"} />
      </header>

      <DevicePanel
        ip={deviceIp}
        name={deviceName}
        connected={connection === "connected"}
        busy={busy}
        statusText={statusText}
        onIpChange={setDeviceIp}
        onNameChange={setDeviceName}
        onPing={() => void runPing(false)}
        onSave={handleSave}
      />

      <TextPanel value={message} busy={busy} onChange={setMessage} onSend={handleSendText} />

      <MonitorPanel
        enabled={monitorEnabled}
        available={monitorAvailable}
        windowInfo={activeWindow}
        resolvedState={activeWindowState}
        statusText={monitorStatusText}
        onToggle={setMonitorEnabled}
      />

      <StatePanel selected={petState} busy={busy} onSelect={setPetState} onApply={handleApplyState} />
    </main>
  );
}
