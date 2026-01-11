# MetricFlow Proxy (Deno Deploy)

Small HTTP proxy/router for **MetricFlowUE**.

It accepts a JSON **POST** with a `projectId`, selects the matching Google Apps Script endpoint, forwards the payload, and returns a normalized response.

## Request

**POST** `/`

Headers:
- `content-type: application/json`
- `x-api-key: <key>` (required only if `PROXY_API_KEY` is set)

## Environment variables

### `PROXY_API_KEY`
If set, requests must include `x-api-key` with the same value. If empty/missing, auth is disabled.

### `ROUTES_JSON`
A JSON map of `projectId -> Apps Script URL`.

Example:
```json
{
  "metricflow_test": "https://script.google.com/macros/s/XXX/exec",
  "flooded": "https://script.google.com/macros/s/YYY/exec"
}
```

## Deploy (Deno Deploy)

This service is located inside a larger repository.  
For deployment, use **only the `services/metricflow-proxy/` folder** as the project root.

1. In Deno Deploy, create a new project and connect a repository that contains this folder.
2. Configure Environment Variables:
- `PROXY_API_KEY` — shared secret for request authentication
- `ROUTES_JSON` — JSON map of `projectId -> Apps Script URL`
3. After deployment, send `POST` requests to the generated Deno Deploy URL.


