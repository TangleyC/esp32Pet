#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

namespace {
constexpr uint16_t COLOR_BG = TFT_BLACK;
constexpr uint16_t COLOR_FACE = TFT_CYAN;
constexpr uint16_t COLOR_TEXT = TFT_WHITE;
constexpr uint16_t COLOR_MUTED = TFT_DARKGREY;
constexpr uint16_t COLOR_OK = TFT_GREEN;
constexpr uint16_t COLOR_ERROR = TFT_RED;

TFT_eSPI tft;
WebServer server(80);

String currentState = "idle";
String currentFace = "(^_^)";
String currentText = "待机中";

struct PetExpression {
  const char* state;
  const char* face;
  const char* text;
  uint16_t accent;
};

const PetExpression expressions[] = {
  {"idle", "(•‿•)", "待机中", TFT_CYAN},
  {"sleep", "(-_-) Zzz", "", TFT_BLUE},
  {"coding", "(⌐■_■)", "正在写代码...", TFT_YELLOW},
  {"error", "(⊙﹏⊙)", "程序出错啦", TFT_RED},
  {"success", "(≧▽≦)", "任务完成", TFT_GREEN},
  {"happy", "(^o^)", "今天真开心", TFT_MAGENTA},
};

void addCorsHeaders() {
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

void renderPet(uint16_t accent = COLOR_FACE) {
  tft.fillScreen(COLOR_BG);
  tft.drawRoundRect(8, 8, tft.width() - 16, tft.height() - 16, 12, accent);
  tft.drawFastHLine(20, 58, tft.width() - 40, COLOR_MUTED);

  drawCenteredString("DeskPet", 34, 2, COLOR_MUTED);
  drawCenteredString(currentFace, 118, 4, accent);

  if (currentText.length() > 0) {
    drawWrappedText(currentText, 24, 164, tft.width() - 48, 22, COLOR_TEXT);
  }

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.drawString(WiFi.localIP().toString(), tft.width() / 2, tft.height() - 18, 2);
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

void handlePing() {
  JsonDocument response;
  response["ok"] = true;
  response["device"] = DEVICE_NAME;
  sendJson(200, response);
}

void handleText() {
  JsonDocument request;
  if (!parseJsonBody(request)) {
    return;
  }

  const char* content = request["content"] | "";
  if (strlen(content) == 0) {
    sendError(400, "content is required");
    return;
  }

  currentFace = "(=^･ω･^=)";
  currentText = content;
  renderPet(COLOR_FACE);
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

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  tft.fillScreen(COLOR_BG);
  drawCenteredString("DeskPet", 76, 4, COLOR_FACE);
  drawCenteredString("Connecting WiFi...", 142, 2, COLOR_TEXT);

  Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
  uint8_t dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    drawCenteredString(String("Connecting WiFi") + String('.', dots % 4), 178, 2, COLOR_MUTED);
    dots++;
  }

  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void setupRoutes() {
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/ping", HTTP_OPTIONS, handleOptions);
  server.on("/text", HTTP_POST, handleText);
  server.on("/text", HTTP_OPTIONS, handleOptions);
  server.on("/state", HTTP_POST, handleState);
  server.on("/state", HTTP_OPTIONS, handleOptions);
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

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

  connectWiFi();
  setupRoutes();

  const PetExpression* idle = findExpression("idle");
  if (idle != nullptr) {
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
