const CONFIG = {
  SPREADSHEET_ID: SpreadsheetApp.getActiveSpreadsheet().getId(),
  SESSIONS_SHEET: "AllSessions",
  EVENTS_SHEET: "AllEvents"
}

function getSessionsHeader_() {
  return [
    "projectId",
    "sessionId",
    "startedAtUTC",
    "endedAtUTC",
    "projectName",
    "projectVersion",
    "platform",
    "platformVersion",
    "updatedAtUTC",
  ];
}

function getEventsHeader_() {
  return [
    "projectId",
    "sessionId",
    "seq",
    "timestampUTC",
    "eventName",
    "sheet",
    "eventContextJson",
    "payloadJson",
    "receivedAtUTC",
  ];
}

function doPost(e) {
  try{
    const contentType = (e && e.postData && e.postData.type) ? String(e.postData.type) : "";
    if(contentType && !contentType.includes("application/json")){
      return resError(415, "expected_application_json", "Expected Content-Type: application/json");
    }

    const raw = (e && e.postData && e.postData.contents) ? e.postData.contents : "";
    if(!raw) return resError(400, "empty_body", "Request body is empty");

    let body;
    try{
      body = JSON.parse(raw);
    } catch(err){
      return resError(400, "invalid_json", "Body is not valid JSON");
    }

    const v = validateBase(body);
    if(!v.ok) return resError(400, v.code, v.message, v.details);

    if(!CONFIG.SPREADSHEET_ID){
      return resError(500, "missing_spreadsheet_id", "SPREADSHEET_ID is not found");
    }

    switch (body.op){
      case "upsertSession":
          return handleUpsertSession(body);
        case "appendEvents":
          return handleAppendEvents(body); 
    }
  }
  catch(err){
    return resError(500, "internal_error", String(err && err.message ? err.message : err));
  }
}

function handleUpsertSession(body){
  const ss = SpreadsheetApp.openById(CONFIG.SPREADSHEET_ID);
  const sh = ensureSheetWithHeader_(ss, CONFIG.SESSIONS_SHEET, getSessionsHeader_());
  const header = getSessionsHeader_();

  const s = body.session;

  const values = {
    projectId: body.projectId,
    sessionId: s.sessionId,
    startedAtUTC: s.startedAtUTC || "",
    endedAtUTC: s.endedAtUTC || "",
    projectName: s.projectName || "",
    projectVersion: s.projectVersion || "",
    platform: s.platform || "",
    platformVersion: s.platformVersion || "",
    updatedAtUTC: new Date().toISOString(),
  };

  const row = buildRowByHeader_(header, values);

  const keyMap = { sessionId: s.sessionId };
  const rowNumber = findRowByKeyMap_(sh, header, keyMap);

  if(rowNumber > 0){
    sh.getRange(rowNumber, 1, 1, header.length).setValues([row]);
    return resOk({ op: body.op, sessionId: s.sessionId, action: "update", rowNumber });
  }
  else{
    sh.appendRow(row);
    return resOk({ op: body.op, sessionId: s.sessionId, action: "insert" });
  }
}

function handleAppendEvents(body){
  return resOk({ op: body.op, sessionId: body.sessionId, appended: body.events.length });
}

function ensureSheetWithHeader_(ss, name, header){
  const safeName = sanitizeSheetName_(name);
  let sh = ss.getSheetByName(safeName);
  if(!sh) sh = ss.insertSheet(safeName);

  const lastRow = sh.getLastRow();
  if(lastRow === 0){
    sh.getRange(1, 1, 1, header.length).setValues([header]);
    return sh;
  }

  return sh;
}

function sanitizeSheetName_(name){
  let s = String(name || "").trim();
  if(!s) return "";
  s = s.replace(/[\[\]\:\*\?\/\\]/g, "_");
  if (s.length > 100) s = s.slice(0, 100);
  return s;
}

function findRowByKeyMap_(sh, header, keyMap){
  const idx = {};
  for(let i = 0; i < header.length; i++) idx[header[i]] = i;

  const lastRow = sh.getLastRow();
  if(lastRow < 2) return -1;

  const data = sh.getRange(2, 1, lastRow - 1, header.length).getValues();

  for(let r = 0; r < data.length; r++){
    let match = true;
    for (const key in keyMap){
      let col = idx[key];
      if(col === undefined) throw new Error("Unknown key column: " + key);
      
      const a = String(data[r][col] || "");
      const b = String(keyMap[key] || "");
      if(a !== b){
        match = false;
        break;
      }
    }
    if(match) return r + 2;
  }

  return -1;
}

function buildRowByHeader_(header, valuesByColumn){
  const row = new Array(header.length);

  for(let i = 0; i < header.length; i++){
    const colName = header[i];
    row[i] = (valuesByColumn && colName in valuesByColumn) ? valuesByColumn[colName] : "";
  }

  return row;
}

function validateBase(body) {
  if (!isObj(body)) return bad("invalid_body", "Body must be a JSON object");

  if (!isNonEmptyStr(body.projectId)) return bad("invalid_project_id", "projectId is required");
  if (!isNonEmptyStr(body.op)) return bad("invalid_op", "op is required");

  if (body.op === "upsertSession") return validateUpsertSession(body);
  if (body.op === "appendEvents") return validateAppendEvents(body);

  return bad("invalid_op", "Unknown op", { op: body.op });
}

function validateUpsertSession(body) {
  const s = body.session;
  if (!isObj(s)) return bad("invalid_session", "session must be an object");

  if (!isNonEmptyStr(s.sessionId)) return bad("invalid_session_id", "session.sessionId is required");
  if (!isNonEmptyStr(s.startedAtUTC)) return bad("invalid_startedAtUTC", "session.startedAtUTC is required");

  if (s.endedAtUTC !== undefined && !isStr(s.endedAtUTC)) {
    return bad("invalid_endedAtUTC", "session.endedAtUTC must be a string");
  }

  return ok();
}

function validateAppendEvents(body) {
  if (!isNonEmptyStr(body.sessionId)) return bad("invalid_session_id", "sessionId is required");
  if (!Array.isArray(body.events)) return bad("invalid_events", "events must be an array");

  for (let i = 0; i < body.events.length; i++) {
    const ev = body.events[i];
    if (!isObj(ev)) return bad("invalid_event", "Each event must be an object", { index: i });

    if (!isNonEmptyStr(ev.eventName)) return bad("invalid_event_name", "eventName is required", { index: i });
    if (!isNonEmptyStr(ev.timestampUTC)) return bad("invalid_timestampUTC", "timestampUTC is required", { index: i });

    if (!isNonEmptyStr(ev.seq)) return bad("invalid_seq", "seq is required", { index: i });
  }

  return ok();
}


function resOk(data) {
  return ContentService
    .createTextOutput(JSON.stringify({ ok: true, data: data || {} }))
    .setMimeType(ContentService.MimeType.JSON);
}

function resError(httpStatus, code, message, details){
  const payload = {ok: false, error: {httpStatus, code, message}};
  if(details !== undefined) payload.error.details = details;

  return ContentService.createTextOutput(JSON.stringify(payload)).setMimeType(ContentService.MimeType.JSON);
}

function ok(){ return { ok: true }; }
function bad(code, message, details) { return { ok: false, code, message, details }; }
function isObj(v) { return v !== null && typeof v === "object" && !Array.isArray(v); }
function isStr(v) { return typeof v === "string"; }
function isNonEmptyStr(v) { return typeof v === "string" && v.trim().length > 0; }