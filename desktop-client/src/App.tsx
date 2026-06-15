import { useEffect, useMemo, useState } from "react";
import { DevicePanel } from "./components/DevicePanel";
import { StatePanel } from "./components/StatePanel";
import { TextPanel } from "./components/TextPanel";
import { applyState, pingDevice, sendText, type PetState } from "./services/api";
import { loadDevice, saveDevice } from "./services/device";

type ConnectionState = "idle" | "connected" | "error";

const AUTO_RECONNECT_MS = 15000;

export default function App() {
  const savedDevice = useMemo(loadDevice, []);
  const [deviceIp, setDeviceIp] = useState(savedDevice.ip);
  const [deviceName, setDeviceName] = useState(savedDevice.name);
  const [message, setMessage] = useState("主人，你回来啦");
  const [petState, setPetState] = useState<PetState>("idle");
  const [connection, setConnection] = useState<ConnectionState>("idle");
  const [busy, setBusy] = useState(false);
  const [statusText, setStatusText] = useState("等待连接设备");

  const runPing = async (silent = false) => {
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
  };

  useEffect(() => {
    const timer = window.setInterval(() => {
      if (deviceIp.trim()) {
        void runPing(true);
      }
    }, AUTO_RECONNECT_MS);

    return () => window.clearInterval(timer);
  }, [deviceIp]);

  const handleSave = () => {
    saveDevice({ ip: deviceIp, name: deviceName });
    setStatusText("设备配置已保存");
  };

  const handleSendText = async () => {
    setBusy(true);
    setStatusText("正在发送文字...");
    try {
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

      <StatePanel selected={petState} busy={busy} onSelect={setPetState} onApply={handleApplyState} />
    </main>
  );
}
