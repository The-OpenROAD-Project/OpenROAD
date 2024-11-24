import uvicorn

import fastapi
from fastapi.responses import JSONResponse


app = fastapi.FastAPI()


@app.post("/mock")
async def agent_retriever(payload: dict):
    """
    Mock implementation for ORAssistant API.
    Accepts a POST request and responds with a mock response.
    """
    query = payload.get("query", "")
    list_sources = payload.get("list_sources", False)

    response_data = {
        "response": f"Sent Message: {query}",
    }

    if list_sources:
        response_data["sources"] = ["source_1", "source_2", "source_3"]

    return JSONResponse(content=response_data)


port = 8080
while True:
    try:
        uvicorn.run(app, host="0.0.0.0", port=port)
        print(f"Mock API Server started at port {port}")
        break
    except Exception as e:
        port += 1
        continue
