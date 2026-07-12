const $ = (id) => document.getElementById(id);
let current = null;
let simulationTimer = null;
let nextSimOrderId = 100000;
const MAX_HISTORY = 180;
const history = { mid: [], micro: [], spread: [], imbalance: [], latency: [], volume: [], trades: [] };

function formatNumber(value, digits = 0) {
  if (value === null || value === undefined || Number.isNaN(value)) return '—';
  return new Intl.NumberFormat(undefined, {maximumFractionDigits: digits}).format(value);
}
function feedback(text, error = false) {
  $('feedback').textContent = text;
  $('feedback').style.color = error ? 'var(--sell)' : 'var(--muted)';
}
async function post(path, fields = {}) {
  const response = await fetch(path, {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams(fields)});
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || 'Request rejected');
  return data;
}
function topMetrics(data) {
  const bid = data.bids[0]; const ask = data.asks[0];
  const spread = bid && ask ? ask.price - bid.price : null;
  const mid = bid && ask ? (ask.price + bid.price) / 2 : null;
  const micro = bid && ask && bid.quantity + ask.quantity > 0
    ? (ask.price * bid.quantity + bid.price * ask.quantity) / (bid.quantity + ask.quantity) : mid;
  const bidDepth = data.bids.slice(0,5).reduce((s,x)=>s+x.quantity,0);
  const askDepth = data.asks.slice(0,5).reduce((s,x)=>s+x.quantity,0);
  const imbalance = bidDepth + askDepth ? bidDepth / (bidDepth + askDepth) : .5;
  return {bid, ask, spread, mid, micro, bidDepth, askDepth, imbalance};
}
function renderMetrics(metrics, derived) {
  const values = [
    ['Submitted', metrics.submitted], ['Accepted', metrics.accepted], ['Rejected', metrics.rejected],
    ['Trades', metrics.trades], ['Volume', metrics.tradedQuantity], ['Active orders', metrics.activeOrders],
    ['Spread', derived.spread], ['Imbalance', `${formatNumber(derived.imbalance * 100,1)}%`]
  ];
  $('metrics').innerHTML = values.map(([label,value]) => `<div class="metric"><div class="metric-label">${label}</div><div class="metric-value">${typeof value === 'number' ? formatNumber(value,2) : value}</div></div>`).join('');
}
function renderBook(target, rows, side) {
  const max = Math.max(1,...rows.map(x=>x.quantity));
  const ordered = side === 'ask' ? [...rows].reverse() : rows;
  $(target).innerHTML = ordered.length ? ordered.map(row => `<div class="book-row"><div class="depth-bar" style="width:${Math.max(3,row.quantity/max*100)}%"></div><span class="${side}-price">${row.price}</span><span>${formatNumber(row.quantity)}</span><span>${row.orders}</span><span>${'▰'.repeat(Math.max(1,Math.round(row.quantity/max*12)))}</span></div>`).join('') : '<div class="empty">No resting orders</div>';
}
function renderSpread(data, d) {
  $('spread').innerHTML = `<div class="spread-item"><small>Best bid</small><span class="bid-price">${d.bid?.price ?? '—'}</span></div><div class="spread-item"><small>Mid / spread</small><span>${d.mid === null ? '—' : formatNumber(d.mid,2)} / ${d.spread ?? '—'}</span></div><div class="spread-item"><small>Best ask</small><span class="ask-price">${d.ask?.price ?? '—'}</span></div>`;
  $('imbalanceBadge').textContent = `Imbalance ${formatNumber(d.imbalance*100,1)}% bid`;
  $('micropriceBadge').textContent = `Microprice ${formatNumber(d.micro,2)}`;
}
function renderTrades(trades) {
  $('trades').innerHTML = trades.length ? trades.map(t=>`<div class="feed-row"><span>#${t.sequence}</span><span class="${t.side.toLowerCase()}">${t.side} ${t.quantity} @ ${t.price}</span><span>${t.maker}→${t.taker}</span></div>`).join('') : '<div class="empty">No trades yet</div>';
}
function renderReports(reports) {
  $('reports').innerHTML = reports.length ? reports.map(r=>`<div class="feed-row"><span>#${r.sequence}</span><span class="${r.status.toLowerCase()}">${r.status}</span><span>id ${r.orderId} · rem ${r.remaining}</span></div>`).join('') : '<div class="empty">No reports yet</div>';
}
function renderSymbols(symbols, selected) {
  const all = symbols.length ? symbols : [1];
  $('viewSymbol').innerHTML = all.map(s=>`<option value="${s}" ${Number(selected)===Number(s)?'selected':''}>${s}</option>`).join('');
}
function pushHistory(key, value) {
  history[key].push(value); if (history[key].length > MAX_HISTORY) history[key].shift();
}
function css(name) { return getComputedStyle(document.documentElement).getPropertyValue(name).trim(); }
function svgSetup(svg) {
  const width = 800;
  const height = Number(svg.dataset.height || 220);
  svg.setAttribute('viewBox', `0 0 ${width} ${height}`);
  while (svg.firstChild) svg.removeChild(svg.firstChild);
  return {svg, width, height};
}
function svgNode(name, attrs={}) {
  const node=document.createElementNS('http://www.w3.org/2000/svg',name);
  Object.entries(attrs).forEach(([key,value])=>node.setAttribute(key,String(value)));
  return node;
}
function svgText(svg,x,y,text,attrs={}) {
  const node=svgNode('text',{x,y,...attrs}); node.textContent=text; svg.appendChild(node); return node;
}
function drawGrid(svg,w,h,pad=30) {
  for(let i=0;i<5;i++){
    const y=pad+(h-pad*2)*i/4;
    svg.appendChild(svgNode('line',{x1:pad,y1:y,x2:w-pad,y2:y,stroke:css('--line'),'stroke-width':0.8}));
  }
}
function drawSeries(svg, series, options={}) {
  const {width:w,height:h}=svgSetup(svg); const pad=34; drawGrid(svg,w,h,pad);
  const all=series.flatMap(item=>item.values.filter(Number.isFinite));
  if(!all.length){svgText(svg,w/2,h/2,'Waiting for market observations',{fill:css('--muted'),'font-size':13,'text-anchor':'middle'});return;}
  let min=options.min ?? Math.min(...all), max=options.max ?? Math.max(...all);
  if(min===max){const delta=Math.max(1,Math.abs(min)*0.0001);min-=delta;max+=delta;}
  if(options.center!==undefined){const span=Math.max(1e-9,Math.abs(max-options.center),Math.abs(min-options.center));min=options.center-span;max=options.center+span;}
  series.forEach(item=>{
    const points=[];
    item.values.forEach((value,index)=>{
      if(!Number.isFinite(value)) return;
      const x=pad+(w-pad*2)*(index/Math.max(1,item.values.length-1));
      const y=h-pad-(h-pad*2)*(value-min)/(max-min);
      points.push(`${x.toFixed(2)},${y.toFixed(2)}`);
    });
    if(points.length) svg.appendChild(svgNode('polyline',{points:points.join(' '),fill:'none',stroke:item.color,'stroke-width':2.2,'stroke-linejoin':'round','stroke-linecap':'round','vector-effect':'non-scaling-stroke'}));
  });
  svgText(svg,5,pad+4,formatNumber(max,2),{fill:css('--muted'),'font-size':10,'font-family':'SFMono-Regular, Menlo, monospace'});
  svgText(svg,5,h-pad+4,formatNumber(min,2),{fill:css('--muted'),'font-size':10,'font-family':'SFMono-Regular, Menlo, monospace'});
}
function drawBars(svg,values,colors,symmetric=false) {
  const {width:w,height:h}=svgSetup(svg); const pad=34; drawGrid(svg,w,h,pad);
  if(!values.length){svgText(svg,w/2,h/2,'Waiting for market observations',{fill:css('--muted'),'font-size':13,'text-anchor':'middle'});return;}
  const max=Math.max(1e-12,...values.map(value=>Math.abs(value)));
  const slot=(w-pad*2)/Math.max(1,values.length); const barWidth=Math.max(2,slot*.68);
  values.forEach((value,index)=>{
    const x=pad+slot*index+(slot-barWidth)/2; const base=symmetric?h/2:h-pad; const scale=(h-pad*2)/(symmetric?2:1); const barHeight=Math.abs(value)/max*scale;
    const y=value>=0?base-barHeight:base;
    const fill=Array.isArray(colors)?colors[index]:colors;
    svg.appendChild(svgNode('rect',{x,y,width:barWidth,height:Math.max(0.5,barHeight),rx:2,fill,opacity:0.86}));
  });
  if(symmetric) svg.appendChild(svgNode('line',{x1:pad,y1:h/2,x2:w-pad,y2:h/2,stroke:css('--line'),'stroke-width':1}));
}
function drawDepth(data) {
  const values=[]; const colors=[];
  [...data.bids].reverse().forEach(item=>{values.push(-item.quantity);colors.push(css('--buy'));});
  data.asks.forEach(item=>{values.push(item.quantity);colors.push(css('--sell'));});
  drawBars($('depthChart'),values,colors,true);
}
function renderCharts(data,d) {
  drawSeries($('priceChart'),[{values:history.mid,color:css('--accent-2')},{values:history.micro,color:css('--accent')}]);
  drawSeries($('spreadChart'),[{values:history.spread,color:css('--warning')}],{min:0});
  drawSeries($('imbalanceChart'),[{values:history.imbalance.map(value=>value*100),color:css('--accent')}],{min:0,max:100});
  drawDepth(data);
  drawBars($('tradeChart'),data.tradesFeed.slice(0,25).reverse().map(item=>item.side==='BUY'?item.quantity:-item.quantity),data.tradesFeed.slice(0,25).reverse().map(item=>item.side==='BUY'?css('--buy'):css('--sell')),true);
  drawSeries($('latencyChart'),[{values:history.latency,color:css('--accent-2')}],{min:0});
}
function renderInvariants(data,d) {
  const checks=[
    ['No crossed book', !(d.bid&&d.ask&&d.bid.price>=d.ask.price)],
    ['Non-negative quantities', [...data.bids,...data.asks].every(x=>x.quantity>0&&x.orders>0)],
    ['Bid depth sorted', data.bids.every((x,i,a)=>i===0||a[i-1].price>x.price)],
    ['Ask depth sorted', data.asks.every((x,i,a)=>i===0||a[i-1].price<x.price)],
    ['Valid imbalance', d.imbalance>=0&&d.imbalance<=1]
  ];
  $('invariants').innerHTML=checks.map(([n,ok])=>`<div class="invariant"><span>${n}</span><span class="${ok?'ok':'bad'}">${ok?'PASS':'FAIL'}</span></div>`).join('');
}
function renderDistribution(data) {
  const groups=[['Limit depth',data.bids.reduce((s,x)=>s+x.quantity,0)+data.asks.reduce((s,x)=>s+x.quantity,0)],['Traded volume',data.metrics.tradedQuantity],['Accepted',data.metrics.accepted],['Rejected',data.metrics.rejected],['Active orders',data.metrics.activeOrders]];
  const max=Math.max(1,...groups.map(x=>x[1]));
  $('distribution').innerHTML=groups.map(([name,v])=>`<div class="distribution-row"><span>${name}</span><div class="distribution-track"><div class="distribution-fill" style="width:${v/max*100}%"></div></div><strong>${formatNumber(v)}</strong></div>`).join('');
}
async function refresh() {
  const symbol=$('viewSymbol').value||$('symbol').value||1; const start=performance.now();
  try {
    const response=await fetch(`/api/state?symbol=${encodeURIComponent(symbol)}`,{cache:'no-store'}); if(!response.ok)throw new Error('State request failed');
    const data=await response.json(); const rtt=performance.now()-start; current=data; $('connection').textContent='Live';
    const d=topMetrics(data); pushHistory('latency',rtt); pushHistory('mid',d.mid); pushHistory('micro',d.micro); pushHistory('spread',d.spread); pushHistory('imbalance',d.imbalance); pushHistory('volume',data.metrics.tradedQuantity);
    renderMetrics(data.metrics,d); renderSymbols(data.symbols,symbol); renderSpread(data,d); renderBook('asks',data.asks,'ask'); renderBook('bids',data.bids,'bid'); renderTrades(data.tradesFeed); renderReports(data.reports); renderCharts(data,d); renderInvariants(data,d); renderDistribution(data);
  } catch(e){$('connection').textContent='Disconnected';}
}
async function submitFields(fields, quiet=false) { await post('/api/order',fields); if(!quiet)feedback(`Order ${fields.orderId} submitted.`); }
$('submitOrder').addEventListener('click',async()=>{try{const fields={orderId:$('orderId').value,clientId:$('clientId').value,symbol:$('symbol').value,side:$('side').value,type:$('type').value,price:$('type').value==='market'?0:$('price').value,quantity:$('quantity').value};await submitFields(fields);$('orderId').value=Number(fields.orderId)+1;$('viewSymbol').value=fields.symbol;await refresh();}catch(e){feedback(e.message,true);}});
$('cancelOrder').addEventListener('click',async()=>{try{await post('/api/cancel',{orderId:$('orderId').value,clientId:$('clientId').value,symbol:$('symbol').value});feedback(`Cancel sent for order ${$('orderId').value}.`);await refresh();}catch(e){feedback(e.message,true);}});
$('replaceOrder').addEventListener('click',async()=>{try{await post('/api/replace',{orderId:$('orderId').value,clientId:$('clientId').value,symbol:$('symbol').value,price:$('price').value,quantity:$('quantity').value});feedback(`Replace sent for order ${$('orderId').value}.`);await refresh();}catch(e){feedback(e.message,true);}});
$('reset').addEventListener('click',async()=>{await post('/api/reset');Object.values(history).forEach(a=>a.length=0);feedback('Exchange reset.');await refresh();});
document.querySelectorAll('[data-scenario]').forEach(b=>b.addEventListener('click',async()=>{try{await post('/api/scenario',{name:b.dataset.scenario});Object.values(history).forEach(a=>a.length=0);feedback(`${b.textContent} scenario loaded.`);await refresh();}catch(e){feedback(e.message,true);}}));
$('viewSymbol').addEventListener('change',()=>{Object.values(history).forEach(a=>a.length=0);refresh();});
$('type').addEventListener('change',()=>{$('price').disabled=$('type').value==='market';});
$('clearHistory').addEventListener('click',()=>{Object.values(history).forEach(a=>a.length=0);refresh();});
document.querySelectorAll('.tab').forEach(btn=>btn.addEventListener('click',()=>{document.querySelectorAll('.tab').forEach(x=>x.classList.toggle('active',x===btn));document.querySelectorAll('.tab-page').forEach(x=>x.classList.toggle('active',x.id===`tab-${btn.dataset.tab}`));setTimeout(()=>current&&renderCharts(current,topMetrics(current)),20);}));
function randomOrder() {
  const base=Number($('simBase').value); const range=Math.max(1,Number($('simRange').value)); const maxQty=Math.max(1,Number($('simQty').value)); const side=Math.random()<.5?'buy':'sell'; const aggressive=Math.random()<.16; let type=aggressive?'market':'limit'; if(!aggressive&&Math.random()<.08)type='ioc';
  const offset=Math.floor(Math.random()*(range+1)); const price=type==='market'?0:base+(side==='buy'?-offset:offset); return {orderId:nextSimOrderId++,clientId:700+Math.floor(Math.random()*20),symbol:$('viewSymbol').value||1,side,type,price,quantity:1+Math.floor(Math.random()*maxQty)};
}
$('startSimulation').addEventListener('click',()=>{if(simulationTimer)return;const rate=Math.max(1,Math.min(50,Number($('simRate').value)));simulationTimer=setInterval(async()=>{try{await submitFields(randomOrder(),true);}catch(_){}},1000/rate);$('simulationStatus').textContent=`Running at approximately ${rate} orders/second`;});
$('stopSimulation').addEventListener('click',()=>{clearInterval(simulationTimer);simulationTimer=null;$('simulationStatus').textContent='Stopped';});
window.addEventListener('resize',()=>current&&renderCharts(current,topMetrics(current)));
refresh(); setInterval(refresh,400);

let timelineTimer = null;
let timelineTotal = 0;
let timelineFrame = 0;

function renderHistoricalBook(data) {
  const side = (name, rows, cls) => `<div class="history-side"><strong>${name}</strong>${rows.map(r=>`<div class="history-level"><span class="${cls}">${r.price}</span><span>${formatNumber(r.quantity)} · ${r.orders} orders</span></div>`).join('') || '<div class="empty">Empty</div>'}</div>`;
  $('historicalBook').innerHTML = side('Bids', data.bids, 'bid-price') + side('Asks', data.asks, 'ask-price');
}
function renderTimelineActions(actions) {
  $('timelineActions').innerHTML = actions.length ? [...actions].reverse().map(a=>`<div class="timeline-action"><span>#${a.index}</span><span>${a.actor}</span><span class="${a.accepted?'accepted':'rejected'}">${a.kind} id ${a.orderId}${a.price!==undefined?` @ ${a.price} × ${a.quantity}`:''}</span></div>`).join('') : '<div class="empty">No commands</div>';
}
async function loadTimeline(frame = timelineFrame) {
  const symbol = $('viewSymbol').value || 1;
  const response = await fetch(`/api/timeline?symbol=${encodeURIComponent(symbol)}&frame=${encodeURIComponent(frame)}`, {cache:'no-store'});
  if (!response.ok) throw new Error('Timeline request failed');
  const data = await response.json();
  timelineTotal = data.timeline.total; timelineFrame = data.timeline.frame;
  $('timelineSlider').max = timelineTotal; $('timelineSlider').value = timelineFrame;
  $('timelineFrame').textContent = `Frame ${timelineFrame} / ${timelineTotal}`;
  $('timelineHash').textContent = `Hash ${data.metrics.stateHash}`;
  renderHistoricalBook(data); renderTimelineActions(data.timeline.actions);
}
function stopTimeline(){ clearInterval(timelineTimer); timelineTimer=null; $('timelinePlay').textContent='▶ Play'; }
$('timelineSlider').addEventListener('input', async e=>{ stopTimeline(); await loadTimeline(Number(e.target.value)); });
$('timelineStart').addEventListener('click', async()=>{stopTimeline(); await loadTimeline(0);});
$('timelineEnd').addEventListener('click', async()=>{stopTimeline(); await loadTimeline(timelineTotal);});
$('timelineBack').addEventListener('click', async()=>{stopTimeline(); await loadTimeline(Math.max(0,timelineFrame-1));});
$('timelineForward').addEventListener('click', async()=>{stopTimeline(); await loadTimeline(Math.min(timelineTotal,timelineFrame+1));});
$('timelinePlay').addEventListener('click',()=>{if(timelineTimer){stopTimeline();return;}$('timelinePlay').textContent='⏸ Pause'; const speed=Number($('timelineSpeed').value); timelineTimer=setInterval(async()=>{if(timelineFrame>=timelineTotal){stopTimeline();return;}await loadTimeline(timelineFrame+1);},speed);});
$('runArena').addEventListener('click',async()=>{try{$('arenaStatus').textContent='Running…';const r=await post('/api/arena',{steps:$('arenaSteps').value,dropEvery:$('dropEvery').value,duplicateEvery:$('duplicateEvery').value});$('arenaStatus').textContent=`Completed ${r.frames} recorded commands`;await refresh();await loadTimeline(r.frames);}catch(e){$('arenaStatus').textContent=e.message;}});
$('explainTrade').addEventListener('click',async()=>{try{const response=await fetch(`/api/explain?sequence=${encodeURIComponent($('explainSequence').value)}`);const data=await response.json();$('tradeExplanation').textContent=data.found?data.explanation:'Trade sequence not found in the current live session.';}catch(e){$('tradeExplanation').textContent=e.message;}});
$('trades').addEventListener('click',e=>{const row=e.target.closest('.feed-row');if(!row)return;const match=row.textContent.match(/#(\d+)/);if(match){$('explainSequence').value=match[1];document.querySelector('[data-tab="timemachine"]').click();$('explainTrade').click();}});
document.querySelector('[data-tab="timemachine"]').addEventListener('click',()=>loadTimeline(timelineTotal).catch(()=>{}));

// ---- Live external market data -------------------------------------------------
let liveTimer = null;
const liveHistory = {mid: [], micro: [], spread: [], rate: [], latency: []};
const paper = {inventory: 0, cash: 0};
let lastLiveData = null;

function formatQuantity(value) {
  if (!Number.isFinite(Number(value))) return '—';
  const n = Number(value);
  const digits = Math.abs(n) < 0.01 ? 8 : Math.abs(n) < 1 ? 6 : 4;
  return new Intl.NumberFormat(undefined, {minimumFractionDigits: 0, maximumFractionDigits: digits}).format(n);
}
function formatPrice(value) {
  if (!Number.isFinite(Number(value))) return '—';
  const n = Number(value);
  return new Intl.NumberFormat(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 8}).format(n);
}
function liveDerived(data){
  const bid=data.bids?.[0], ask=data.asks?.[0];
  const spread=bid&&ask?ask.price-bid.price:null;
  const mid=bid&&ask?(ask.price+bid.price)/2:null;
  const micro=bid&&ask&&(bid.quantity+ask.quantity)>0?(ask.price*bid.quantity+bid.price*ask.quantity)/(bid.quantity+ask.quantity):mid;
  const spreadBps=mid&&spread!=null?spread/mid*10000:null;
  return {bid,ask,spread,mid,micro,spreadBps};
}
const WORKING_LIVE_VENUES=new Set(['coinbase','kraken','binance','demo']);
const PLACEHOLDER_VENUES={
  alpaca:{title:'Alpaca IEX Equities',description:'Free IEX quotes and trades require Alpaca credentials. The adapter remains intentionally disabled in this release.',status:'Coming soon — credentials and entitlement validation required.',coverage:'US equities',data:'Top-of-book quotes + trades',auth:'API key required'},
  massive:{title:'Massive / Polygon Flat Files',description:'Your current account does not have Flat Files object access. The historical adapter is retained as a roadmap item.',status:'Coming soon — dataset subscription required.',coverage:'US equities historical',data:'Bars / trades / quotes',auth:'Paid entitlement required'},
  polygon:{title:'Massive / Polygon Live Stocks',description:'Real-time stock WebSocket integration is not enabled in this free build.',status:'Coming soon — live stock entitlement required.',coverage:'US equities live',data:'NBBO quotes + trades',auth:'API key + plan required'}
};
function isPlaceholderVenue(){ return !WORKING_LIVE_VENUES.has($('liveVenue').value); }
function setLiveCommand(){
  const venue=$('liveVenue').value;
  const defaults={coinbase:'btcusd',kraken:'btcusd',binance:'btcusd',demo:'btcusd',alpaca:'AAPL',massive:'AAPL',polygon:'AAPL'};
  const symbol=$('liveSymbol').value.trim() || defaults[venue];
  const spec=PLACEHOLDER_VENUES[venue];
  $('alpacaPlaceholder').hidden=true;
  $('massivePlaceholder').hidden=true;
  const generic=$('genericProviderPlaceholder');
  generic.hidden=!spec;
  if(spec){
    $('genericProviderTitle').textContent=spec.title;
    $('genericProviderDescription').textContent=spec.description;
    $('genericProviderStatus').textContent=spec.status;
    $('genericProviderGrid').innerHTML=[['Coverage',spec.coverage],['Data',spec.data],['Authentication',spec.auth],['Status','Coming soon']].map(([a,b])=>`<div><span>${a}</span><strong>${b}</strong></div>`).join('');
  }
  const commands={
    coinbase:`./scripts/run_live_coinbase.sh ${symbol.toLowerCase()}`,
    kraken:`./scripts/run_live_kraken.sh ${symbol.toLowerCase()}`,
    binance:`./scripts/run_live_binance.sh ${symbol.toLowerCase()}`,
    demo:'Built-in recorded session — no adapter required.'
  };
  $('liveCommand').textContent=spec?spec.status:commands[venue];
  $('connectLive').textContent=spec?'View integration status':venue==='demo'?'View demo session':'View live feed';
  $('stopLive').disabled=Boolean(spec)||venue==='demo';
}
function plannedVenuePanel(title, description){
  return `<div class="provider-coming-soon"><div><strong>${title}</strong><p>${description}</p></div></div>`;
}
function renderPlannedVenue(){
  stopLiveView();
  $('liveStatus').textContent='Alpaca integration is planned and intentionally disabled in this release.';
  $('liveConnectionBadge').textContent='COMING SOON';
  $('liveConnectionBadge').className='badge warning';
  $('liveBookSubtitle').textContent='ALPACA · US EQUITIES · provider roadmap';
  $('liveMetrics').innerHTML=[
    ['Venue','Alpaca'],['Status','Coming soon'],['Coverage','US equities'],['Feed','IEX planned'],
    ['Events','—'],['Trades','—'],['Sequence gaps','—'],['Uptime','—']
  ].map(([k,v])=>`<div class="live-metric"><small>${k}</small><strong>${v}</strong></div>`).join('');
  $('liveSpread').innerHTML=`
    <div class="live-quote-card"><small>Best bid</small><strong>—</strong></div>
    <div class="live-quote-card"><small>Midprice</small><strong>—</strong></div>
    <div class="live-quote-card"><small>Spread</small><strong>—</strong></div>
    <div class="live-quote-card"><small>Spread (bps)</small><strong>—</strong></div>`;
  $('liveAge').textContent='—'; $('liveRtt').textContent='—'; $('liveReconnects').textContent='—'; $('liveDropped').textContent='—';
  $('liveAsks').innerHTML=plannedVenuePanel('Market depth unavailable','Enable the authenticated Alpaca adapter to populate external quotes and trades.');
  $('liveBids').innerHTML='';
  $('liveTrades').innerHTML=plannedVenuePanel('No external trade stream','Alpaca trade data will appear here after the provider integration is configured.');
  $('liveHeatmap').innerHTML=plannedVenuePanel('Liquidity analytics pending','Displayed-liquidity analytics require an active venue stream.');
  $('liveQueueProxy').innerHTML=plannedVenuePanel('Queue proxy unavailable','Top-of-book quote data is required before this estimate can be calculated.');
  $('liveEventTimeline').innerHTML=plannedVenuePanel('No normalized events','The shared event timeline is ready for the future Alpaca adapter.');
  ['livePriceChart','liveSpreadChart','liveRateChart','liveLatencyChart'].forEach(id=>{
    const node=$(id); if(node) node.innerHTML=`<text x="400" y="108" text-anchor="middle" fill="#7d7893" font-size="15">Alpaca integration coming soon</text>`;
  });
  renderPaper({mid:null});
}

function renderHistoricalVenue(){
  stopLiveView();
  $('liveStatus').textContent='Massive Flat Files is configured as a historical-data provider.';
  $('liveConnectionBadge').textContent='HISTORICAL';
  $('liveConnectionBadge').className='badge accepted';
  $('liveBookSubtitle').textContent='MASSIVE · US STOCKS SIP · historical research catalog';
  $('liveMetrics').innerHTML=[
    ['Provider','Massive'],['Mode','Historical'],['Dataset','Minute aggregates'],['Ticker','AAPL'],
    ['Demo bars','390'],['Storage','SQLite'],['Endpoint','S3 compatible'],['Live matching','Disabled']
  ].map(([k,v])=>`<div class="live-metric"><small>${k}</small><strong>${v}</strong></div>`).join('');
  $('liveSpread').innerHTML=`
    <div class="live-quote-card"><small>Data mode</small><strong>Historical</strong></div>
    <div class="live-quote-card"><small>Granularity</small><strong>1 minute</strong></div>
    <div class="live-quote-card"><small>Demo ticker</small><strong>AAPL</strong></div>
    <div class="live-quote-card"><small>Bars loaded</small><strong>390</strong></div>`;
  $('liveAge').textContent='EOD'; $('liveRtt').textContent='—'; $('liveReconnects').textContent='—'; $('liveDropped').textContent='—';
  $('liveAsks').innerHTML=plannedVenuePanel('Historical provider selected','Massive Flat Files contains historical bars, trades, and quotes rather than a live order book. Use the download and import scripts to populate the research catalog.');
  $('liveBids').innerHTML='';
  $('liveTrades').innerHTML=plannedVenuePanel('Historical replay workflow','Imported bars are available in data/historical/market_history.db and data/historical/massive_aapl.json.');
  $('liveHeatmap').innerHTML=plannedVenuePanel('Research-ready storage','Use the historical catalog for backtests, execution analysis, and replay sessions.');
  $('liveQueueProxy').innerHTML=plannedVenuePanel('No Level-3 queue rank','Minute aggregates do not contain individual resting-order queues.');
  $('liveEventTimeline').innerHTML=plannedVenuePanel('Commands available','Run scripts/check_massive.sh, list_massive_files.sh, download_massive_file.sh, and import_massive_minute_aggs.py.');
  ['livePriceChart','liveSpreadChart','liveRateChart','liveLatencyChart'].forEach(id=>{
    const node=$(id); if(node) node.innerHTML=`<text x="400" y="108" text-anchor="middle" fill="#7d7893" font-size="15">Historical provider — use Research and Replay after import</text>`;
  });
  renderPaper({mid:null});
}
function renderGenericPlaceholder(){
  stopLiveView();
  const venue=$('liveVenue').value;
  const spec=PLACEHOLDER_VENUES[venue];
  $('liveStatus').textContent=spec?.status||'Provider unavailable';
  $('liveConnectionBadge').textContent='COMING SOON';
  $('liveConnectionBadge').className='badge warning';
  $('liveBookSubtitle').textContent=`${(spec?.title||venue).toUpperCase()} · provider roadmap`;
  $('liveMetrics').innerHTML=[['Provider',spec?.title||venue],['Status','Coming soon'],['Coverage',spec?.coverage||'—'],['Data',spec?.data||'—'],['Events','—'],['Trades','—'],['Sequence gaps','—'],['Uptime','—']].map(([k,v])=>`<div class="live-metric"><small>${k}</small><strong>${v}</strong></div>`).join('');
  $('liveSpread').innerHTML=['Best bid','Midprice','Spread','Spread (bps)'].map(k=>`<div class="live-quote-card"><small>${k}</small><strong>—</strong></div>`).join('');
  $('liveAge').textContent='—'; $('liveRtt').textContent='—'; $('liveReconnects').textContent='—'; $('liveDropped').textContent='—';
  $('liveAsks').innerHTML=plannedVenuePanel('Provider not enabled',spec?.description||'This provider is not enabled.');
  $('liveBids').innerHTML=''; $('liveTrades').innerHTML=plannedVenuePanel('No stream',spec?.status||'Coming soon');
  $('liveHeatmap').innerHTML=plannedVenuePanel('No liquidity data','Enable the provider to populate live analytics.');
  $('liveQueueProxy').innerHTML=plannedVenuePanel('No queue proxy','Requires an active depth feed.');
  $('liveEventTimeline').innerHTML=plannedVenuePanel('No events','This provider is intentionally presented as a roadmap item.');
  ['livePriceChart','liveSpreadChart','liveRateChart','liveLatencyChart'].forEach(id=>{const n=$(id);if(n)n.innerHTML='<text x="400" y="108" text-anchor="middle" fill="#7d7893" font-size="15">Provider coming soon</text>';});
  renderPaper({mid:null});
}
function renderLiveMetrics(data,d,rtt){
  const m=data.metrics||{};
  const rows=[['Venue',data.venue||'—'],['Symbol',data.symbol||'—'],['Events',m.events??0],['Trades',m.trades??0],['Messages/sec',formatNumber(m.updatesPerSecond,1)],['Sequence gaps',data.sequenceGaps??0],['Last sequence',data.lastSequence??'—'],['Uptime',`${formatNumber(m.uptimeSeconds,1)}s`]];
  $('liveMetrics').innerHTML=rows.map(([k,v])=>`<div class="live-metric"><small>${k}</small><strong>${v}</strong></div>`).join('');
  $('liveSpread').innerHTML=`
    <div class="live-quote-card"><small>Best bid</small><strong class="bid-price">${formatPrice(d.bid?.price)}</strong></div>
    <div class="live-quote-card"><small>Midprice</small><strong>${formatPrice(d.mid)}</strong></div>
    <div class="live-quote-card"><small>Spread</small><strong>${formatPrice(d.spread)}</strong></div>
    <div class="live-quote-card"><small>Spread (bps)</small><strong>${d.spreadBps==null?'—':formatNumber(d.spreadBps,3)}</strong></div>`;
  $('liveAge').textContent=data.ageMs==null?'—':`${formatNumber(data.ageMs,0)} ms`;
  $('liveRtt').textContent=`${formatNumber(rtt,1)} ms`;
  $('liveReconnects').textContent=m.reconnects??0;
  $('liveDropped').textContent=m.droppedEvents??0;
}
function renderLiveBook(target, rows, side){
  const max=Math.max(1e-12,...rows.map(x=>Number(x.quantity)||0));
  const ordered=side==='ask'?[...rows].reverse():rows;
  $(target).innerHTML=ordered.length?ordered.map(row=>`<div class="book-row"><div class="depth-bar" style="width:${Math.max(2,(Number(row.quantity)||0)/max*100)}%"></div><span class="${side}-price">${formatPrice(row.price)}</span><span>${formatQuantity(row.quantity)}</span><span>${row.orders??1}</span><span>${'▰'.repeat(Math.max(1,Math.round((Number(row.quantity)||0)/max*12)))}</span></div>`).join(''):'<div class="empty">No visible levels</div>';
}
function renderLiveTrades(trades){
  $('liveTrades').innerHTML=trades?.length?trades.map(t=>`<div class="feed-row"><span>#${t.sequence??'—'}</span><span class="${String(t.side).toLowerCase()}">${t.side} ${formatQuantity(t.quantity)} @ ${formatPrice(t.price)}</span><span>external</span></div>`).join(''):'<div class="empty">No live trades yet</div>';
}
function renderLiveHeatmap(data){
  const levels=[...(data.asks||[]).slice(0,8).reverse().map(x=>({...x,side:'ask'})),...(data.bids||[]).slice(0,8).map(x=>({...x,side:'bid'}))];
  const max=Math.max(1e-12,...levels.map(x=>Number(x.quantity)||0));
  $('liveHeatmap').innerHTML=levels.length?levels.map(x=>`<div class="heat-row"><span class="${x.side}-price">${formatPrice(x.price)}</span><div class="heat-track"><div class="heat-fill ${x.side}" style="width:${(Number(x.quantity)||0)/max*100}%"></div></div><span>${formatQuantity(x.quantity)}</span></div>`).join(''):'<div class="empty">No depth</div>';
}
function renderQueueProxy(d){
  if(!d.bid||!d.ask){$('liveQueueProxy').innerHTML='<div class="empty">No best level</div>';return;}
  const total=d.bid.quantity+d.ask.quantity||1;
  const bidShare=d.bid.quantity/total*100;
  $('liveQueueProxy').innerHTML=`<div class="queue-track"><div class="queue-ahead" style="width:${bidShare}%"></div></div><div class="queue-caption"><span>Best bid displayed ahead: ${formatQuantity(d.bid.quantity)}</span><span>Best ask displayed ahead: ${formatQuantity(d.ask.quantity)}</span></div>`;
}
function renderEventTimeline(data){
  const events=[];
  (data.trades||[]).slice(0,10).forEach(t=>events.push({type:'TRADE',meta:`${t.side} ${formatQuantity(t.quantity)} @ ${formatPrice(t.price)}`,seq:t.sequence}));
  (data.bids||[]).slice(0,4).forEach(x=>events.push({type:'BID',meta:`${formatQuantity(x.quantity)} @ ${formatPrice(x.price)}`,seq:data.lastSequence}));
  (data.asks||[]).slice(0,4).forEach(x=>events.push({type:'ASK',meta:`${formatQuantity(x.quantity)} @ ${formatPrice(x.price)}`,seq:data.lastSequence}));
  $('liveEventTimeline').innerHTML=events.length?events.map(e=>`<div class="event-chip"><div class="event-type">${e.type} #${e.seq??'—'}</div><div class="event-meta">${e.meta}</div></div>`).join(''):'<div class="empty">No normalized events yet</div>';
}
function renderPaper(d){
  const mark=d.mid??0;
  $('paperInventory').textContent=formatQuantity(paper.inventory);
  $('paperCash').textContent=formatNumber(paper.cash,2);
  $('paperPnl').textContent=formatNumber(paper.cash+paper.inventory*mark,2);
}
function renderLiveCharts(){
  drawSeries($('livePriceChart'),[{values:liveHistory.mid,color:css('--accent-2')},{values:liveHistory.micro,color:css('--accent')}]);
  drawSeries($('liveSpreadChart'),[{values:liveHistory.spread,color:css('--warning')}],{min:0});
  drawSeries($('liveRateChart'),[{values:liveHistory.rate,color:css('--accent')}],{min:0});
  drawBars($('liveLatencyChart'),liveHistory.latency.slice(-60),css('--accent-2'));
}
async function refreshLive(){
  if(isPlaceholderVenue()){ renderGenericPlaceholder(); return; }
  let venue=$('liveVenue').value; const symbol=$('liveSymbol').value.trim().toLowerCase(); if(venue==='demo') venue='binance'; const started=performance.now();
  try{
    const response=await fetch(`/api/live/state?venue=${encodeURIComponent(venue)}&symbol=${encodeURIComponent(symbol)}`,{cache:'no-store'});
    const data=await response.json(); const rtt=performance.now()-started; const d=liveDerived(data); lastLiveData=data;
    const publishedAge=Number(data.ageMs);
    const computedAge=data.lastEventTsNs?Math.max(0,Date.now()-Number(data.lastEventTsNs)/1e6):publishedAge;
    data.ageMs=Number.isFinite(computedAge)?computedAge:publishedAge;
    const age=Number(data.ageMs); const healthy=data.transportConnected===true && Number.isFinite(age) && age<5000;
    const stale=data.transportConnected===true && !healthy;
    const demo=data.mode==='DEMO_RECORDED';
    $('liveStatus').textContent=data.status||'unknown'; $('liveConnectionBadge').textContent=demo?'DEMO':healthy?'LIVE':stale?'STALE':'OFFLINE';
    $('liveConnectionBadge').className=`badge ${demo?'warning':healthy?'accepted':stale?'warning':'rejected'}`;
    $('liveBookSubtitle').textContent=`${String(data.venue||venue).toUpperCase()} · ${data.symbol||symbol.toUpperCase()} · session ${data.sessionId||'—'}`;
    renderLiveBook('liveAsks',data.asks||[],'ask'); renderLiveBook('liveBids',data.bids||[],'bid'); renderLiveMetrics(data,d,rtt); renderLiveTrades(data.trades||[]); renderLiveHeatmap(data); renderQueueProxy(d); renderEventTimeline(data); renderPaper(d);
    pushLive('mid',d.mid); pushLive('micro',d.micro); pushLive('spread',d.spread); pushLive('rate',Number(data.metrics?.updatesPerSecond)||0); pushLive('latency',rtt); renderLiveCharts();
  }catch(e){$('liveStatus').textContent=`Unavailable: ${e.message}`;$('liveConnectionBadge').textContent='OFFLINE';$('liveConnectionBadge').className='badge rejected';}
}
function pushLive(key,value){if(Number.isFinite(value)){liveHistory[key].push(value);if(liveHistory[key].length>MAX_HISTORY)liveHistory[key].shift();}}
function stopLiveView(){clearInterval(liveTimer);liveTimer=null;$('liveStatus').textContent='View stopped';}
$('paperBuy').addEventListener('click',()=>{const d=liveDerived(lastLiveData||{});if(!d.ask)return;const q=.001;paper.inventory+=q;paper.cash-=q*d.ask.price;renderPaper(d);});
$('paperSell').addEventListener('click',()=>{const d=liveDerived(lastLiveData||{});if(!d.bid)return;const q=.001;paper.inventory-=q;paper.cash+=q*d.bid.price;renderPaper(d);});
$('paperReset').addEventListener('click',()=>{paper.inventory=0;paper.cash=0;renderPaper(liveDerived(lastLiveData||{}));});
$('liveVenue').addEventListener('change',()=>{
  const venue=$('liveVenue').value;
  const defaults={coinbase:'btcusd',kraken:'btcusd',binance:'btcusd',demo:'btcusd',alpaca:'AAPL',massive:'AAPL',polygon:'AAPL'};
  $('liveSymbol').value=defaults[venue];
  setLiveCommand();
  if(isPlaceholderVenue()) renderGenericPlaceholder();
});
$('liveSymbol').addEventListener('input',setLiveCommand);
$('connectLive').addEventListener('click',async()=>{
  stopLiveView();
  Object.values(liveHistory).forEach(a=>a.length=0);
  if(isPlaceholderVenue()){ renderGenericPlaceholder(); return; }
  await refreshLive();
  liveTimer=setInterval(refreshLive,Math.max(100,Number($('liveRefreshMs').value)||500));
});
$('stopLive').addEventListener('click',stopLiveView);
document.querySelector('[data-tab="live"]').addEventListener('click',()=>{setLiveCommand();refreshLive();});
setLiveCommand();

