function corsHeaders(){
    return{
        "access-control-allow-origin": "*",
        "access-control-allow-methods": "POST,OPTIONS",
        "access-control-allow-headers": "content-type,x-api-key",
        "access-control-max-age": "86400",
    }
}

function response(body: unknown, status = 200){
    return new Response(JSON.stringify(body), {
        status,
        headers: {
            "content-type": "application/json; charset=utf-8",
            ...corsHeaders(),
        }
    });
}

function err(httpStatus: number, code: string, message: string, details?: unknown, requestId?: string){
    const payload: any = {
        ok: false,
        error: { httpStatus, code, message },
        requestId
    };
    if(details !== undefined){ payload.error.details = details; }
    return response(payload, httpStatus);
}

function ok(data: unknown, requestId?: string){
    return response({ ok: true, data, requestId }, 200);
}

function isValidPorjectId(v: unknown): v is string{
    return typeof v === "string" && /^[a-zA-Z0-9._-]{1,64}$/.test(v);
}

function loadRoutesFromEnv(): Record<string, string> {
    const raw = Deno.env.get("ROUTES_JSON");
    if (!raw) return {};
    try {
        const parsed = JSON.parse(raw);
        if(!parsed || typeof parsed !== "object") return {};
        return parsed as Record<string, string>;
    }
    catch{
        return {};
    }
}

const ROUTES = loadRoutesFromEnv();
const PROXY_API_KEY = Deno.env.get("PROXY_API_KEY") ?? "";

function makeRequestId() {
  return crypto.randomUUID();
}

export async function handler(req: Request): Promise<Response>{
    const requestId = makeRequestId();

    if(req.method === "OPTIONS"){
        return new Response(null, {status: 204, headers: corsHeaders()});
    }

    if(req.method !== "POST"){
        return err(405, "method_not_allowed", "Method not allowed", { method: req.method }, requestId);
    }

    if(PROXY_API_KEY){
        const key = req.headers.get("x-api-key") ?? "";
        if(key !== PROXY_API_KEY){
            return err(401, "unauthorized", "Unauthorized", undefined, requestId);
            
        }
    }

    const ct = req.headers.get("content-type") ?? "";
    if(!ct.includes("application/json")){
        return err(415, "unsupported_media_type", "Expected application/json", { contentType: ct }, requestId);
    }

    
    let body: any;
    try{
        body = await req.json();
    } catch {
        return err(400, "invalid_json", "Invalid JSON", undefined, requestId);
    }

    const projectId = body?.projectId;
    if(!isValidPorjectId(projectId)){
        return err(400, "invalid_project_id", "Invalid projectId", { projectId }, requestId);
    }

    const targetUrl = ROUTES[projectId];
    if(!targetUrl){
        return err(404, "unknown_project_id", "Unknown projectId", { projectId }, requestId);
    }

    let upstream: Response;
    let upstreamText = "";
    try{
        upstream = await fetch(targetUrl, {
            method: "POST",
            headers: {
                "content-type": "application/json; charset=utf-8",
            },
            body: JSON.stringify(body)
        });
        upstreamText = await upstream.text();
    } catch(e) {
        return err(502, "upstream_fetch_failed", "Upstream fetch failed", { projectId, details: String(e) }, requestId);
    }

    let upstreamJson: any = null;
    try{
        upstreamJson = JSON.parse(upstreamText);
    }
    catch{
        return err(
            502, 
            "upstream_invalid_json", 
            "Upstream returned invalid JSON", 
            { projectId, upstreamStatus: upstream.status, upstreamBody: upstreamText }, 
            requestId
        );
    }

    if(upstreamJson?.ok === true){
        return ok(upstreamJson.data, requestId);
    }

    if(upstreamJson?.ok === false){
        const hs = Number(upstreamJson?.error?.httpStatus);
        const mappedStatus = Number.isFinite(hs) ? hs : 502;

        const payload = { ...upstreamJson, requestId };
        return response(payload, mappedStatus);
    }

    return err(
        502, 
        "upstream_bad_response", 
        "Upstream returned unexpected JSON shape", 
        { projectId, upstreamStatus: upstream.status, upstreamBody: upstreamJson }, 
        requestId
    );
}

export function startServer(port = 8000){
    return Deno.serve({ port }, handler)
}

startServer()