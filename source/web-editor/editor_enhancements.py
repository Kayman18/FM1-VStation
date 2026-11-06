import re

PRESET_VIEW = r'''<div id="presetView" class="view preset-view">
<div class="preset-manager-grid">
  <section class="pm-panel">
    <div class="pm-title"><div><h2>Factory Presets</h2><div class="small">175 built-in patches · alphabetical · read-only</div></div><div id="presetSummary" class="preset-summary"></div></div>
    <div class="pm-filter-row"><input id="presetSearch" placeholder="Search factory presets"><select id="categoryFilter"><option value="">All categories</option></select></div>
    <div id="presetList" class="preset-list"></div>
  </section>
  <section class="pm-panel">
    <div class="pm-title"><div><h2>User Preset Library</h2><div class="small">Stored locally in this browser. Name/category come from the patch fields above.</div></div><div id="userSummary" class="preset-summary"></div></div>
    <div class="library-primary"><button id="librarySaveNew" class="primary">SAVE CURRENT TO LIBRARY</button><button id="libraryUpdate">UPDATE SELECTED</button></div>
    <div class="pm-filter-row user-filter-row"><input id="userSearch" placeholder="Search user presets"><select id="userCategoryFilter"><option value="">All categories</option></select><select id="userSort"><option value="name">A → Z</option><option value="category">Category</option><option value="modified">Newest</option></select></div>
    <div id="userPresetList" class="preset-list user-preset-list"></div>
    <div class="library-actions"><button id="libraryExportSelected">EXPORT SELECTED JSON</button><button id="libraryDelete">DELETE SELECTED</button><button id="libraryImportPresets">IMPORT PRESET JSON</button><button id="libraryExportAll">EXPORT LIBRARY</button><button id="libraryImportAll">IMPORT LIBRARY</button></div>
    <input id="libraryPresetFiles" type="file" accept="application/json,.json" multiple hidden><input id="libraryBundleFile" type="file" accept="application/json,.json" hidden>
    <div id="libraryStatus" class="small library-status">No user preset selected.</div>
  </section>
</div>
<div class="json-strip"><div><b>Portable JSON</b><span class="small"> Export the current patch or load a standalone JSON directly to the FM1. Legacy TEST40 177-value JSON imports are accepted; removed fields are ignored.</span></div><div><button id="saveJson2">SAVE CURRENT JSON</button><button id="loadJson2">LOAD JSON TO FM1</button></div></div>
</div>'''

CSS = r'''
.preset-manager-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}.pm-panel{background:#e6e8e8;border:1px solid #8d9296;padding:10px;min-width:0}.pm-title{display:flex;justify-content:space-between;gap:12px;align-items:flex-start;margin-bottom:8px}.pm-title h2{margin:0 0 2px}.pm-filter-row{display:grid;grid-template-columns:minmax(160px,1fr) 160px;gap:6px;margin-bottom:7px}.user-filter-row{grid-template-columns:minmax(150px,1fr) 135px 110px}.pm-filter-row input,.pm-filter-row select{width:100%;padding:8px;border:1px solid #8b9095;background:#f7f8f8}.pm-panel .preset-list{height:390px}.user-preset-list .preset-item{grid-template-columns:1fr 105px 88px}.preset-item.user-selected{background:#6d4d22;box-shadow:inset 3px 0 #f2b84a}.preset-item .date{color:#88939d;font-size:9px;text-align:right}.library-primary,.library-actions{display:flex;gap:5px;flex-wrap:wrap;margin:7px 0}.library-primary button,.library-actions button,.json-strip button{background:#444950;color:#fff;border:1px solid #6c737c;padding:7px 9px;border-radius:3px;cursor:pointer}.library-primary button.primary{background:#315d9e;border-color:#6d8dbc}.library-actions button:disabled,.library-primary button:disabled{opacity:.42}.library-status{padding:6px 2px;min-height:27px}.json-strip{margin-top:12px;padding:9px 10px;background:#25292e;color:#e9eef3;display:flex;align-items:center;justify-content:space-between;gap:12px}.json-strip .small{color:#aeb7c0;margin-left:8px}.json-strip>div:last-child{display:flex;gap:5px;white-space:nowrap}
@media(max-width:900px){.preset-manager-grid{grid-template-columns:1fr}.pm-panel .preset-list{height:330px}.pm-filter-row,.user-filter-row{grid-template-columns:1fr}.json-strip{align-items:flex-start;flex-direction:column}}
'''

JS = r'''
const USER_LIBRARY_KEY='fm1.vstation.userPresets.v1';
let userLibrary=[],selectedUserId='';
function cleanText(v,fallback=''){return String(v??fallback).trim().slice(0,64)}
function makeId(){return 'p'+Date.now().toString(36)+Math.floor(Math.random()*1679616).toString(36).padStart(4,'0')}
function presetSnapshot(name,category,values=state,poolValue=pool,extra={}){const now=new Date().toISOString();return {format:'FM1-VStation-Preset',version:2,name:cleanText(name,'FM1 Patch')||'FM1 Patch',category:cleanText(category,'USER')||'USER',created:extra.created||now,modified:now,firmware:(firmware===2?'RC':firmware===1?'RC1':'TEST'+(firmware||42)),source:extra.source||{slot:currentSlot,name:patchInfo().name,category:patchInfo().category},pool:Math.max(1,Math.min(8,Number(poolValue)||8)),parameterCount:170,values:[...values].slice(0,170).map((v,i)=>clampParam(i,v))}}
function normalizePresetObject(data){if(!data||data.format!=='FM1-VStation-Preset')throw Error('Not an FM1 V-Station preset JSON file.');if(!Array.isArray(data.values)||(data.values.length!==170&&data.values.length!==177))throw Error('Preset must contain 170 parameters or a legacy 177-parameter TEST40 state.');const legacy=data.values.length===177;const vals=data.values.slice(0,170).map((v,i)=>clampParam(i,v));const obj=presetSnapshot(data.name||'Imported',data.category||'USER',vals,data.pool,{created:data.created,source:data.source});if(data.modified)obj.modified=String(data.modified);return {preset:obj,legacy}}
function downloadJson(obj,filename){const blob=new Blob([JSON.stringify(obj,null,2)+'\n'],{type:'application/json'}),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=filename;document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove()},0)}
function safeFilename(name,fallback='FM1-Preset'){return (String(name).replace(/[^a-z0-9._-]+/gi,'_').replace(/^_+|_+$/g,'')||fallback)}
function loadUserLibrary(){try{const raw=localStorage.getItem(USER_LIBRARY_KEY);if(!raw){userLibrary=[];return}const parsed=JSON.parse(raw);if(!Array.isArray(parsed))throw Error('bad library');userLibrary=[];for(const x of parsed){try{const n=normalizePresetObject(x).preset;n.id=cleanText(x.id)||makeId();n.created=x.created||n.created;n.modified=x.modified||n.modified;userLibrary.push(n)}catch(_){}}}catch(_){userLibrary=[]}}
function persistUserLibrary(){localStorage.setItem(USER_LIBRARY_KEY,JSON.stringify(userLibrary))}
function currentMetadata(){const pi=patchInfo();return {name:cleanText($('#jsonName').value,pi.name)||pi.name,category:cleanText($('#jsonCategory').value,pi.category)||pi.category}}
function renderUserLibrary(){const q=($('#userSearch')?.value||'').trim().toLowerCase(),cat=$('#userCategoryFilter')?.value||'',sort=$('#userSort')?.value||'name';const cats=[...new Set(userLibrary.map(p=>p.category).filter(Boolean))].sort((a,b)=>a.localeCompare(b,undefined,{numeric:true}));const cf=$('#userCategoryFilter');if(cf){const keep=cf.value;cf.innerHTML='<option value="">All categories</option>'+cats.map(c=>'<option></option>').join('');[...cf.options].slice(1).forEach((o,i)=>{o.value=cats[i];o.textContent=cats[i]});if(cats.includes(keep))cf.value=keep}let rows=userLibrary.filter(p=>(!cat||p.category===cat)&&(!q||(p.name+' '+p.category).toLowerCase().includes(q)));rows=[...rows].sort((a,b)=>sort==='modified'?String(b.modified).localeCompare(String(a.modified)):sort==='category'?(a.category.localeCompare(b.category,undefined,{numeric:true})||a.name.localeCompare(b.name,undefined,{numeric:true})):a.name.localeCompare(b.name,undefined,{numeric:true}));const list=$('#userPresetList');if(!list)return;list.innerHTML='';for(const p of rows){const b=document.createElement('button');b.className='preset-item'+(p.id===selectedUserId?' user-selected':'');const date=p.modified?new Date(p.modified).toLocaleDateString():'';b.innerHTML='<b></b><span class="cat"></span><span class="date"></span>';b.querySelector('b').textContent=p.name;b.querySelector('.cat').textContent=p.category;b.querySelector('.date').textContent=date;b.title='Click to load this user preset to the FM1';b.onclick=()=>loadUserPreset(p.id);list.appendChild(b)}$('#userSummary').textContent=rows.length+' of '+userLibrary.length+' user presets';const sel=userLibrary.find(p=>p.id===selectedUserId);$('#libraryUpdate').disabled=!sel;$('#libraryDelete').disabled=!sel;$('#libraryExportSelected').disabled=!sel;$('#libraryStatus').textContent=sel?'Selected: '+sel.name+' · '+sel.category:'No user preset selected.'}
function saveCurrentToLibrary(){const m=currentMetadata(),p=presetSnapshot(m.name,m.category);p.id=makeId();userLibrary.push(p);selectedUserId=p.id;persistUserLibrary();renderUserLibrary();note('Saved '+p.name+' to the local User Preset Library.')}
function updateSelectedLibrary(){const i=userLibrary.findIndex(p=>p.id===selectedUserId);if(i<0)return note('Select a user preset first.');const old=userLibrary[i],m=currentMetadata(),p=presetSnapshot(m.name,m.category,state,pool,{created:old.created,source:old.source});p.id=old.id;userLibrary[i]=p;persistUserLibrary();renderUserLibrary();note('Updated library preset '+p.name+'.')}
function loadUserPreset(id){const p=userLibrary.find(x=>x.id===id);if(!p)return;if(!requireConnection())return;selectedUserId=id;state.splice(0,170,...p.values);pool=p.pool;currentSlot=0;custom=true;customName=p.name;customCategory=p.category;$('#jsonName').value=p.name;$('#jsonCategory').value=p.category;updateAll();bulkTransition='library';sendFullState(state,0,true,pool,'library');captureBaseline(p.name,p.category);renderUserLibrary();note('Loaded user preset '+p.name+'.')}
function deleteSelectedLibrary(){const p=userLibrary.find(x=>x.id===selectedUserId);if(!p)return;if(!confirm('Delete user preset "'+p.name+'" from this browser?'))return;userLibrary=userLibrary.filter(x=>x.id!==selectedUserId);selectedUserId='';persistUserLibrary();renderUserLibrary();note('Deleted '+p.name+' from the local library.')}
function exportSelectedLibrary(){const p=userLibrary.find(x=>x.id===selectedUserId);if(!p)return;const {id,...out}=p;downloadJson(out,safeFilename(p.name)+'.json');note('Exported '+p.name+'.')}
async function importPresetFiles(files){let added=0,legacy=0,failed=0;for(const f of files){try{const n=normalizePresetObject(JSON.parse(await f.text()));const p=n.preset;p.id=makeId();userLibrary.push(p);added++;if(n.legacy)legacy++}catch(_){failed++}}persistUserLibrary();renderUserLibrary();note('Imported '+added+' preset'+(added===1?'':'s')+(legacy?' · '+legacy+' legacy file'+(legacy===1?'':'s')+' converted':'')+(failed?' · '+failed+' rejected':''));}
function exportLibrary(){const presets=userLibrary.map(({id,...p})=>p);downloadJson({format:'FM1-VStation-Preset-Library',version:1,exported:new Date().toISOString(),parameterCount:170,presets},'FM1-VStation-User-Library.json');note('Exported '+presets.length+' user presets as one library bundle.')}
async function importLibraryFile(file){try{const data=JSON.parse(await file.text());if(!data||data.format!=='FM1-VStation-Preset-Library'||!Array.isArray(data.presets))throw Error('Not an FM1 V-Station library bundle.');let added=0,failed=0;for(const x of data.presets){try{const p=normalizePresetObject(x).preset;p.id=makeId();userLibrary.push(p);added++}catch(_){failed++}}persistUserLibrary();renderUserLibrary();note('Merged '+added+' presets from library bundle'+(failed?' · '+failed+' rejected':''));}catch(e){note('Library import failed: '+e.message)}}
'''

def enhance(html: str) -> str:
    # Replace preset view as a single block.
    html, n = re.subn(r'<div id="presetView" class="view preset-view">.*?</div></div>\s*(?=<div id="allView")', PRESET_VIEW, html, count=1, flags=re.S)
    if n != 1:
        raise RuntimeError('presetView replacement failed')
    html = html.replace('</style>', CSS + '\n</style>')
    # Insert library code immediately after state declarations.
    marker = 'let perf={};'
    if marker not in html:
        raise RuntimeError('JS state marker missing')
    html = html.replace(marker, marker + '\n' + JS, 1)
    # Replace portable JSON functions with v2/legacy-compatible versions.
    html = re.sub(r"function saveJson\(\)\{.*?\}\nasync function loadJsonFile\(file\)\{.*?\}\n(?=function revertPatch)", r'''function saveJson(){const m=currentMetadata(),data=presetSnapshot(m.name,m.category);downloadJson(data,safeFilename(data.name)+'.json');note('Saved current patch JSON: '+data.name)}
async function loadJsonFile(file){try{const n=normalizePresetObject(JSON.parse(await file.text()));const data=n.preset;if(!requireConnection())return;state.splice(0,170,...data.values);pool=data.pool;currentSlot=0;custom=true;customName=data.name;customCategory=data.category;$('#jsonName').value=customName;$('#jsonCategory').value=customCategory;updateAll();bulkTransition='json';sendFullState(state,0,true,pool,'json');captureBaseline(customName,customCategory);note('Loaded JSON preset '+customName+(n.legacy?' · legacy removed fields ignored':''))}catch(e){note('JSON load failed: '+e.message)}}
''', html, count=1, flags=re.S)
    # User library startup after factory browser initialization.
    html = html.replace('renderPresetList()}', 'renderPresetList();loadUserLibrary();renderUserLibrary()}', 1)
    # Bind manager controls before tab bindings.
    bind_marker = "$('#randomize').onclick=randomizePatch;"
    bind_code = """$('#randomize').onclick=randomizePatch;$('#librarySaveNew').onclick=saveCurrentToLibrary;$('#libraryUpdate').onclick=updateSelectedLibrary;$('#libraryDelete').onclick=deleteSelectedLibrary;$('#libraryExportSelected').onclick=exportSelectedLibrary;$('#libraryImportPresets').onclick=()=>$('#libraryPresetFiles').click();$('#libraryPresetFiles').onchange=async e=>{if(e.target.files.length)await importPresetFiles([...e.target.files]);e.target.value=''};$('#libraryExportAll').onclick=exportLibrary;$('#libraryImportAll').onclick=()=>$('#libraryBundleFile').click();$('#libraryBundleFile').onchange=async e=>{if(e.target.files[0])await importLibraryFile(e.target.files[0]);e.target.value=''};$('#userSearch').oninput=renderUserLibrary;$('#userCategoryFilter').onchange=renderUserLibrary;$('#userSort').onchange=renderUserLibrary;"""
    if bind_marker not in html:
        raise RuntimeError('bind marker missing')
    html = html.replace(bind_marker, bind_code, 1)
    return html
