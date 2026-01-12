function corsHeaders(){
    return{
        "access-control-allow-origin": "*",
        "access-control-allow-methods": "POST,OPTIONS",
        "access-control-allow-headers": "content-type,x-api-key",
        "access-control-max-age": "86400",
    }
}

function json(resBody: unknown, status = 200){
    return new Response(JSON.stringify(resBody), {
        status,
        headers: {
            "content-type": "application/json; charset=utf-8",
            ...corsHeaders(),
        }
    })
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

export async function handler(req: Request): Promise<Response>{
    if(req.method === "OPTIONS"){
        return new Response(null, {status: 204, headers: corsHeaders()});
    }

    if(req.method !== "POST"){
        return json({ error: "Method not allowed" }, 405);
    }

    if(PROXY_API_KEY){
        const key = req.headers.get("x-api-key") ?? "";
        if(key !== PROXY_API_KEY){
            return json({ error: "Unauthorized"}, 401);
        }
    }

    const ct = req.headers.get("content-type") ?? "";
    if(!ct.includes("application/json")){
        return json({ error: "Expected application/json"}, 415)
    }

    
    let body: any;
    try{
        body = await req.json();
    } catch {
        return json({error: "Invalid JSON"}, 400);
    }

    const projectId = body?.projectId;
    if(!isValidPorjectId(projectId)){
        return json( { error: "Invalid projectId" }, 400);
    }

    const targetUrl = ROUTES[projectId];
    if(!targetUrl){
        return json({ error: "Unknown projectId", projectId }, 404);
    }

    let upstream: Response;
    try{
        upstream = await fetch(targetUrl, {
            method: "POST",
            headers: {
                "content-type": "application/json; charset=utf-8",
            },
            body: JSON.stringify(body)
        });
    } catch(e) {
        return json({ erorr: "Upstream fetch failed", projectId, details: String(e)}, 502);
    }

    const upstreamText = await upstream.text();
    return json({
        ok: upstream.ok,
        projectId,
        upstreamStatus: upstream.status,
        upstreamBody: upstreamText
    },
    upstream.ok ? 200 : 502);
}

export function startServer(port = 8000){
    return Deno.serve({ port }, handler)
}

startServer()