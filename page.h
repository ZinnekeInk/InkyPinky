// page.h  -  the Inky Pinky web page (must stay in its own file: the Arduino IDE
// would otherwise try to read the JavaScript below as C++)
#pragma once
#include <Arduino.h>

const char PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="only light">
<title>Inky Pinky</title>
<style>
*{box-sizing:border-box}
html{color-scheme:only light}
body{margin:0;background:#fff;color:#000;font-family:system-ui,-apple-system,"Segoe UI",Roboto,Arial,sans-serif}
header{border-bottom:1.5px solid #000;padding:14px 16px;display:flex;align-items:center;justify-content:space-between;gap:12px}
header b{font-size:20px;font-weight:500;cursor:pointer;color:#C8007F}
header b.sub{color:#000}
header small{display:block;font-size:12px;margin-top:2px}
nav{display:none;gap:6px}
nav a{padding:8px 16px;font-size:14px;cursor:pointer;border-radius:8px;border:1.5px solid transparent}
nav a.on{border-color:#000;font-weight:bold}
.view{display:none;padding:10px;max-width:1280px;margin:0 auto}.view.on{display:block}
.card{border:1.5px solid #000;border-radius:12px;padding:14px;margin-bottom:10px}
.t{font-size:12px;margin:0 0 8px}
.sec{font-weight:bold;font-size:13px;margin:0 0 8px}
.pen{display:flex;align-items:center;justify-content:space-between;gap:10px}
#badge{font-size:28px;font-weight:bold;padding:8px 22px;min-width:120px;text-align:center;border-radius:12px;border:1.5px solid #000}
#badge.dn{border:5px solid #C8007F;color:#C8007F}
#why{font-size:14px}
.btns{display:grid;grid-template-columns:1fr 1fr;gap:8px}
button{font:inherit;font-weight:bold;font-size:15px;height:44px;cursor:pointer;padding:0 12px;border-radius:10px;background:#fff;color:#000;border:1.5px solid #000}
.bd{border-width:4px}.wide{width:100%;margin-top:6px}
.row{display:flex;align-items:center;gap:10px;margin:12px 0;font-size:14px}
.row .n{width:92px;flex:none}.row .v{width:44px;text-align:right;font-weight:bold;flex:none}
input[type=range]{flex:1;min-width:0;-webkit-appearance:none;appearance:none;height:20px;background:transparent}
input[type=range]::-webkit-slider-runnable-track{height:8px;border:1.5px solid #000;border-radius:4px;background:#fff}
input[type=range]::-moz-range-track{height:8px;border:1.5px solid #000;border-radius:4px;background:#fff}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;margin-top:-6.5px;border-radius:50%;background:#fff;border:1.5px solid #000}
input[type=range]::-moz-range-thumb{width:16px;height:16px;border-radius:50%;background:#fff;border:1.5px solid #000}
.note{font-size:12px}
.lb{font-size:12px;margin:0 0 3px}
.in{width:100%;height:38px;border:1.5px solid #000;background:#fff;padding:0 10px;font:inherit;font-size:14px;margin-bottom:8px;border-radius:8px}
.err{color:#C8007F;font-size:13px;min-height:16px}
.link{display:flex;justify-content:space-between;font-weight:bold;cursor:pointer}
.prog{height:12px;border:1.5px solid #000;margin:8px 0 4px;border-radius:5px;overflow:hidden}
#pg{height:100%;width:0;background:repeating-linear-gradient(90deg,#000 0 1px,transparent 1px 4px)}
.coffee{display:flex;flex-direction:column;gap:12px}
.cbtn{display:flex;align-items:center;justify-content:center;height:44px;padding:0 20px;border:1.5px solid #C8007F;border-radius:10px;color:#C8007F;font-weight:bold;font-size:15px;text-decoration:none;white-space:nowrap}
@media(min-width:800px){
 nav{display:flex}.link{display:none}
 header{padding:0 32px;height:64px}header b{font-size:22px}header small{display:inline;margin-left:14px}
 .view{padding:24px 32px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:20px}
 .card{padding:20px 24px;margin-bottom:20px}
 #badge{font-size:31px;padding:10px 31px;min-width:150px}button,.cbtn{height:52px;font-size:16px}
 .coffee{flex-direction:row;align-items:center;justify-content:space-between;gap:32px}
}
</style></head><body>
<header><div><b id="ttl" onclick="go('control')">Inky Pinky</b><small id="sub">connecting…</small></div>
<nav><a id="n_control" class="on" onclick="go('control')">Control</a><a id="n_wifi" onclick="go('wifi')">WiFi</a><a id="n_fw" onclick="go('fw')">Firmware</a></nav></header>

<div id="v_control" class="view on"><div class="grid"><div>
 <div class="card pen"><div><p class="t">Pen</p><div id="why">…</div></div><div id="badge">UP</div></div>
 <div class="card"><p class="t">Test</p><div class="btns">
  <button onclick="pen('up')">Up</button><button class="bd" onclick="pen('down')">Down</button></div></div>
</div><div>
 <div class="card"><p class="t">Angles · the servo moves while you slide</p>
  <div class="row"><span class="n">Up angle</span><input type="range" id="ua" min="15" max="165" oninput="sl('ua')"><span class="v" id="uav"></span></div>
  <div class="row"><span class="n">Down angle</span><input type="range" id="da" min="15" max="165" oninput="sl('da')"><span class="v" id="dav"></span></div>
  <button class="wide" id="svb" onclick="save()">Save</button></div>
 <div class="card link" onclick="go('wifi')"><span>WiFi</span><span>›</span></div>
 <div class="card link" onclick="go('fw')"><span>Firmware</span><span>›</span></div>
</div></div>
 <div class="card coffee"><div><p class="sec">Enjoying Inky Pinky?</p>
  <p class="note" style="margin:0">It’s made by one tinkerer in spare evenings, and it’s free. If it helps, a coffee is always welcome and keeps the next idea brewing.</p></div>
  <a class="cbtn" href="https://buymeacoffee.com/zoranpopovici" target="_blank" rel="noopener">Buy me a coffee</a></div>
</div>

<div id="v_wifi" class="view"><div class="grid">
 <div class="card"><p class="sec">Own hotspot</p>
  <p class="lb">Name</p><input class="in" id="apn">
  <p class="lb">Password (8+ characters, leave empty to keep)</p><input class="in" id="app" type="password" autocomplete="new-password"></div>
 <div class="card"><p class="sec">Home network</p>
  <p class="lb">Network</p><input class="in" id="ss" list="nets" placeholder="Pick or type a network"><datalist id="nets"></datalist>
  <p class="lb">Password (leave empty to keep)</p><input class="in" id="spw" type="password" autocomplete="new-password">
  <div class="note" id="sta">…</div></div>
 </div>
 <div class="err" id="werr"></div>
 <button class="wide" onclick="saveWifi()">Save and restart</button>
</div>

<div id="v_fw" class="view"><div class="grid"><div>
 <div class="card"><p class="sec">Update</p>
  <p class="note">Pick a .bin file. In Arduino IDE: Sketch › Export compiled binary.</p>
  <input type="file" id="bin" accept=".bin" style="margin:8px 0;font-size:13px;max-width:100%">
  <div class="prog"><div id="pg"></div></div><div class="note" id="pgt">Pen stays up during the update</div>
  <div class="err" id="ferr"></div>
  <button class="wide" onclick="upload()">Upload and restart</button></div>
 </div><div>
 <div class="card"><p class="sec">Device</p><p class="note" style="margin:0">Version <b id="iv"></b></p></div>
 <div class="btns"><button onclick="restart()">Restart</button><button onclick="reset()">Factory reset</button></div>
</div></div></div>

<script>
const $=i=>document.getElementById(i);
let loaded=false,view='control',S={},last={},pend={};
function wide(){return matchMedia('(min-width:800px)').matches}
function title(){const m=view=='control'||wide();$('ttl').textContent=m?'Inky Pinky':'‹ '+(view=='wifi'?'WiFi':'Firmware');$('ttl').classList.toggle('sub',!m)}
function go(v){view=v;['control','wifi','fw'].forEach(x=>{$('v_'+x).classList.toggle('on',x==v);$('n_'+x).classList.toggle('on',x==v)});title();if(v=='wifi'){wifiFill();scan()}}
function sl(k){$(k+'v').textContent=$(k).value+'°';const q=k+'='+$(k).value,n=Date.now();clearTimeout(pend[k]);
 if(n-(last[k]||0)>80){last[k]=n;fetch('/api/angle?'+q).catch(()=>{})}else pend[k]=setTimeout(()=>sl(k),80)}
function pen(s){fetch('/api/pen?s='+s)}
function save(){fetch('/api/save',{method:'POST'}).then(r=>{$('svb').textContent=r.ok?'Saved':'Not saved';setTimeout(()=>$('svb').textContent='Save',1200)})}
function render(s){S=s;const d=s.pen=='down';
 $('badge').textContent=d?'DOWN':'UP';$('badge').classList.toggle('dn',d);
 $('why').textContent=s.upd?'Updating · pen up':s.man>0?'Test · '+s.man+' s left':(s.laser?'Laser on · pen down':'Laser off · pen up');
 $('sub').textContent=s.ap+' · '+location.hostname+' · v'+s.ver;$('iv').textContent=s.ver;
 $('sta').textContent=s.staOk?'● Connected · '+s.staip:(s.sta?'○ Not connected to '+s.sta:'○ No home network set');
 if(!loaded){loaded=true;['ua','da'].forEach(k=>{$(k).value=s[k];$(k+'v').textContent=s[k]+'°'});wifiFill()}}
async function poll(){try{render(await(await fetch('/api/state')).json())}catch(e){$('why').textContent='No connection…'}setTimeout(poll,200)}
function wifiFill(){if(S.ap&&!$('apn').value)$('apn').value=S.ap;if(S.sta&&!$('ss').value)$('ss').value=S.sta}
async function scan(){try{const r=await(await fetch('/api/scan')).json();
 if(r.running){if(view=='wifi')setTimeout(scan,1500);return}
 $('nets').innerHTML='';r.nets.forEach(n=>{const o=document.createElement('option');o.value=n;$('nets').appendChild(o)})}catch(e){}}
function saveWifi(){const p=$('app').value;$('werr').textContent='';
 if(p&&p.length<8){$('werr').textContent='Hotspot password needs 8 or more characters.';return}
 const b=new URLSearchParams({apn:$('apn').value.trim(),app:p,ss:$('ss').value.trim(),spw:$('spw').value});
 fetch('/api/wifi',{method:'POST',body:b}).then(r=>r.text().then(t=>{$('werr').textContent=r.ok?'Saved. Restarting… reconnect to the hotspot or home network.':t}))}
function upload(){const f=$('bin').files[0];$('ferr').textContent='';
 if(!f||!f.name.toLowerCase().endsWith('.bin')){$('ferr').textContent='Choose a .bin file first.';return}
 const x=new XMLHttpRequest(),fd=new FormData();fd.append('firmware',f,f.name);
 x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);$('pg').style.width=p+'%';$('pgt').textContent='Uploading · '+p+'% · pen stays up'}};
 x.onload=()=>{if(x.status==200&&x.responseText=='OK'){$('pgt').textContent='Done. Restarting…';setTimeout(()=>location.reload(),9000)}else $('ferr').textContent='Update failed: '+x.responseText};
 x.onerror=()=>$('ferr').textContent='Upload interrupted.';
 x.open('POST','/update');x.send(fd)}
function restart(){if(confirm('Restart Inky Pinky?'))fetch('/api/restart',{method:'POST'})}
function reset(){if(confirm('Factory reset? WiFi and angles go back to defaults.'))fetch('/api/reset',{method:'POST'})}
addEventListener('resize',title);
poll();
</script></body></html>)rawliteral";
