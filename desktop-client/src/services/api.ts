export type PetState = "idle" | "sleep" | "coding" | "error" | "success" | "happy";

export type PingResponse = {
  ok: boolean;
  device: string;
};

export type CommandResponse = {
  success: boolean;
};

const normalizeBaseUrl = (ip: string) => {
  const value = ip.trim();
  if (!value) {
    throw new Error("请先填写设备 IP");
  }

  if (/^https?:\/\//i.test(value)) {
    return value.replace(/\/+$/, "");
  }

  return `http://${value.replace(/\/+$/, "")}`;
};

const requestJson = async <T>(ip: string, path: string, init?: RequestInit): Promise<T> => {
  const response = await fetch(`${normalizeBaseUrl(ip)}${path}`, {
    ...init,
    headers: {
      Accept: "application/json",
      "Content-Type": "application/json",
      ...init?.headers,
    },
  });

  if (!response.ok) {
    throw new Error(`设备响应异常：HTTP ${response.status}`);
  }

  return response.json() as Promise<T>;
};

export const pingDevice = (ip: string) => requestJson<PingResponse>(ip, "/ping");

export const sendText = (ip: string, content: string) =>
  requestJson<CommandResponse>(ip, "/text", {
    method: "POST",
    body: JSON.stringify({ content }),
  });

export const applyState = (ip: string, state: PetState) =>
  requestJson<CommandResponse>(ip, "/state", {
    method: "POST",
    body: JSON.stringify({ state }),
  });
