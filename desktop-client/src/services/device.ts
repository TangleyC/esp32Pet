const STORAGE_KEY = "deskpet.device";

export type SavedDevice = {
  ip: string;
  name: string;
};

export const loadDevice = (): SavedDevice => {
  const raw = window.localStorage.getItem(STORAGE_KEY);
  if (!raw) {
    return { ip: "192.168.1.88", name: "DeskPet ESP32" };
  }

  try {
    return { ip: "", name: "DeskPet ESP32", ...JSON.parse(raw) };
  } catch {
    // localStorage 可能被手动编辑或旧版本污染，失败时回到默认设备配置。
    return { ip: "192.168.1.88", name: "DeskPet ESP32" };
  }
};

export const saveDevice = (device: SavedDevice) => {
  window.localStorage.setItem(STORAGE_KEY, JSON.stringify(device));
};
