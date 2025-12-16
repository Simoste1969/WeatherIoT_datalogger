#include "network_manager.h"
#include "aux_devices.h"
#include "sd_logger.h"

WebServer server(80);
DNSServer dnsServer;
SystemConfig *currentCfg; 
bool _isApMode = false;

float w_temp=0, w_hum=0, w_pres=0, w_gas=0, w_ntc=0, w_sdU=0, w_sdT=0;
bool w_nmos=false, w_sdOk=false;
String w_time="--:--:--";
int w_rssi=0; 

std::vector<DataPoint> hist;
const int MAX_HIST = 288; 

bool isApMode() { return _isApMode; }

void logToHistory(String time, float temp, float hum) {
    if (hist.size() >= MAX_HIST) hist.erase(hist.begin());
    hist.push_back({time, temp, hum});
}

// Wrapper per chiamare sd_logger
void initHistory() {
    loadHistoryFromSD(hist, MAX_HIST);
}

// ... [handleSetup, handleSave, handleReset, handleFormatSD, handleAutoSave, handleToggle - INVARIATI DALLA V5.4] ...
// (Copia le funzioni dalla versione precedente se necessario, qui riporto solo Dashboard e modifiche)

void handleSetup() {
  String h = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:sans-serif;padding:20px;max-width:500px;margin:auto;background:#f4f7f6;}h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;}input{width:100%;padding:10px;margin:5px 0;border:1px solid #bdc3c7;border-radius:4px;box-sizing:border-box;}button{width:100%;padding:12px;color:#fff;border:none;border-radius:5px;cursor:pointer;margin-top:10px;font-weight:bold;font-size:1rem;}.btn-save{background:#2980b9;}.btn-back{background:#7f8c8d;}.btn-wipe{background:#c0392b;margin-top:20px;}</style></head><body>";
  h += "<h2>Setup Sistema</h2>";
  h += "<form action='/save' method='POST'>";
  h += "<h3>Connessione WiFi</h3>SSID:<input type='text' name='ssid' value='" + String(currentCfg->ssid) + "' required>Password:<input type='password' name='pass'>";
  h += "<h3>Cloud ThingSpeak</h3><label><input type='checkbox' name='ts_en' value='1' " + String(currentCfg->tsEnabled?"checked":"") + " style='width:auto;'> Abilita Invio Dati</label><br>";
  h += "API Key:<input type='text' name='ts_key' value='" + String(currentCfg->tsKey) + "'>";
  h += "<button type='submit' class='btn-save'>SALVA E RIAVVIA</button></form>";
  h += "<hr><button onclick=\"if(confirm('Cancellare TUTTI i dati sulla SD?')) location.href='/format_sd'\" class='btn-wipe'>FORMATTA SCHEDA SD</button>";
  h += "<button onclick=\"location.href='/'\" class='btn-back'>TORNA ALLA DASHBOARD</button></body></html>";
  server.send(200, "text/html", h);
}
void handleSave() {
  String s=server.arg("ssid"); String p=server.arg("pass");
  memset(currentCfg->ssid,0,32); strncpy(currentCfg->ssid, s.c_str(), 31);
  memset(currentCfg->pass,0,64); strncpy(currentCfg->pass, p.c_str(), 63);
  currentCfg->tsEnabled = server.hasArg("ts_en");
  memset(currentCfg->tsKey,0,20); strncpy(currentCfg->tsKey, server.arg("ts_key").c_str(), 19);
  saveSystemConfig(*currentCfg);
  server.send(200, "text/html", "<h1>Salvato! Riavvio...</h1>"); delay(1000); ESP.restart();
}
void handleReset() { wipeSystemConfig(); server.send(200, "text/html", "<h1>Reset OK. Riavvio...</h1>"); delay(1000); ESP.restart(); }
void handleFormatSD() { wipeSDCard(); server.send(200, "text/html", "<h1>SD Formattata</h1><br><a href='/setup'>Indietro</a>"); }
void handleAutoSave() { currentCfg->nmosAutoMode=server.hasArg("auto"); if(server.hasArg("min")) currentCfg->humThreshMin=server.arg("min").toFloat(); if(server.hasArg("max")) currentCfg->humThreshMax=server.arg("max").toFloat(); saveSystemConfig(*currentCfg); server.send(200, "text/plain", "OK"); }
void handleToggle() { if(!currentCfg->nmosAutoMode){ bool n=!getNMOSState(); setNMOSState(n); server.send(200, "text/plain", n?"ON":"OFF"); } else server.send(400,"text/plain","Auto"); }

void handleDashboard() {
  String L="[", dT="[", dH="[";
  for(size_t i=0; i<hist.size(); i++) {
      // MODIFICA QUI: Entrambi con 1 decimale
      L+="\""+hist[i].time+"\","; 
      dT+=String(hist[i].temp,1)+","; 
      dH+=String(hist[i].hum,1)+","; 
  }
  if(L.length()>1){L.remove(L.length()-1);dT.remove(dT.length()-1);dH.remove(dH.length()-1);} 
  L+="]"; dT+="]"; dH+="]";

  String h = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>WeatherIoT</title>";
  h += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  h += "<style>";
  h += "body { font-family: 'Segoe UI', Helvetica, sans-serif; margin: 0; background: #ecf0f1; text-align: center; color: #333; }";
  h += ".header { background: #2c3e50; color: #fff; padding: 15px 20px; display: flex; justify-content: space-between; align-items: center; box-shadow: 0 2px 8px rgba(0,0,0,0.2); }";
  h += ".brand h1 { margin: 0; font-size: 1.1rem; } .brand small { color: #bdc3c7; font-size: 0.75rem; display:block;}";
  h += ".status-bar { display: flex; align-items: center; gap: 15px; }";
  h += ".wifi-ico { display: flex; align-items: flex-end; gap: 2px; width: 20px; height: 16px; }";
  h += ".w-bar { width: 4px; background: rgba(255,255,255,0.3); border-radius: 1px; } .w-bar.on { background: #2ecc71; } .b1{height:25%} .b2{height:50%} .b3{height:75%} .b4{height:100%}";
  h += ".sd-ico { position: relative; width: 18px; height: 22px; background: #fff; border-radius: 2px; color:#333; font-weight:bold; font-size:10px; display:flex; align-items:center; justify-content:center; }";
  h += ".sd-ico.err { background: #e74c3c; color: white; } .sd-ico::after { content:''; position:absolute; top:2px; right:2px; width:4px; height:4px; background:#bdc3c7; }";
  h += ".sd-info { text-align: left; font-size: 0.7rem; color: #bdc3c7; line-height: 1.1; }";
  h += ".btn { padding: 5px 12px; border-radius: 4px; font-weight: bold; text-decoration: none; border: none; cursor: pointer; font-size: 0.8rem; }";
  h += ".btn-stp { border: 1px solid #fff; background: transparent; color: #fff; } .btn-stp:hover{ background:#fff; color:#2c3e50; }";
  h += ".btn-rst { background: #c0392b; color: #fff; }";
  h += ".container { max-width: 1000px; margin: 20px auto; padding: 0 10px; }";
  h += ".last-upd { color: #7f8c8d; font-size: 0.9rem; margin-bottom: 20px; }";
  h += ".cards { display: flex; flex-wrap: wrap; justify-content: center; gap: 15px; margin-bottom: 30px; }";
  h += ".card { background: #fff; padding: 15px; border-radius: 8px; width: 130px; box-shadow: 0 2px 5px rgba(0,0,0,0.05); }";
  h += ".card p { margin: 0; font-size: 0.75rem; color: #7f8c8d; text-transform: uppercase; }";
  h += ".card h2 { margin: 5px 0 0; font-size: 1.8rem; color: #2c3e50; }";
  h += ".charts { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 40px; margin-left: auto; margin-right: auto; max-width: 1000px;}";
  h += ".chart-box { background: #fff; padding: 10px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.05); }";
  h += ".panel { background: #fff; border-left: 5px solid #f1c40f; padding: 15px; border-radius: 5px; margin: 0 auto 30px; max-width: 600px; text-align: left; box-shadow: 0 2px 5px rgba(0,0,0,0.05); }";
  h += ".p-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }";
  h += ".toggle-sw { position: relative; width: 40px; height: 20px; } .toggle-sw input { opacity: 0; width: 0; height: 0; }";
  h += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 20px; }";
  h += ".slider:before { position: absolute; content: ''; height: 14px; width: 14px; left: 3px; bottom: 3px; background-color: white; transition: .4s; border-radius: 50%; }";
  h += "input:checked + .slider { background-color: #2196F3; } input:checked + .slider:before { transform: translateX(20px); }";
  h += "@media (max-width: 768px) { .charts { grid-template-columns: 1fr; } .header { flex-direction: column; gap: 10px; text-align: center; } .status-bar { flex-wrap:wrap; justify-content:center; } }";
  h += ".insta { display: inline-block; background: linear-gradient(45deg, #f09433, #e6683c, #dc2743, #cc2366, #bc1888); color: white; padding: 12px 30px; border-radius: 50px; text-decoration: none; font-weight: bold; box-shadow: 0 4px 10px rgba(0,0,0,0.2); transition: transform 0.2s; } .insta:hover { transform: scale(1.05); }";
  h += "</style></head><body>";

  h += "<div class='header'><div class='brand'><h1>WeatherIoT Datalogger</h1><small>made by Klinkon Electronics</small></div>";
  h += "<div class='status-bar'><div style='display:flex;align-items:center;gap:5px'><div class='wifi-ico'><div class='w-bar b1'></div><div class='w-bar b2'></div><div class='w-bar b3'></div><div class='w-bar b4'></div></div><span id='rssi' style='font-size:0.7rem'>--</span></div>";
  h += "<div style='display:flex;align-items:center;gap:5px'><div class='sd-ico' id='sdI'>SD</div><div class='sd-info'><div id='sdGb'>--GB</div><div id='sdPct'>--%</div></div></div>";
  h += "<a href='/setup' class='btn btn-stp'>SETUP</a><button onclick=\"if(confirm('Reset Totale?')) location.href='/reset'\" class='btn btn-rst'>RESET</button></div></div>";

  h += "<div class='container'><div class='last-upd'>Ultimo Aggiornamento: <span id='tm'>" + w_time + "</span></div>";
  h += "<div class='cards'><div class='card'><p>Temperatura</p><h2 id='vT' style='color:#e74c3c'>--</h2>&deg;C</div><div class='card'><p>Umidit&agrave;</p><h2 id='vH' style='color:#3498db'>--</h2>%</div><div class='card'><p>Pressione</p><h2 id='vP'>--</h2>hPa</div><div class='card'><p>Qualit&agrave; Aria</p><h2 id='vG' style='color:#27ae60'>--</h2>%</div>";
#ifdef ENABLE_NTC_SENSOR
  h += "<div class='card'><p>Sonda NTC</p><h2 id='vN' style='color:#e67e22'>--</h2>&deg;C</div>";
#endif
  h += "</div>";

  h += "<div class='charts'><div class='chart-box'><canvas id='cT'></canvas></div><div class='chart-box'><canvas id='cH'></canvas></div></div>";

#ifdef ENABLE_NMOS_LOAD
  h += "<div class='panel'><h3>Gestione Carico (NMOS)</h3><div class='p-row'><span><b>Modalit&agrave; Automatica</b><small><br>Attiva Carico se Umidit&agrave; > MAX</small></span><label class='toggle-sw'><input type='checkbox' id='autoSw' onchange='svAuto()' " + String(currentCfg->nmosAutoMode?"checked":"") + "><span class='slider'></span></label></div>";
  h += "<div id='autoSet' style='opacity:" + String(currentCfg->nmosAutoMode?"1":"0.5") + "; display:flex; gap:10px; align-items:center; margin-bottom:10px;'>ON > <input type='number' id='mx' value='" + String(currentCfg->humThreshMax) + "' style='width:50px'>% OFF < <input type='number' id='mn' value='" + String(currentCfg->humThreshMin) + "' style='width:50px'>% <button onclick='svAuto()' style='font-size:0.7rem'>SALVA SOGLIE</button></div>";
  h += "<div class='p-row' style='border-top:1px solid #eee; padding-top:10px'><span>Stato Attuale: <b id='stT' style='color:" + String(w_nmos?"green":"red") + "'>" + String(w_nmos?"ATTIVO":"SPENTO") + "</b></span><button id='manBtn' onclick='tgMan()' class='btn' style='background:#2ecc71;color:white' " + String(currentCfg->nmosAutoMode?"disabled":"") + ">SWITCH MANUALE</button></div></div>";
#endif

  h += "<a href='https://www.instagram.com/klinkon_electronics/' target='_blank' class='insta'>Seguimi su Instagram</a><br><br></div>";

  h += "<script>const cOpt={responsive:true, maintainAspectRatio:false, elements:{point:{radius:0}}, scales:{x:{ticks:{maxTicksLimit:8}}}};";
  h += "const cT=new Chart(document.getElementById('cT'),{type:'line',data:{labels:"+L+",datasets:[{label:'Temperatura (°C)',borderColor:'#e74c3c',backgroundColor:'rgba(231,76,60,0.1)',fill:true,data:"+dT+"}]},options:cOpt});";
  h += "const cH=new Chart(document.getElementById('cH'),{type:'line',data:{labels:"+L+",datasets:[{label:'Umidità (%)',borderColor:'#3498db',backgroundColor:'rgba(52,152,235,0.1)',fill:true,data:"+dH+"}]},options:cOpt});";
  h += "function upW(r){ const b=document.querySelectorAll('.w-bar'); b.forEach(e=>e.classList.remove('on')); if(r>-90)b[0].classList.add('on'); if(r>-75)b[1].classList.add('on'); if(r>-65)b[2].classList.add('on'); if(r>-55)b[3].classList.add('on'); document.getElementById('rssi').innerText=r+'dBm'; }";
  h += "function svAuto(){ const a=document.getElementById('autoSw').checked; const mn=document.getElementById('mn').value; const mx=document.getElementById('mx').value; document.getElementById('manBtn').disabled=a; document.getElementById('autoSet').style.opacity=a?'1':'0.5'; fetch('/auto_save?auto='+(a?1:0)+'&min='+mn+'&max='+mx); }";
  h += "function tgMan(){ fetch('/toggle').then(r=>r.text()).then(t=>{ const s=document.getElementById('stT'); s.innerText=(t=='ON'?'ATTIVO':'SPENTO'); s.style.color=(t=='ON'?'green':'red'); }); }";

  h += "setInterval(()=>{ fetch('/data').then(r=>r.json()).then(j=>{";
  h += "  document.getElementById('tm').innerText=j.t; document.getElementById('vT').innerText=j.T.toFixed(1); document.getElementById('vH').innerText=j.H.toFixed(0);";
  h += "  document.getElementById('vP').innerText=j.P.toFixed(0); document.getElementById('vG').innerText=j.G.toFixed(0);";
#ifdef ENABLE_NTC_SENSOR
  h += "  document.getElementById('vN').innerText=j.N.toFixed(1);";
#endif
#ifdef ENABLE_NMOS_LOAD
  h += "  const s=document.getElementById('stT'); s.innerText=j.M?'ATTIVO':'SPENTO'; s.style.color=j.M?'green':'red';";
#endif
  h += "  upW(j.r);";
  h += "  const sd=document.getElementById('sdI'); const sdG=document.getElementById('sdGb'); const sdP=document.getElementById('sdPct');";
  h += "  if(j.sO){ sd.classList.remove('err'); sd.innerText='SD'; sdG.innerText=j.sU.toFixed(2)+'/'+j.sT.toFixed(2)+'GB'; sdP.innerText=((j.sU/j.sT)*100).toFixed(0)+'%'; }";
  h += "  else { sd.classList.add('err'); sd.innerText='!'; sdG.innerText='NO SD'; sdP.innerText=''; }";
  h += "  if(cT.data.labels.slice(-1)[0]!==j.t){";
  h += "    cT.data.labels.push(j.t); cT.data.datasets[0].data.push(j.T); cH.data.labels.push(j.t); cH.data.datasets[0].data.push(j.H);";
  h += "    if(cT.data.labels.length>288){ cT.data.labels.shift(); cT.data.datasets[0].data.shift(); cH.data.labels.shift(); cH.data.datasets[0].data.shift(); }";
  h += "    cT.update('none'); cH.update('none');";
  h += "  }}); }, 5000);</script></body></html>";
  server.send(200, "text/html", h);
}

void handleData() {
  String j = "{";
  j += "\"T\":"+String(w_temp)+",\"H\":"+String(w_hum)+",\"P\":"+String(w_pres)+",\"G\":"+String(w_gas)+",";
  j += "\"N\":"+String(w_ntc)+",\"M\":"+String(w_nmos?"true":"false")+",";
  j += "\"t\":\""+w_time+"\",\"r\":"+String(w_rssi)+",\"sO\":"+String(w_sdOk?"true":"false")+",";
  j += "\"sU\":"+String(w_sdU,2)+",\"sT\":"+String(w_sdT,2)+"}";
  server.send(200, "application/json", j);
}

void handleCaptivePortal() { server.sendHeader("Location", "http://192.168.4.1/", true); server.send(302, "text/plain", ""); }

void setupNetwork(SystemConfig &cfg) {
  currentCfg = &cfg; 
  WiFi.disconnect(true); WiFi.mode(WIFI_OFF); delay(100);

  if (String(cfg.ssid).length() == 0 || String(cfg.ssid) == "null") _isApMode = true;
  else {
      WiFi.mode(WIFI_STA);
      if(cfg.useStaticIP) {
         IPAddress ip,gw,sub; 
         if(ip.fromString(cfg.staticIP) && gw.fromString(cfg.gateway) && sub.fromString("255.255.255.0")) WiFi.config(ip, gw, sub);
      }
      WiFi.begin(cfg.ssid, cfg.pass);
      int i=0; while(WiFi.status()!=WL_CONNECTED && i<20){ delay(500); i++; }
      _isApMode = (WiFi.status()!=WL_CONNECTED);
  }

  if(_isApMode) {
    WiFi.disconnect(); WiFi.mode(WIFI_AP);
    IPAddress apIP(192, 168, 4, 1); WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError); dnsServer.start(53, "*", apIP);
    server.on("/", handleSetup); server.on("/save", HTTP_POST, handleSave);
    server.on("/generate_204", handleSetup); server.onNotFound(handleCaptivePortal);
  } else {
    server.on("/", handleDashboard); server.on("/data", handleData);
    server.on("/setup", handleSetup); server.on("/save", HTTP_POST, handleSave); server.on("/reset", handleReset);
    server.on("/format_sd", handleFormatSD); 
    server.on("/auto_save", handleAutoSave); server.on("/toggle", handleToggle);
  }
  server.begin();
}

void handleClient() { 
    if(_isApMode) dnsServer.processNextRequest(); 
    server.handleClient(); 
}

void updateSensorData(float temp, float hum, float pres, float gasPct, float ntcTemp, bool nmosSt, String timeStr, int rssi, bool sdOk, float sdUsed, float sdTot) {
  w_temp=temp; w_hum=hum; w_pres=pres; w_gas=gasPct; w_ntc=ntcTemp; w_nmos=nmosSt;
  w_time=timeStr; w_rssi=rssi; w_sdOk=sdOk; w_sdU=sdUsed; w_sdT=sdTot;
}