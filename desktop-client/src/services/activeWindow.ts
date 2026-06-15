import { invoke } from "@tauri-apps/api/core";
import type { PetState } from "./api";

export type ActiveWindowInfo = {
  process: string;
  title: string;
};

type WindowStateRule = {
  process?: string[];
  titleContains?: string[];
  state: PetState;
};

const WINDOW_STATE_RULES: WindowStateRule[] = [
  {
    process: ["Code", "Codex", "Cursor", "WebStorm", "idea64", "pycharm64", "devenv"],
    state: "working",
  },
  {
    process: ["WindowsTerminal", "wt", "powershell", "pwsh", "cmd"],
    state: "working",
  },
  {
    process: ["chrome", "msedge", "firefox"],
    titleContains: ["Codex", "GitHub", "Jenkins", "PlatformIO", "Tauri", "React", "ESP32"],
    state: "thinking",
  },
  {
    process: ["chrome", "msedge", "firefox"],
    state: "fishing",
  },
  {
    process: ["WeChat", "Weixin", "WeChatAppEx", "WXWork", "WXWorkWeb", "WeMail", "QQ", "Telegram", "DingTalk"],
    state: "message",
  },
  {
    titleContains: ["微信", "企业微信", "WeChat", "WXWork"],
    state: "message",
  },
  {
    process: ["Clash Verge", "Clash Verge Service"],
    state: "cool",
  },
];

const DEFAULT_WINDOW_STATE: PetState = "online";

export const isTauriRuntime = () => "__TAURI_INTERNALS__" in window;

export const getActiveWindow = async (): Promise<ActiveWindowInfo | null> => {
  if (!isTauriRuntime()) {
    return null;
  }

  return invoke<ActiveWindowInfo | null>("active_window");
};

const includesIgnoreCase = (value: string, keyword: string) =>
  value.toLocaleLowerCase().includes(keyword.toLocaleLowerCase());

const processMatches = (rule: WindowStateRule, process: string) =>
  !rule.process?.length || rule.process.some((candidate) => candidate.toLocaleLowerCase() === process.toLocaleLowerCase());

const titleMatches = (rule: WindowStateRule, title: string) =>
  !rule.titleContains?.length || rule.titleContains.some((keyword) => includesIgnoreCase(title, keyword));

export const resolveWindowState = (windowInfo: ActiveWindowInfo): PetState => {
  const rule = WINDOW_STATE_RULES.find((candidate) => processMatches(candidate, windowInfo.process) && titleMatches(candidate, windowInfo.title));
  return rule?.state ?? DEFAULT_WINDOW_STATE;
};
