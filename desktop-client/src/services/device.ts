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
    return { ip: "192.168.1.88", name: "DeskPet ESP32" };
  }
};

export const saveDevice = (device: SavedDevice) => {
  window.localStorage.setItem(STORAGE_KEY, JSON.stringify(device));
};
