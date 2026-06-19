#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <PNGdec.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
// 颜色集中放在这里，后续做主题或像素风皮肤时只需要改一处。
constexpr uint16_t COLOR_BG = TFT_BLACK;
constexpr uint16_t COLOR_FACE = TFT_CYAN;
constexpr uint16_t COLOR_TEXT = TFT_WHITE;
constexpr uint16_t COLOR_MUTED = TFT_DARKGREY;
constexpr uint16_t COLOR_OK = TFT_GREEN;
constexpr uint16_t COLOR_ERROR = TFT_RED;

TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;
WebServer server(80);
Preferences preferences;
PNG png;
File pngFile;
SPIClass sdSpi(VSPI);

constexpr const char* DEFAULT_DEVICE_NAME = "DeskPet ESP32";
constexpr const char* SETUP_AP_SSID = "DeskPet-Setup";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr int SD_CS_PIN = 5;
constexpr int SD_MOSI_PIN = 23;
constexpr int SD_MISO_PIN = 19;
constexpr int SD_SCLK_PIN = 18;
constexpr uint32_t SD_SPI_FREQUENCY = 20000000;

String currentState = "idle";
String currentFace = "(^_^)";
String currentText = "Idle";
String deviceName = DEFAULT_DEVICE_NAME;
bool setupMode = false;
bool littleFsAvailable = false;
bool sdAvailable = false;
uint16_t spriteLineBuffer[320];
int spriteDrawX = 0;
int spriteDrawY = 0;

// 桌宠状态表：桌面端只传 state，固件端负责映射表情、文案和强调色。
struct PetExpression {
  const char* state;
  const char* face;
  const char* text;
  uint16_t accent;
};

const PetExpression expressions[] = {
  {"online", "(^_^)", "在线", TFT_CYAN},
  {"thinking", "(?_?)", "思考中", TFT_YELLOW},
  {"idea", "(!)", "有想法了", TFT_YELLOW},
  {"cool", "[B-)]", "酷", TFT_CYAN},
  {"working", "[PC]", "工作中", TFT_YELLOW},
  {"busy", "(-_-;)", "忙碌中", TFT_ORANGE},
  {"fishing", "<><", "摸鱼中", TFT_CYAN},
  {"message", "(^o^)", "收到消息", TFT_MAGENTA},
  {"like", "<3", "喜欢", TFT_MAGENTA},
  {"sad", "T_T", "难过", TFT_BLUE},
  {"angry", "(>_<)", "生气", TFT_RED},
  {"confused", "(?)", "疑惑", TFT_YELLOW},
  {"offline", "(x_x)", "离线", TFT_DARKGREY},
  {"idle", "(^_^)", "Idle", TFT_CYAN},
  {"sleep", "(-_-) Zzz", "", TFT_BLUE},
  {"coding", "[CODE]", "Coding...", TFT_YELLOW},
  {"error", "(!)", "Program error", TFT_RED},
  {"success", "(OK)", "Task done", TFT_GREEN},
  {"happy", "(^o^)", "Happy today", TFT_MAGENTA},
};

void* pngOpen(const char* filename, int32_t* size) {
  if (sdAvailable && SD.exists(filename)) {
    pngFile = SD.open(filename, FILE_READ);
  } else if (littleFsAvailable && LittleFS.exists(filename)) {
    pngFile = LittleFS.open(filename, "r");
  }

  if (!pngFile) {
    return nullptr;
  }

  *size = pngFile.size();
  return &pngFile;
}

bool copyFileToSd(const char* path) {
  if (!littleFsAvailable || !sdAvailable || !LittleFS.exists(path)) {
    return false;
  }

  File source = LittleFS.open(path, "r");
  if (!source) {
    Serial.print("LittleFS open failed: ");
    Serial.println(path);
    return false;
  }

  File target = SD.open(path, FILE_WRITE);
  if (!target) {
    Serial.print("SD write failed: ");
    Serial.println(path);
    source.close();
    return false;
  }

  uint8_t buffer[512];
  while (source.available()) {
    size_t bytesRead = source.read(buffer, sizeof(buffer));
    if (target.write(buffer, bytesRead) != bytesRead) {
      Serial.print("SD write incomplete: ");
      Serial.println(path);
      source.close();
      target.close();
      return false;
    }
  }

  source.close();
  target.close();
  Serial.print("Copied sprite to SD: ");
  Serial.println(path);
  return true;
}

void syncSpritesToSd() {
  if (!littleFsAvailable || !sdAvailable) {
    return;
  }

  if (!SD.exists("/sprites")) {
    SD.mkdir("/sprites");
  }

  File root = LittleFS.open("/sprites");
  if (!root || !root.isDirectory()) {
    Serial.println("LittleFS /sprites directory missing");
    return;
  }

  uint16_t copied = 0;
  uint16_t skipped = 0;
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String path = String("/sprites/") + entry.name();
      if (SD.exists(path)) {
        skipped++;
      } else if (copyFileToSd(path.c_str())) {
        copied++;
      }
    }

    entry.close();
    entry = root.openNextFile();
  }

  root.close();
  Serial.printf("Sprite sync complete: copied=%u skipped=%u\n", copied, skipped);
}

void initStorage() {
  littleFsAvailable = LittleFS.begin();
  if (!littleFsAvailable) {
    Serial.println("LittleFS mount failed; flash sprites will not be shown");
  } else {
    Serial.println("LittleFS mounted");
  }

  sdSpi.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  sdAvailable = SD.begin(SD_CS_PIN, sdSpi, SD_SPI_FREQUENCY);
  if (!sdAvailable) {
    Serial.println("SD mount failed; using LittleFS sprites only");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    sdAvailable = false;
    Serial.println("No SD card detected; using LittleFS sprites only");
    return;
  }

  uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);
  uint64_t usedMb = SD.usedBytes() / (1024ULL * 1024ULL);
  uint64_t totalMb = SD.totalBytes() / (1024ULL * 1024ULL);
  Serial.printf("SD mounted: card=%lluMB fs=%lluMB used=%lluMB\n", cardSizeMb, totalMb, usedMb);

  syncSpritesToSd();
}

void pngClose(void* handle) {
  File* file = static_cast<File*>(handle);
  if (file != nullptr) {
    file->close();
  }
}

int32_t pngRead(PNGFILE* file, uint8_t* buffer, int32_t length) {
  File* handle = static_cast<File*>(file->fHandle);
  if (handle == nullptr) {
    return 0;
  }

  return handle->read(buffer, length);
}

int32_t pngSeek(PNGFILE* file, int32_t position) {
  File* handle = static_cast<File*>(file->fHandle);
  if (handle == nullptr) {
    return 0;
  }

  return handle->seek(position);
}

int pngDraw(PNGDRAW* draw) {
  png.getLineAsRGB565(draw, spriteLineBuffer, PNG_RGB565_BIG_ENDIAN, COLOR_BG);
  tft.pushImage(spriteDrawX, spriteDrawY + draw->y, draw->iWidth, 1, spriteLineBuffer);
  return 1;
}

void addCorsHeaders() {
  // Tauri WebView 内的 fetch 仍可能受到 CORS 约束，ESP32 端主动放行本地控制台。
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Accept");
}

void sendJson(int status, const JsonDocument& document) {
  String payload;
  serializeJson(document, payload);
  addCorsHeaders();
  server.send(status, "application/json", payload);
}

void sendSuccess() {
  JsonDocument response;
  response["success"] = true;
  sendJson(200, response);
}

void sendError(int status, const char* message) {
  JsonDocument response;
  response["success"] = false;
  response["error"] = message;
  sendJson(status, response);
}

void handleOptions() {
  addCorsHeaders();
  server.send(204);
}

void drawCenteredString(const String& text, int y, int font, uint16_t color) {
  tft.setTextColor(color, COLOR_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, tft.width() / 2, y, font);
}

void drawWrappedText(const String& text, int x, int y, int maxWidth, int lineHeight, uint16_t color) {
  // TFT_eSPI 没有直接的自动换行能力，这里按像素宽度做一层简单包装。
  tft.setTextColor(color, COLOR_BG);
  tft.setTextDatum(TL_DATUM);

  String line;
  int cursorY = y;

  for (uint16_t i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == '\n') {
      tft.drawString(line, x, cursorY, 2);
      line = "";
      cursorY += lineHeight;
      continue;
    }

    String candidate = line + c;
    if (tft.textWidth(candidate, 2) > maxWidth && line.length() > 0) {
      tft.drawString(line, x, cursorY, 2);
      line = String(c);
      cursorY += lineHeight;
    } else {
      line = candidate;
    }
  }

  if (line.length() > 0) {
    tft.drawString(line, x, cursorY, 2);
  }
}

uint8_t utf8CharLength(uint8_t firstByte) {
  if ((firstByte & 0x80) == 0) {
    return 1;
  }

  if ((firstByte & 0xE0) == 0xC0) {
    return 2;
  }

  if ((firstByte & 0xF0) == 0xE0) {
    return 3;
  }

  if ((firstByte & 0xF8) == 0xF0) {
    return 4;
  }

  return 1;
}

void drawUtf8WrappedText(const String& text, int x, int y, int maxWidth, int lineHeight, uint16_t color) {
  // U8g2 font handles UTF-8 Chinese glyphs; y is baseline rather than top-left.
  u8f.setFont(u8g2_font_wqy12_t_gb2312);
  u8f.setFontMode(1);
  u8f.setForegroundColor(color);
  u8f.setBackgroundColor(COLOR_BG);

  String line;
  int cursorY = y;

  for (uint16_t i = 0; i < text.length();) {
    uint8_t firstByte = static_cast<uint8_t>(text[i]);
    if (text[i] == '\n') {
      u8f.drawUTF8(x, cursorY, line.c_str());
      line = "";
      cursorY += lineHeight;
      i++;
      continue;
    }

    uint8_t charLength = utf8CharLength(firstByte);
    String glyph = text.substring(i, min<uint16_t>(i + charLength, text.length()));
    String candidate = line + glyph;

    if (u8f.getUTF8Width(candidate.c_str()) > maxWidth && line.length() > 0) {
      u8f.drawUTF8(x, cursorY, line.c_str());
      line = glyph;
      cursorY += lineHeight;
    } else {
      line = candidate;
    }

    i += charLength;
  }

  if (line.length() > 0) {
    u8f.drawUTF8(x, cursorY, line.c_str());
  }
}

bool drawSprite(const String& state, int y) {
  String path = "/sprites/" + state + ".png";
  bool existsOnSd = sdAvailable && SD.exists(path);
  bool existsOnFlash = littleFsAvailable && LittleFS.exists(path);
  if (!existsOnSd && !existsOnFlash) {
    Serial.print("Sprite missing: ");
    Serial.println(path);
    return false;
  }

  int result = png.open(path.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);
  if (result != PNG_SUCCESS) {
    Serial.print("PNG open failed: ");
    Serial.print(path);
    Serial.print(" code=");
    Serial.println(result);
    return false;
  }

  spriteDrawX = max(0, (tft.width() - png.getWidth()) / 2);
  spriteDrawY = y;
  png.decode(nullptr, 0);
  png.close();
  return true;
}

void renderPet(uint16_t accent = COLOR_FACE) {
  // 所有屏幕刷新都走这里，避免 /text 和 /state 画面布局分叉。
  tft.fillScreen(COLOR_BG);
  tft.drawRoundRect(8, 8, tft.width() - 16, tft.height() - 16, 12, accent);
  tft.drawFastHLine(20, 58, tft.width() - 40, COLOR_MUTED);

  drawCenteredString("DeskPet", 34, 2, COLOR_MUTED);
  if (!drawSprite(currentState, 66)) {
    drawCenteredString(currentFace, 118, 4, accent);
  }

  if (currentText.length() > 0) {
    drawUtf8WrappedText(currentText, 24, 210, tft.width() - 48, 18, COLOR_TEXT);
  }

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.drawString(WiFi.localIP().toString(), tft.width() / 2, tft.height() - 18, 2);
}

void renderMessage(const String& rawText) {
  tft.fillScreen(COLOR_BG);
  tft.drawRoundRect(8, 8, tft.width() - 16, tft.height() - 16, 12, COLOR_FACE);
  tft.fillRect(8, 8, tft.width() - 16, 48, COLOR_FACE);

  tft.setTextColor(TFT_BLACK, COLOR_FACE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("MESSAGE", tft.width() / 2, 32, 4);

  if (!drawSprite("message", 58)) {
    drawCenteredString("(=^.^=)", 88, 4, COLOR_FACE);
  }
  drawUtf8WrappedText(rawText, 24, 190, tft.width() - 48, 18, COLOR_TEXT);

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.drawString(WiFi.localIP().toString(), tft.width() / 2, tft.height() - 18, 2);
}

void renderSetupMode() {
  tft.fillScreen(COLOR_BG);
  tft.drawRoundRect(8, 8, tft.width() - 16, tft.height() - 16, 12, TFT_YELLOW);
  tft.drawFastHLine(20, 58, tft.width() - 40, COLOR_MUTED);

  drawCenteredString("Setup Mode", 40, 4, TFT_YELLOW);
  drawCenteredString("WiFi: DeskPet-Setup", 112, 2, COLOR_TEXT);
  drawCenteredString("Open: 192.168.4.1", 150, 2, COLOR_TEXT);
  drawCenteredString("Save WiFi to restart", 196, 2, COLOR_MUTED);
}

const PetExpression* findExpression(const String& state) {
  for (const auto& expression : expressions) {
    if (state == expression.state) {
      return &expression;
    }
  }
  return nullptr;
}

bool parseJsonBody(JsonDocument& document) {
  // Arduino WebServer 把 POST 原始 body 放在 "plain" 参数里。
  if (!server.hasArg("plain")) {
    sendError(400, "missing request body");
    return false;
  }

  DeserializationError error = deserializeJson(document, server.arg("plain"));
  if (error) {
    sendError(400, "invalid json");
    return false;
  }

  return true;
}

String htmlEscape(const String& value) {
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  return escaped;
}

String getSetupPageHtml(const String& message = "") {
  String escapedName = htmlEscape(deviceName);
  String escapedMessage = htmlEscape(message);

  String html;
  html.reserve(2800);
  html += F("<!doctype html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">");
  html += F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
  html += F("<title>DeskPet Setup</title><style>");
  html += F("body{margin:0;font-family:system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#111827;color:#f9fafb;}");
  html += F("main{max-width:420px;margin:0 auto;padding:28px 18px;}h1{font-size:26px;margin:0 0 8px;}p{color:#cbd5e1;line-height:1.55;}");
  html += F("form{display:grid;gap:14px;margin-top:22px;}label{display:grid;gap:7px;font-weight:700;color:#e5e7eb;}");
  html += F("input{height:44px;border:1px solid #334155;border-radius:8px;background:#0f172a;color:#fff;padding:0 12px;font:inherit;}");
  html += F("button{height:46px;border:0;border-radius:8px;background:#22d3ee;color:#082f49;font-weight:800;font:inherit;}");
  html += F(".card{border:1px solid #334155;border-radius:10px;padding:18px;background:#1f2937;}.msg{color:#67e8f9;font-weight:700;}");
  html += F("a{color:#67e8f9;}</style></head><body><main><div class=\"card\">");
  html += F("<h1>DeskPet Setup</h1>");
  html += F("<p>请输入 2.4GHz WiFi 信息。保存后 ESP32 会写入 NVS 并自动重启。</p>");
  if (escapedMessage.length() > 0) {
    html += "<p class=\"msg\">" + escapedMessage + "</p>";
  }
  html += F("<form method=\"post\" action=\"/save-wifi\">");
  html += F("<label>SSID<input name=\"ssid\" autocomplete=\"off\" required></label>");
  html += F("<label>Password<input name=\"password\" type=\"password\" autocomplete=\"current-password\"></label>");
  html += "<label>Device Name<input name=\"deviceName\" value=\"" + escapedName + "\" autocomplete=\"off\"></label>";
  html += F("<button type=\"submit\">保存并重启</button></form>");
  html += F("<p><a href=\"/reset-wifi\">清除 WiFi 配置</a></p>");
  html += F("</div></main></body></html>");
  return html;
}

void handlePing() {
  JsonDocument response;
  response["ok"] = true;
  response["device"] = deviceName;
  response["mode"] = setupMode ? "setup" : "normal";
  sendJson(200, response);
}

void handleStatus() {
  JsonDocument response;
  response["ok"] = true;
  response["device"] = deviceName;
  response["mode"] = setupMode ? "setup" : "normal";
  response["state"] = currentState;
  response["face"] = currentFace;
  response["text"] = currentText;
  response["ip"] = setupMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  response["storage"]["sd"] = sdAvailable;
  response["storage"]["littlefs"] = littleFsAvailable;
  sendJson(200, response);
}

void handleStorage() {
  JsonDocument response;
  response["ok"] = true;
  response["littlefs"]["mounted"] = littleFsAvailable;
  response["sd"]["mounted"] = sdAvailable;

  if (sdAvailable) {
    response["sd"]["cardSizeMb"] = SD.cardSize() / (1024ULL * 1024ULL);
    response["sd"]["totalMb"] = SD.totalBytes() / (1024ULL * 1024ULL);
    response["sd"]["usedMb"] = SD.usedBytes() / (1024ULL * 1024ULL);
    response["sd"]["spritesDir"] = SD.exists("/sprites");
  }

  sendJson(200, response);
}

void handleSetupPage() {
  addCorsHeaders();
  server.send(200, "text/html; charset=utf-8", getSetupPageHtml());
}

void handleSaveWifi() {
  String ssid;
  String password;
  String newDeviceName;

  if (server.hasArg("ssid")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    newDeviceName = server.arg("deviceName");
  } else {
    JsonDocument request;
    if (!parseJsonBody(request)) {
      return;
    }

    ssid = request["ssid"] | "";
    password = request["password"] | "";
    newDeviceName = request["deviceName"] | "";
  }

  ssid.trim();
  newDeviceName.trim();

  if (ssid.length() == 0) {
    addCorsHeaders();
    server.send(400, "text/html; charset=utf-8", getSetupPageHtml("SSID 不能为空"));
    return;
  }

  if (newDeviceName.length() == 0) {
    newDeviceName = DEFAULT_DEVICE_NAME;
  }

  preferences.begin("deskpet", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("deviceName", newDeviceName);
  preferences.end();

  addCorsHeaders();
  server.send(200, "text/html; charset=utf-8", getSetupPageHtml("保存成功，设备正在重启..."));
  delay(600);
  ESP.restart();
}

void handleResetWifi() {
  preferences.begin("deskpet", false);
  preferences.clear();
  preferences.end();

  JsonDocument response;
  response["success"] = true;
  response["restarting"] = true;
  sendJson(200, response);
  delay(500);
  ESP.restart();
}

void handleText() {
  JsonDocument request;
  if (!parseJsonBody(request)) {
    return;
  }

  const char* content = request["content"] | request["text"] | "";
  if (strlen(content) == 0) {
    sendError(400, "content is required");
    return;
  }

  // 文本消息使用固定的“收到主人消息”表情，内容直接来自桌面端。
  currentState = "message";
  currentFace = "(=^.^=)";
  currentText = content;
  Serial.print("Text received: ");
  Serial.println(currentText);
  renderMessage(currentText);
  sendSuccess();
}

void handleState() {
  JsonDocument request;
  if (!parseJsonBody(request)) {
    return;
  }

  const char* state = request["state"] | "";
  const PetExpression* expression = findExpression(state);
  if (expression == nullptr) {
    sendError(400, "unsupported state");
    return;
  }

  currentState = expression->state;
  currentFace = expression->face;
  currentText = expression->text;
  Serial.print("State changed: ");
  Serial.println(currentState);
  renderPet(expression->accent);
  sendSuccess();
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }

  sendError(404, "not found");
}

bool connectSavedWiFi() {
  preferences.begin("deskpet", false);
  String ssid = preferences.isKey("ssid") ? preferences.getString("ssid", "") : "";
  String password = preferences.isKey("password") ? preferences.getString("password", "") : "";
  deviceName = preferences.isKey("deviceName") ? preferences.getString("deviceName", DEFAULT_DEVICE_NAME) : DEFAULT_DEVICE_NAME;
  preferences.end();

  ssid.trim();
  if (ssid.length() == 0) {
    Serial.println("No saved WiFi config");
    return false;
  }

  setupMode = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  tft.fillScreen(COLOR_BG);
  drawCenteredString("DeskPet", 76, 4, COLOR_FACE);
  drawCenteredString("Connecting WiFi...", 142, 2, COLOR_TEXT);

  Serial.printf("Connecting to WiFi SSID: %s\n", ssid.c_str());
  uint32_t startMs = millis();
  uint8_t dots = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
    drawCenteredString(String("Connecting WiFi") + String('.', dots % 4), 178, 2, COLOR_MUTED);
    dots++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connect timeout");
    WiFi.disconnect(true);
    return false;
  }

  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void startSetupMode() {
  setupMode = true;
  deviceName = DEFAULT_DEVICE_NAME;
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(SETUP_AP_SSID);

  Serial.println("Setup mode started");
  Serial.print("AP SSID: ");
  Serial.println(SETUP_AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  renderSetupMode();
}

void setupRoutes() {
  // 每个 POST 接口都配一个 OPTIONS 处理，方便浏览器/WebView 预检请求通过。
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/ping", HTTP_OPTIONS, handleOptions);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/storage", HTTP_GET, handleStorage);
  server.on("/storage", HTTP_OPTIONS, handleOptions);
  server.on("/text", HTTP_POST, handleText);
  server.on("/text", HTTP_OPTIONS, handleOptions);
  server.on("/message", HTTP_POST, handleText);
  server.on("/message", HTTP_OPTIONS, handleOptions);
  server.on("/state", HTTP_POST, handleState);
  server.on("/state", HTTP_OPTIONS, handleOptions);
  server.on("/", HTTP_GET, handleSetupPage);
  server.on("/save-wifi", HTTP_POST, handleSaveWifi);
  server.on("/save-wifi", HTTP_OPTIONS, handleOptions);
  server.on("/reset-wifi", HTTP_GET, handleResetWifi);
  server.on("/reset-wifi", HTTP_POST, handleResetWifi);
  server.on("/reset-wifi", HTTP_OPTIONS, handleOptions);
  server.onNotFound(handleNotFound);
  server.begin();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);
  u8f.begin(tft);

  initStorage();

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

  bool wifiConnected = connectSavedWiFi();
  if (!wifiConnected) {
    startSetupMode();
  }

  setupRoutes();

  const PetExpression* idle = findExpression("online");
  if (wifiConnected && idle != nullptr) {
    currentState = idle->state;
    currentFace = idle->face;
    currentText = idle->text;
    renderPet(idle->accent);
  }

  Serial.println("DeskPet HTTP server started");
}

void loop() {
  server.handleClient();
}
